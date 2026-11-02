#include <pch.h>
#include <M_Sound.h>
#include <M_Mesh.h>
#include <M_Log.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>

// Static definitions
UINT32                        M_Sound::s_AudioDeviceID   = 0;
void                         *M_Sound::s_BGMStream       = nullptr;
SoundClip                     M_Sound::s_BGMClip         = {};
BOOL                          M_Sound::s_BGMLooping      = false;
FLOAT                         M_Sound::s_MasterVolume    = 1.0f;
FLOAT                         M_Sound::s_BGMVolume       = 0.7f;
FLOAT                         M_Sound::s_SFXVolume       = 1.0f;
BOOL                          M_Sound::s_IsInitialized   = false;
std::mutex                    M_Sound::s_Mutex;
UINT                          M_Sound::s_NextSoundID     = 0;
uMap<UINT, SoundClip> M_Sound::s_SoundByID;
uMap<String, UINT>    M_Sound::s_SoundByPath;
Array<void *>                 M_Sound::s_ActiveSFXStreams;

void M_Sound::Ensure_Initialized() {
    if (!s_IsInitialized) {
        Init();
    }
}

BOOL M_Sound::Init() {
    if (s_IsInitialized) return true;

    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        LOG(Warn, LogLevel::Core, "M_Sound::Init -> Failed to initialize SDL audio: {}", SDL_GetError());
        return false;
    }

    s_AudioDeviceID = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (s_AudioDeviceID == 0) {
        LOG(Warn, LogLevel::Core, "M_Sound::Init -> Failed to open default audio device: {}", SDL_GetError());
        return false;
    }

    SDL_ResumeAudioDevice(s_AudioDeviceID);
    s_IsInitialized = true;
    LOG(Info, LogLevel::Core, "M_Sound::Init -> Audio device [{}] initialized successfully.", s_AudioDeviceID);
    return true;
}

void M_Sound::Shutdown() {
    if (!s_IsInitialized) return;

    Stop_BGM();

    std::lock_guard<std::mutex> lock(s_Mutex);
    for (void *s : s_ActiveSFXStreams) {
        if (s) SDL_DestroyAudioStream(static_cast<SDL_AudioStream *>(s));
    }
    s_ActiveSFXStreams.clear();

    for (auto &[id, clip] : s_SoundByID) {
        if (clip.AudioBuf) {
            SDL_free(clip.AudioBuf);
            clip.AudioBuf = nullptr;
        }
    }
    s_SoundByID.clear();
    s_SoundByPath.clear();

    if (s_AudioDeviceID != 0) {
        SDL_CloseAudioDevice(s_AudioDeviceID);
        s_AudioDeviceID = 0;
    }

    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    s_IsInitialized = false;
    LOG(Info, LogLevel::Core, "M_Sound::Shutdown completed.");
}

void M_Sound::Tick(FLOAT deltaTime) {
    (void)deltaTime;
    if (!s_IsInitialized) return;

    std::lock_guard<std::mutex> lock(s_Mutex);

    // 1. Clean up completed one-shot sound effect streams
    for (size_t i = 0; i < s_ActiveSFXStreams.size(); ) {
        SDL_AudioStream *stream = static_cast<SDL_AudioStream *>(s_ActiveSFXStreams[i]);
        if (!stream || SDL_GetAudioStreamQueued(stream) <= 0) {
            if (stream) SDL_DestroyAudioStream(stream);
            s_ActiveSFXStreams[i] = s_ActiveSFXStreams.back();
            s_ActiveSFXStreams.pop_back();
        } else {
            ++i;
        }
    }

    // 2. Loop BGM if audio data is exhausted
    if (s_BGMStream && s_BGMLooping && s_BGMClip.AudioBuf) {
        SDL_AudioStream *stream = static_cast<SDL_AudioStream *>(s_BGMStream);
        if (SDL_GetAudioStreamQueued(stream) <= 0) {
            SDL_PutAudioStreamData(stream, s_BGMClip.AudioBuf, s_BGMClip.AudioLen);
        }
    }
}

UINT M_Sound::Load(const FilePath &path) {
    if (path.empty()) return 0;

    String key = path.string();
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        auto it = s_SoundByPath.find(key);
        if (it != s_SoundByPath.end()) return it->second;
    }

    SDL_AudioSpec spec{};
    Uint8 *buf = nullptr;
    Uint32 len = 0;

    if (!SDL_LoadWAV(key.c_str(), &spec, &buf, &len)) {
        String alt = "Assets/Audio/" + path.filename().string();
        if (!SDL_LoadWAV(alt.c_str(), &spec, &buf, &len)) {
            // Check mounted .pak archives via M_File
            Array<BYTE> pakData = M_File::Read_Bytes(key);
            if (pakData.empty()) {
                pakData = M_File::Read_Bytes(alt);
            }
            if (!pakData.empty()) {
                SDL_IOStream *io = SDL_IOFromConstMem(pakData.data(), pakData.size());
                if (!io || !SDL_LoadWAV_IO(io, true, &spec, &buf, &len)) {
                    LOG(Warn, LogLevel::Developer, "M_Sound::Load -> Could not parse WAV from pak: {}", SDL_GetError());
                    return 0;
                }
            } else {
                LOG(Warn, LogLevel::Developer, "M_Sound::Load -> Could not load [{}] or [{}]: {}", key, alt, SDL_GetError());
                return 0;
            }
        }
        key = alt;
    }

    // Standardize to SDL_AUDIO_S16LE, 2 channels (stereo interleaved) for spatial direction panning
    if (spec.format != SDL_AUDIO_S16LE || spec.channels != 2) {
        SDL_AudioSpec dstSpec{};
        dstSpec.format   = SDL_AUDIO_S16LE;
        dstSpec.channels = 2;
        dstSpec.freq     = spec.freq > 0 ? spec.freq : 44100;

        Uint8 *convertedBuf = nullptr;
        int    convertedLen = 0;
        if (SDL_ConvertAudioSamples(&spec, buf, static_cast<int>(len), &dstSpec, &convertedBuf, &convertedLen)) {
            SDL_free(buf);
            buf  = convertedBuf;
            len  = static_cast<Uint32>(convertedLen);
            spec = dstSpec;
        }
    }

    SoundClip clip{};
    clip.SoundID  = ++s_NextSoundID;
    clip.FilePath = key;
    clip.AudioBuf = buf;
    clip.AudioLen = len;
    std::memcpy(clip.SpecData, &spec, sizeof(SDL_AudioSpec));

    std::lock_guard<std::mutex> lock(s_Mutex);
    s_SoundByID[clip.SoundID] = clip;
    s_SoundByPath[key]        = clip.SoundID;
    LOG(Info, LogLevel::Core, "M_Sound::Load -> [{}] (ID={}, {} bytes, {}Hz)", key, clip.SoundID, len, spec.freq);
    return clip.SoundID;
}

void M_Sound::Play(UINT soundID, FLOAT volume) {
    Ensure_Initialized();
    if (!s_IsInitialized || s_AudioDeviceID == 0 || soundID == 0) return;

    SoundClip clip{};
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        auto it = s_SoundByID.find(soundID);
        if (it == s_SoundByID.end()) return;
        clip = it->second;
    }

    if (!clip.AudioBuf || clip.AudioLen == 0) return;

    SDL_AudioSpec spec{};
    std::memcpy(&spec, clip.SpecData, sizeof(SDL_AudioSpec));

    SDL_AudioStream *stream = SDL_CreateAudioStream(&spec, nullptr);
    if (!stream) return;

    if (!SDL_BindAudioStream(s_AudioDeviceID, stream)) {
        SDL_DestroyAudioStream(stream);
        return;
    }

    FLOAT gain = volume * s_MasterVolume * s_SFXVolume;
    SDL_SetAudioStreamGain(stream, gain);
    SDL_PutAudioStreamData(stream, clip.AudioBuf, clip.AudioLen);

    std::lock_guard<std::mutex> lock(s_Mutex);
    s_ActiveSFXStreams.push_back(stream);
}

void M_Sound::Play(const FilePath &path, FLOAT volume) {
    UINT id = Load(path);
    if (id != 0) {
        Play(id, volume);
    }
}

void M_Sound::Play(UINT soundID, BOOL bIsLocal, FLOAT volume) {
    if (bIsLocal) {
        Play(soundID, volume);
    } else {
        Play_At(soundID, M_Mesh::Get_CameraPos(), 35.0f, volume);
    }
}

void M_Sound::Play(const FilePath &path, BOOL bIsLocal, FLOAT volume) {
    UINT id = Load(path);
    if (id != 0) {
        Play(id, bIsLocal, volume);
    }
}

void M_Sound::Play(UINT soundID, const Vector3D &worldPos, FLOAT maxDistance, FLOAT volume) {
    Play_At(soundID, worldPos, maxDistance, volume);
}

void M_Sound::Play(const FilePath &path, const Vector3D &worldPos, FLOAT maxDistance, FLOAT volume) {
    UINT id = Load(path);
    if (id != 0) {
        Play_At(id, worldPos, maxDistance, volume);
    }
}

void M_Sound::Play(UINT soundID, BOOL bIsLocal, const Vector3D &worldPos, FLOAT maxDistance, FLOAT volume) {
    if (bIsLocal) {
        Play(soundID, volume);
    } else {
        Play_At(soundID, worldPos, maxDistance, volume);
    }
}

void M_Sound::Play(const FilePath &path, BOOL bIsLocal, const Vector3D &worldPos, FLOAT maxDistance, FLOAT volume) {
    UINT id = Load(path);
    if (id != 0) {
        Play(id, bIsLocal, worldPos, maxDistance, volume);
    }
}

void M_Sound::Play_At(
    UINT            soundID,
    const Vector3D &worldPos,
    const Vector3D &listenerPos,
    const Vector3D &listenerRight,
    const Vector3D &listenerForward,
    FLOAT           maxDistance,
    FLOAT           baseVolume) {

    Ensure_Initialized();
    if (!s_IsInitialized || s_AudioDeviceID == 0 || soundID == 0 || maxDistance <= 0.001f || baseVolume <= 0.001f) return;

    Vector3D delta = worldPos - listenerPos;
    FLOAT distSq = delta.LengthSq();
    FLOAT maxDistSq = maxDistance * maxDistance;

    // Beyond audible range: skip entirely (0 CPU, 0 memory allocation!)
    if (distSq >= maxDistSq) return;

    FLOAT dist = std::sqrt(distSq);
    FLOAT normDist = dist / maxDistance;
    FLOAT distFalloff = (1.0f - normDist) * (1.0f - normDist); // Smooth quadratic falloff

    // Compute direction unit vector
    Vector3D dir = (dist > 0.001f) ? (delta / dist) : Vector3D{ 0.0f, 0.0f, 1.0f };

    // Panning factor (-1.0 = full left, +1.0 = full right)
    FLOAT pan = dir.Dot(listenerRight);
    pan = std::clamp(pan, -1.0f, 1.0f);

    // Front/Rear acoustic damping: sounds behind the listener have slight acoustic damping (~15%)
    FLOAT fwdDot = dir.Dot(listenerForward);
    FLOAT rearFactor = (fwdDot < 0.0f) ? (1.0f + 0.15f * fwdDot) : 1.0f;

    FLOAT netDistanceVolume = baseVolume * distFalloff * rearFactor;
    if (netDistanceVolume <= 0.001f) return;

    // Constant-power stereo panning law (L^2 + R^2 = 1, eliminates center volume dip)
    FLOAT leftGain  = std::sqrt(0.5f * (1.0f - pan));
    FLOAT rightGain = std::sqrt(0.5f * (1.0f + pan));

    // If sound is dead center (within 5% deadzone), use standard hardware Play()
    if (std::abs(pan) < 0.05f) {
        Play(soundID, netDistanceVolume);
        return;
    }

    SoundClip clip{};
    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        auto it = s_SoundByID.find(soundID);
        if (it == s_SoundByID.end()) return;
        clip = it->second;
    }

    if (!clip.AudioBuf || clip.AudioLen == 0) return;

    SDL_AudioSpec spec{};
    std::memcpy(&spec, clip.SpecData, sizeof(SDL_AudioSpec));

    if (spec.format == SDL_AUDIO_S16LE && spec.channels == 2) {
        SDL_AudioStream *stream = SDL_CreateAudioStream(&spec, nullptr);
        if (!stream) return;

        if (!SDL_BindAudioStream(s_AudioDeviceID, stream)) {
            SDL_DestroyAudioStream(stream);
            return;
        }

        // Thread-local scratch buffer avoids any dynamic allocations per playback
        thread_local Array<int16_t> s_PannedBuffer;
        size_t sampleCount = clip.AudioLen / sizeof(int16_t);
        if (s_PannedBuffer.size() < sampleCount) {
            s_PannedBuffer.resize(sampleCount);
        }

        FLOAT overallGain = netDistanceVolume * s_MasterVolume * s_SFXVolume;
        FLOAT lGain = leftGain * overallGain;
        FLOAT rGain = rightGain * overallGain;

        const int16_t *src = reinterpret_cast<const int16_t *>(clip.AudioBuf);
        int16_t *dst = s_PannedBuffer.data();
        size_t pairCount = sampleCount / 2;

        for (size_t i = 0; i < pairCount; ++i) {
            int32_t l = static_cast<int32_t>(src[i * 2 + 0] * lGain);
            int32_t r = static_cast<int32_t>(src[i * 2 + 1] * rGain);
            dst[i * 2 + 0] = static_cast<int16_t>(std::clamp(l, -32768, 32767));
            dst[i * 2 + 1] = static_cast<int16_t>(std::clamp(r, -32768, 32767));
        }

        SDL_SetAudioStreamGain(stream, 1.0f);
        SDL_PutAudioStreamData(stream, dst, static_cast<int>(clip.AudioLen));

        std::lock_guard<std::mutex> lock(s_Mutex);
        s_ActiveSFXStreams.push_back(stream);
    } else {
        Play(soundID, netDistanceVolume);
    }
}

void M_Sound::Play_At(UINT soundID, const Vector3D &worldPos, const Vector3D &listenerPos, FLOAT maxDistance, FLOAT baseVolume) {
    Play_At(soundID, worldPos, listenerPos, M_Mesh::Get_CameraRight(), M_Mesh::Get_CameraForward(), maxDistance, baseVolume);
}

void M_Sound::Play_At(UINT soundID, const Vector3D &worldPos, FLOAT maxDistance, FLOAT baseVolume) {
    Play_At(soundID, worldPos, M_Mesh::Get_CameraPos(), M_Mesh::Get_CameraRight(), M_Mesh::Get_CameraForward(), maxDistance, baseVolume);
}

void M_Sound::Play_At(const FilePath &path, const Vector3D &worldPos, FLOAT maxDistance, FLOAT baseVolume) {
    UINT id = Load(path);
    if (id != 0) {
        Play_At(id, worldPos, maxDistance, baseVolume);
    }
}

void M_Sound::Play_At(const FilePath &path, const Vector3D &worldPos, const Vector3D &listenerPos, FLOAT maxDistance, FLOAT baseVolume) {
    UINT id = Load(path);
    if (id != 0) {
        Play_At(id, worldPos, listenerPos, maxDistance, baseVolume);
    }
}

void M_Sound::Play_At(
    const FilePath &path,
    const Vector3D &worldPos,
    const Vector3D &listenerPos,
    const Vector3D &listenerRight,
    const Vector3D &listenerForward,
    FLOAT           maxDistance,
    FLOAT           baseVolume) {
    UINT id = Load(path);
    if (id != 0) {
        Play_At(id, worldPos, listenerPos, listenerRight, listenerForward, maxDistance, baseVolume);
    }
}

void M_Sound::Play_BGM(const FilePath &path, FLOAT volume, BOOL bLoop) {
    Ensure_Initialized();
    if (!s_IsInitialized || s_AudioDeviceID == 0) return;

    Stop_BGM();

    UINT bgmID = Load(path);
    if (bgmID == 0) return;

    {
        std::lock_guard<std::mutex> lock(s_Mutex);
        s_BGMClip    = s_SoundByID[bgmID];
        s_BGMLooping = bLoop;
    }

    SDL_AudioSpec spec{};
    std::memcpy(&spec, s_BGMClip.SpecData, sizeof(SDL_AudioSpec));

    SDL_AudioStream *stream = SDL_CreateAudioStream(&spec, nullptr);
    if (!stream) return;

    if (!SDL_BindAudioStream(s_AudioDeviceID, stream)) {
        SDL_DestroyAudioStream(stream);
        return;
    }

    FLOAT gain = volume * s_MasterVolume * s_BGMVolume;
    SDL_SetAudioStreamGain(stream, gain);
    SDL_PutAudioStreamData(stream, s_BGMClip.AudioBuf, s_BGMClip.AudioLen);

    s_BGMStream = stream;
}

void M_Sound::Stop_BGM() {
    if (s_BGMStream) {
        SDL_DestroyAudioStream(static_cast<SDL_AudioStream *>(s_BGMStream));
        s_BGMStream = nullptr;
    }
}

void M_Sound::Pause_BGM() {
    if (s_BGMStream) {
        SDL_PauseAudioStreamDevice(static_cast<SDL_AudioStream *>(s_BGMStream));
    }
}

void M_Sound::Resume_BGM() {
    if (s_BGMStream) {
        SDL_ResumeAudioStreamDevice(static_cast<SDL_AudioStream *>(s_BGMStream));
    }
}

void M_Sound::Set_MasterVolume(FLOAT volume) {
    s_MasterVolume = (volume < 0.0f) ? 0.0f : (volume > 1.0f ? 1.0f : volume);
}

void M_Sound::Set_BGMVolume(FLOAT volume) {
    s_BGMVolume = (volume < 0.0f) ? 0.0f : (volume > 1.0f ? 1.0f : volume);
    if (s_BGMStream) {
        SDL_SetAudioStreamGain(static_cast<SDL_AudioStream *>(s_BGMStream), s_BGMVolume * s_MasterVolume);
    }
}

void M_Sound::Set_SFXVolume(FLOAT volume) {
    s_SFXVolume = (volume < 0.0f) ? 0.0f : (volume > 1.0f ? 1.0f : volume);
}

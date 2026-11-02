#pragma once

#include <pch.h>
#include <U_Variables.h>

// ============================================================================
// M_Sound: Aurora Engine Audio System (SDL3 Native Audio Stream Engine)
// ============================================================================
// 1. Zero external DLL dependencies: links directly with SDL3 audio streams.
// 2. Plays .wav sound effects (SFX) and background music (BGM) with volume control.
// 3. Audio clips are cached in memory: zero disk lag on repeating sounds.
// 4. Headless awareness: dedicated servers skip audio initialization with 0 overhead.
// ============================================================================

struct SoundClip {
    UINT    SoundID   = 0;
    String  FilePath;
    void   *AudioBuf  = nullptr; // Raw PCM byte buffer (allocated by SDL_LoadWAV)
    UINT32  AudioLen  = 0;       // Buffer length in bytes
    BYTE    SpecData[32] = {};   // SDL_AudioSpec buffer
};

class M_Sound {
private:
    static UINT32                        s_AudioDeviceID;
    static void                         *s_BGMStream;
    static SoundClip                     s_BGMClip;
    static BOOL                          s_BGMLooping;
    static FLOAT                         s_MasterVolume;
    static FLOAT                         s_BGMVolume;
    static FLOAT                         s_SFXVolume;
    static BOOL                          s_IsInitialized;
    static std::mutex                    s_Mutex;
    static UINT                          s_NextSoundID;
    static uMap<UINT, SoundClip> s_SoundByID;
    static uMap<String, UINT>    s_SoundByPath;
    static Array<void *>                 s_ActiveSFXStreams;

    static void Ensure_Initialized();

public:
    // Initializes the SDL3 audio subsystem and opens the default playback device
    static BOOL Init();

    // Shuts down audio device and releases all audio buffers
    static void Shutdown();

    // Per-frame tick: reaps completed SFX streams and maintains looping BGM
    static void Tick(FLOAT deltaTime);

    // Preloads a .wav audio file into RAM and returns its unique SoundID (0 on failure)
    static UINT Load(const FilePath &path);

    // Plays a sound effect (2D local playback or 3D world spatial playback)
    // - bIsLocal = true: 2D version playing directly on user with NO direction or falloff (like player is the source)
    // - bIsLocal = false: 3D version automatically calculating directional stereo panning & distance falloff
    static void Play(UINT soundID, FLOAT volume = 1.0f);
    static void Play(const FilePath &path, FLOAT volume = 1.0f);

    static void Play(UINT soundID, BOOL bIsLocal, FLOAT volume = 1.0f);
    static void Play(const FilePath &path, BOOL bIsLocal, FLOAT volume = 1.0f);

    static void Play(UINT soundID, const Vector3D &worldPos, FLOAT maxDistance = 35.0f, FLOAT volume = 1.0f);
    static void Play(const FilePath &path, const Vector3D &worldPos, FLOAT maxDistance = 35.0f, FLOAT volume = 1.0f);

    static void Play(UINT soundID, BOOL bIsLocal, const Vector3D &worldPos, FLOAT maxDistance = 35.0f, FLOAT volume = 1.0f);
    static void Play(const FilePath &path, BOOL bIsLocal, const Vector3D &worldPos, FLOAT maxDistance = 35.0f, FLOAT volume = 1.0f);

    // Distance-attenuated & directional stereo 3D sound playback (WoW style distance falloff + stereo panning)
    // Automatically uses the active camera position, right, and forward vectors when not specified.
    static void Play_At(
        UINT            soundID,
        const Vector3D &worldPos,
        FLOAT           maxDistance = 35.0f,
        FLOAT           baseVolume  = 1.0f);

    static void Play_At(
        const FilePath &path,
        const Vector3D &worldPos,
        FLOAT           maxDistance = 35.0f,
        FLOAT           baseVolume  = 1.0f);

    static void Play_At(
        UINT            soundID,
        const Vector3D &worldPos,
        const Vector3D &listenerPos,
        FLOAT           maxDistance = 35.0f,
        FLOAT           baseVolume  = 1.0f);

    static void Play_At(
        const FilePath &path,
        const Vector3D &worldPos,
        const Vector3D &listenerPos,
        FLOAT           maxDistance = 35.0f,
        FLOAT           baseVolume  = 1.0f);

    // Full 3D listener orientation overload (worldPos + listener position, right, and forward vectors)
    static void Play_At(
        UINT            soundID,
        const Vector3D &worldPos,
        const Vector3D &listenerPos,
        const Vector3D &listenerRight,
        const Vector3D &listenerForward,
        FLOAT           maxDistance = 35.0f,
        FLOAT           baseVolume  = 1.0f);

    static void Play_At(
        const FilePath &path,
        const Vector3D &worldPos,
        const Vector3D &listenerPos,
        const Vector3D &listenerRight,
        const Vector3D &listenerForward,
        FLOAT           maxDistance = 35.0f,
        FLOAT           baseVolume  = 1.0f);

    // Plays background music (BGM) with optional looping
    static void Play_BGM(const FilePath &path, FLOAT volume = 0.5f, BOOL bLoop = true);

    // Stops currently playing background music
    static void Stop_BGM();

    // Pauses or resumes background music
    static void Pause_BGM();
    static void Resume_BGM();

    // Volume controls (0.0f to 1.0f)
    static void Set_MasterVolume(FLOAT volume);
    static void Set_BGMVolume(FLOAT volume);
    static void Set_SFXVolume(FLOAT volume);

    [[nodiscard]] static FLOAT Get_MasterVolume() { return s_MasterVolume; }
    [[nodiscard]] static FLOAT Get_BGMVolume()    { return s_BGMVolume; }
    [[nodiscard]] static FLOAT Get_SFXVolume()    { return s_SFXVolume; }
    [[nodiscard]] static BOOL  Is_Initialized()   { return s_IsInitialized; }
};

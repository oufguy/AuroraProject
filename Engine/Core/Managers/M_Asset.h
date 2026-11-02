#pragma once

#include <pch.h>
#include <U_Variables.h>
#include <U_Colors.h>
#include <M_File.h>
#include <M_Mesh.h>
#include <M_Texture.h>
#include <M_Config.h>
#include <M_Sound.h>
#include <M_Font.h>

// ============================================================================
// M_Asset (Unified Asset & Resource Manager)
// ============================================================================
// Single unified access point for all assets: meshes, textures, configs,
// and files. Users do not need to hunt for separate loader headers.
//
// Usage:
//   MeshData cubeMesh = M_Asset::Create_Cube(COLOR_CYAN);
//   TextureData tex   = M_Asset::Create_Checkerboard(64, 64, 8);
// ============================================================================
class M_Asset {
public:
    // ------------------------------------------------------------------------
    // 3D Meshes
    // ------------------------------------------------------------------------
    static inline MeshData Create_Cube(const COLOR &color = COLOR_WHITE) {
        return M_Mesh::Create_Cube(color);
    }

    static inline MeshData Create_Triangle(const COLOR &c1 = COLOR_RED, const COLOR &c2 = COLOR_GREEN, const COLOR &c3 = COLOR_BLUE) {
        return M_Mesh::Create_Triangle(c1, c2, c3);
    }

    static inline MeshData Create_Quad(const COLOR &color = COLOR_WHITE) {
        return M_Mesh::Create_Quad(color);
    }

    static inline MeshData Create_Ground(FLOAT width = 100.0f, FLOAT depth = 100.0f, const COLOR &color = COLOR(55, 120, 55, 255), FLOAT uvTiling = 10.0f) {
        return M_Mesh::Create_Ground(width, depth, color, uvTiling);
    }

    static inline MeshData Load_Mesh(const FilePath &filePath) {
        return M_Mesh::Load(filePath);
    }

    // ------------------------------------------------------------------------
    // Textures & Images (Returns cached Texture ID for GPU)
    // ------------------------------------------------------------------------
    static inline UINT Create_Checkerboard(UINT32 width = 64, UINT32 height = 64, UINT32 checkerSize = 8,
                                           const COLOR &c1 = COLOR_WHITE, const COLOR &c2 = COLOR_GRAY) {
        return M_Texture::Create_Checkerboard(width, height, checkerSize, c1, c2);
    }

    static inline UINT Load_Texture(const FilePath &filePath) {
        return M_Texture::Load(filePath);
    }

    // ------------------------------------------------------------------------
    // Configurations (.ini)
    // ------------------------------------------------------------------------
    static inline BOOL Load_Config(M_Config &outConfig, const FilePath &filePath, const String &defaultContent = "") {
        return outConfig.Load(filePath, defaultContent);
    }

    static inline BOOL Save_Config(const M_Config &inConfig, const FilePath &filePath) {
        return inConfig.Save(filePath);
    }

    // ------------------------------------------------------------------------
    // Audio (.wav)
    // ------------------------------------------------------------------------
    static inline UINT Load_Sound(const FilePath &filePath) {
        return M_Sound::Load(filePath);
    }

    static inline void Play_Sound(const FilePath &filePath, FLOAT volume = 1.0f) {
        M_Sound::Play(filePath, volume);
    }

    static inline void Play_Sound(UINT soundID, FLOAT volume = 1.0f) {
        M_Sound::Play(soundID, volume);
    }

    // Explicit local 2D playback (no direction / falloff)
    static inline void Play_Sound(const FilePath &filePath, BOOL bIsLocal, FLOAT volume = 1.0f) {
        M_Sound::Play(filePath, bIsLocal, volume);
    }

    static inline void Play_Sound(UINT soundID, BOOL bIsLocal, FLOAT volume = 1.0f) {
        M_Sound::Play(soundID, bIsLocal, volume);
    }

    // Automatic 3D spatial playback from world position
    static inline void Play_Sound(const FilePath &filePath, const Vector3D &worldPos, FLOAT maxDistance = 35.0f, FLOAT volume = 1.0f) {
        M_Sound::Play(filePath, worldPos, maxDistance, volume);
    }

    static inline void Play_Sound(UINT soundID, const Vector3D &worldPos, FLOAT maxDistance = 35.0f, FLOAT volume = 1.0f) {
        M_Sound::Play(soundID, worldPos, maxDistance, volume);
    }

    static inline void Play_Sound(const FilePath &filePath, BOOL bIsLocal, const Vector3D &worldPos, FLOAT maxDistance = 35.0f, FLOAT volume = 1.0f) {
        M_Sound::Play(filePath, bIsLocal, worldPos, maxDistance, volume);
    }

    static inline void Play_Sound(UINT soundID, BOOL bIsLocal, const Vector3D &worldPos, FLOAT maxDistance = 35.0f, FLOAT volume = 1.0f) {
        M_Sound::Play(soundID, bIsLocal, worldPos, maxDistance, volume);
    }

    static inline void Play_Sound_At(const FilePath &filePath, const Vector3D &worldPos, FLOAT maxDistance = 35.0f, FLOAT volume = 1.0f) {
        M_Sound::Play_At(filePath, worldPos, maxDistance, volume);
    }

    static inline void Play_Sound_At(UINT soundID, const Vector3D &worldPos, FLOAT maxDistance = 35.0f, FLOAT volume = 1.0f) {
        M_Sound::Play_At(soundID, worldPos, maxDistance, volume);
    }

    static inline void Play_Sound_At(const FilePath &filePath, const Vector3D &worldPos, const Vector3D &listenerPos, FLOAT maxDistance = 35.0f, FLOAT volume = 1.0f) {
        M_Sound::Play_At(filePath, worldPos, listenerPos, maxDistance, volume);
    }

    static inline void Play_Sound_At(UINT soundID, const Vector3D &worldPos, const Vector3D &listenerPos, FLOAT maxDistance = 35.0f, FLOAT volume = 1.0f) {
        M_Sound::Play_At(soundID, worldPos, listenerPos, maxDistance, volume);
    }

    static inline void Play_Sound_At(
        const FilePath &filePath,
        const Vector3D &worldPos,
        const Vector3D &listenerPos,
        const Vector3D &listenerRight,
        const Vector3D &listenerForward,
        FLOAT           maxDistance = 35.0f,
        FLOAT           volume      = 1.0f) {
        M_Sound::Play_At(filePath, worldPos, listenerPos, listenerRight, listenerForward, maxDistance, volume);
    }

    static inline void Play_Sound_At(
        UINT            soundID,
        const Vector3D &worldPos,
        const Vector3D &listenerPos,
        const Vector3D &listenerRight,
        const Vector3D &listenerForward,
        FLOAT           maxDistance = 35.0f,
        FLOAT           volume      = 1.0f) {
        M_Sound::Play_At(soundID, worldPos, listenerPos, listenerRight, listenerForward, maxDistance, volume);
    }

    static inline void Play_BGM(const FilePath &filePath, FLOAT volume = 0.5f, BOOL bLoop = true) {
        M_Sound::Play_BGM(filePath, volume, bLoop);
    }

    static inline void Stop_BGM() {
        M_Sound::Stop_BGM();
    }

    // ------------------------------------------------------------------------
    // General Files
    // ------------------------------------------------------------------------
    static inline String Read_File(const FilePath &filePath) {
        return M_File::Open(filePath);
    }

    static inline BOOL Write_File(const FilePath &filePath, const String &content, BOOL bOverwrite = true) {
        return M_File::Write(filePath, content, bOverwrite);
    }

    static inline BOOL Exists(const FilePath &filePath) {
        return M_File::Exists(filePath);
    }
};

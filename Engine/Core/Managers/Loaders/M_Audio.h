#pragma once

#include <M_Sound.h>

// ============================================================================
// M_Audio: Aurora Engine Audio System (Clean Canonical Alias for M_Sound)
// ============================================================================
// Provides standard M_Audio naming convention for:
//   - 2D Sound Effects & Looping BGM (SDL3 native audio stream)
//   - Distance-attenuated 3D world audio playback (Play_At)
//   - Volume channels (Master, SFX, BGM) & memory caching
// ============================================================================
using M_Audio = M_Sound;

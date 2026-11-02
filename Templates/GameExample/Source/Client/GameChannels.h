#pragma once

#include <C_Entity.h>

// ============================================================================
// GameChannels.h — Custom Project Collision & Trace Channels
// ============================================================================
// Exactly like Unreal Engine's Config / DefaultEngine.ini:
//
// In Unreal Engine:
//   - ECC_GameTraceChannel1 through ECC_GameTraceChannel18 (Trace Channels)
//   - ECC_GameTraceChannel / ECC_GameObjectChannel (Object Channels)
//
// In Aurora Engine:
//   - Default Engine Channels (TraceChannel & HitChannel):
//       None         = TraceChannel1 / HitChannel1
//       Visibility   = TraceChannel2 / HitChannel2
//       Camera       = TraceChannel3 / HitChannel3
//       WorldStatic  = TraceChannel4 / HitChannel4
//       WorldDynamic = TraceChannel5 / HitChannel5
//       Pawn         = TraceChannel6 / HitChannel6
//       Monster      = TraceChannel7 / HitChannel7
//       Projectile   = TraceChannel8 / HitChannel8
//
//   - Custom Game Channels (TraceChannel9..16 / HitChannel9..16):
//       Aliased below with zero changes to engine source code!
// ============================================================================

namespace GameTraceChannel {
    // Custom Trace Channels (What the raycast/shape query searches for)
    constexpr TraceChannel WeaponLoS      = TraceChannel::TraceChannel9;  // Bullet/Arrow Line of Sight
    constexpr TraceChannel Interaction    = TraceChannel::TraceChannel10; // NPC / Object dialogue / action
    constexpr TraceChannel LootPickup     = TraceChannel::TraceChannel11; // Magnetic item vacuum pickup
    constexpr TraceChannel SpellTargeting = TraceChannel::TraceChannel12; // Magic reticle queries
}

namespace GameHitChannel {
    // Custom Hit / Object Channels (Physical nature of world entities)
    constexpr HitChannel ShieldBarrier    = HitChannel::HitChannel9;  // Forcefields, mana shields
    constexpr HitChannel MiningOre        = HitChannel::HitChannel10; // Mineable mineral nodes
    constexpr HitChannel FlightZone       = HitChannel::HitChannel11; // Aerial flight boundaries
    constexpr HitChannel WaterVolume      = HitChannel::HitChannel12; // Swimmable lakes / rivers
}

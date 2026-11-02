#pragma once

#include <pch.h>
#include <U_Variables.h>
#include <U_Colors.h>
#include <U_Math.h>

// ============================================================================
// Entity Categories (Identifies what kind of object it is)
// ============================================================================
enum class EntityType : BYTE {
    None        = 0,
    Camera      = 1, // 3D Camera Viewport (Always invisible & transparent, no mesh)
    StaticMesh  = 2, // Fixed environment props, terrain, rocks, buildings
    DynamicMesh = 3, // Moving objects, characters, monsters, interactive props
    Character   = 4, // Playable player characters
    Monster     = 5, // Enemies, mobs, bosses
    NPC         = 6, // Quest givers, merchants, friendly characters
    Projectile  = 7, // Missiles, bullets, spell effects
    ItemDrop    = 8, // Loot, gold, pickup items on the ground
    Trigger     = 9  // Invisible zones, teleports, event volumes
};

// ============================================================================
// TraceShape: 3D Geometry swept during tracing (Unreal Engine Compatible)
// ============================================================================
enum class TraceShape : BYTE {
    Line    = 0, // 0-thickness line ray (mouse picking, line of sight, bullets)
    Sphere  = 1, // Sphere radius sweep (fireballs, explosion radius, melee swings)
    Box     = 2, // 3D Box extents (platforms, doors, rectangular barriers)
    Capsule = 3  // Capsule: radius + half-height (Standard 3D character shape!)
};

// ============================================================================
// TraceMode: Trace Collection Strategy
// ============================================================================
enum class TraceMode : BYTE {
    Single = 0, // Stops at the very first blocking hit (closest impact)
    Multi  = 1  // Penetrates and collects ALL hits along the trace (piercing lasers, cleaves)
};

// ============================================================================
// TraceChannel: What the ray / shape query is searching for (Unreal Trace Channel)
// ============================================================================
// Underlying raw slots (TraceChannel1..16) match Unreal GameTraceChannel slots.
// Named defaults provide beginner-friendly Unreal Engine standard channels.
// ============================================================================
enum class TraceChannel : BYTE {
    // Numbered Channel Slots (Unreal Underlying Slots)
    TraceChannel1  = 0,
    TraceChannel2  = 1,
    TraceChannel3  = 2,
    TraceChannel4  = 3,
    TraceChannel5  = 4,
    TraceChannel6  = 5,
    TraceChannel7  = 6,
    TraceChannel8  = 7,
    TraceChannel9  = 8,
    TraceChannel10 = 9,
    TraceChannel11 = 10,
    TraceChannel12 = 11,
    TraceChannel13 = 12,
    TraceChannel14 = 13,
    TraceChannel15 = 14,
    TraceChannel16 = 15,

    // Unreal Engine Default Named Aliases
    None         = TraceChannel1,  // 0
    Visibility   = TraceChannel2,  // 1 (Mouse clicks, targeting, cursor queries, Line of Sight)
    Camera       = TraceChannel3,  // 2 (3rd-person chase camera spring arm)
    WorldStatic  = TraceChannel4,  // 3 (Immovable terrain, buildings, rocks, static walls)
    WorldDynamic = TraceChannel5,  // 4 (Moving platforms, doors, destructibles, elevators)
    Pawn         = TraceChannel6,  // 5 (Playable characters, friendly NPCs)
    Monster      = TraceChannel7,  // 6 (Enemies, mobs, bosses)
    Projectile   = TraceChannel8,  // 7 (Arrows, bullets, spells)

    All          = 0xFF
};

// ============================================================================
// HitChannel: Physical collision category of an Entity (Unreal Object Channel)
// ============================================================================
// Underlying raw slots (HitChannel1..16) match Unreal GameObjectChannel slots.
// Named defaults provide beginner-friendly Unreal Engine standard channels.
// ============================================================================
enum class HitChannel : BYTE {
    // Numbered Channel Slots (Unreal Underlying Slots)
    HitChannel1  = 0,
    HitChannel2  = 1,
    HitChannel3  = 2,
    HitChannel4  = 3,
    HitChannel5  = 4,
    HitChannel6  = 5,
    HitChannel7  = 6,
    HitChannel8  = 7,
    HitChannel9  = 8,
    HitChannel10 = 9,
    HitChannel11 = 10,
    HitChannel12 = 11,
    HitChannel13 = 12,
    HitChannel14 = 13,
    HitChannel15 = 14,
    HitChannel16 = 15,

    // Unreal Engine Default Named Aliases
    None         = HitChannel1,  // 0
    Visibility   = HitChannel2,  // 1
    Camera       = HitChannel3,  // 2
    WorldStatic  = HitChannel4,  // 3 (Immovable terrain, buildings, rocks, static walls)
    WorldDynamic = HitChannel5,  // 4 (Moving platforms, doors, destructibles, elevators)
    Pawn         = HitChannel6,  // 5 (Playable characters, friendly NPCs)
    Monster      = HitChannel7,  // 6 (Enemies, mobs, bosses)
    Projectile   = HitChannel8,  // 7 (Arrows, bullets, spells)

    All          = 0xFF
};

// Aliases for compatibility
using ObjectChannel    = HitChannel;
using CollisionChannel = HitChannel;

// Collision Response
enum class CollisionResponse : BYTE {
    Ignore  = 0, // Passes through completely with zero interaction
    Overlap = 1, // Passes through but generates trigger / touch events
    Block   = 2  // Physically blocks movement and stops traces
};

// ============================================================================
// CollisionProfile: Ergonomic 1-Liner Presets (Unreal Engine Style)
// ============================================================================
enum class CollisionProfile : BYTE {
    Custom          = 0, // Hand-tuned channels and responses
    NoCollision     = 1, // Completely invisible to physics and traces
    BlockAll        = 2, // Solid immovable barrier (Terrain, Rocks, Buildings)
    BlockAllDynamic = 3, // Moving solid barrier (Doors, Platforms, Elevators)
    Entity          = 4, // Living 3D character, monster, NPC (Standard Entity preset)
    Entity_Ghost    = 5, // Passes through entities, but triggers events (Overlap)
    Trigger         = 6, // Invisible event volume / teleporter zone (Overlap only)
    Spectator       = 7  // Ghost camera / GM flight mode (no collision)
};

// ============================================================================
// C_Entity (The Mother of All World Objects)
// ============================================================================
// Contains ONLY the pure essentials every 3D object has:
//   - Unique ID & Type
//   - Active & Visible flags
//   - Collision channels & responses
//   - 3D World Transform (Position, Rotation)
//   - Virtual lifecycle & gameplay interaction hooks (Unreal AActor style)
// ============================================================================
class C_Entity {
public:
    INT               ID                = 0;
    EntityType        Type              = EntityType::None;
    BOOL              bIsActive         = true;
    BOOL              bIsVisible        = true;
    BOOL              bPendingDestroy   = false;
    BOOL              bHasCollision     = false;
    HitChannel        HitType           = HitChannel::WorldStatic;
    CollisionResponse CollisionBehavior = CollisionResponse::Block;
    CollisionProfile  Profile           = CollisionProfile::Custom;

    // 3D Transform in World Space
    Vector3D   Position        = Vector3D(0.0f, 0.0f, 0.0f);
    Vector3D   Rotation        = Vector3D(0.0f, 0.0f, 0.0f); // x = Pitch, y = Yaw, z = Roll (Degrees)

    virtual ~C_Entity() = default;

    // ------------------------------------------------------------------------
    // Virtual Lifecycle & Interaction Hooks (Unreal Engine AActor Style)
    // ------------------------------------------------------------------------
    virtual void BeginPlay() {}
    virtual void Tick(FLOAT deltaTime) { (void)deltaTime; }
    virtual void EndPlay() {}

    // Gameplay Interaction Hooks (Polymorphic dispatch — zero manual switches needed!)
    virtual void On_Interact(C_Entity *instigator) { (void)instigator; }
    virtual void Take_Damage(FLOAT amount, C_Entity *instigator = nullptr) { (void)amount; (void)instigator; }
    virtual void On_Death() {}

    // 3D Bounding Box Query (Overridden by derived classes like StaticMesh and Character)
    [[nodiscard]] virtual AABB Get_WorldAABB() const;

    // Safe Downcasting Helper (Unreal Cast<T> style!)
    template<typename T>
    [[nodiscard]] inline T *As() {
        return dynamic_cast<T*>(this);
    }

    template<typename T>
    [[nodiscard]] inline const T *As() const {
        return dynamic_cast<const T*>(this);
    }

    // Visibility
    void Set_Visible(BOOL visible) { bIsVisible = visible; }
    [[nodiscard]] BOOL Is_Visible() const { return bIsVisible; }

    // Collision Configuration
    void Set_Collision(BOOL bEnable) { bHasCollision = bEnable; }
    [[nodiscard]] BOOL Has_Collision() const { return bHasCollision; }

    // Collision Preset Profile (1-liner setup like Unreal Engine!)
    void Set_CollisionProfile(CollisionProfile profile);
    [[nodiscard]] CollisionProfile Get_CollisionProfile() const { return Profile; }

    // Trace Collision Filter (Can this entity be hit by a given TraceChannel?)
    [[nodiscard]] virtual BOOL Can_Hit_Trace(TraceChannel traceChannel) const;

    // Hit Channel Filter (Can this entity be hit by a given HitChannel?)
    [[nodiscard]] virtual BOOL Can_Hit_HitChannel(HitChannel hitChannel) const;

    void Set_HitChannel(HitChannel ch) { HitType = ch; }
    [[nodiscard]] HitChannel Get_HitChannel() const { return HitType; }

    void Set_CollisionChannel(HitChannel ch) { Set_HitChannel(ch); }
    [[nodiscard]] HitChannel Get_CollisionChannel() const { return Get_HitChannel(); }

    void Set_CollisionResponse(CollisionResponse resp) { CollisionBehavior = resp; }
    [[nodiscard]] CollisionResponse Get_CollisionResponse() const { return CollisionBehavior; }

    // Transform
    void Set_Position(const Vector3D &pos) { Position = pos; }
    void Set_Rotation(const Vector3D &rotDeg) { Rotation = rotDeg; }
    void Add_Rotation(FLOAT deltaPitch, FLOAT deltaYaw, FLOAT deltaRoll = 0.0f) {
        Rotation.x += deltaPitch;
        Rotation.y += deltaYaw;
        Rotation.z += deltaRoll;
    }

    // Lifecycle
    void Destroy() {
        bPendingDestroy = true;
        bIsActive       = false;
    }

    // Distance helpers
    [[nodiscard]] inline FLOAT Get_Distance_To(const Vector3D &point) const {
        return (Position - point).Length();
    }

    [[nodiscard]] inline FLOAT Get_Distance_To(const C_Entity &other) const {
        return (Position - other.Position).Length();
    }

    // Audio helpers (Plays distance-attenuated 3D audio originating at this entity)
    void Play_Sound(const FilePath &soundPath, FLOAT maxDistance = 35.0f, FLOAT volume = 1.0f);

    // Direction Vectors & Movement Helpers (implemented in C_Entity.cpp)
    [[nodiscard]] Vector3D Get_Forward() const;
    [[nodiscard]] Vector3D Get_Right() const;
    [[nodiscard]] Vector3D Get_Up() const;

    void Move(const Vector3D &worldOffset);
    void Move_Forward(FLOAT distance);
    void Move_Right(FLOAT distance);
    void Move_Up(FLOAT distance);
};

// Legacy compatibility alias
using E_Entity = C_Entity;

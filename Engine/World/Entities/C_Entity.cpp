#include <C_Entity.h>
#include <U_Raycast.h>
#include <M_Sound.h>

void C_Entity::Play_Sound(const FilePath &soundPath, FLOAT maxDistance, FLOAT volume) {
    M_Sound::Play_At(soundPath, Position, maxDistance, volume);
}

Vector3D C_Entity::Get_Forward() const {
    return U_Raycast::Get_ForwardVector(Rotation);
}

Vector3D C_Entity::Get_Right() const {
    return U_Raycast::Get_RightVector(Rotation);
}

Vector3D C_Entity::Get_Up() const {
    return U_Raycast::Get_UpVector(Rotation);
}

void C_Entity::Move(const Vector3D &worldOffset) {
    Position.x += worldOffset.x;
    Position.y += worldOffset.y;
    Position.z += worldOffset.z;
}

void C_Entity::Move_Forward(FLOAT distance) {
    Vector3D fwd = Get_Forward();
    Position.x += fwd.x * distance;
    Position.y += fwd.y * distance;
    Position.z += fwd.z * distance;
}

void C_Entity::Move_Right(FLOAT distance) {
    Vector3D right = Get_Right();
    Position.x += right.x * distance;
    Position.y += right.y * distance;
    Position.z += right.z * distance;
}

void C_Entity::Move_Up(FLOAT distance) {
    Position.y += distance;
}

void C_Entity::Set_CollisionProfile(CollisionProfile profile) {
    Profile = profile;
    switch (profile) {
        case CollisionProfile::NoCollision:
            bHasCollision     = false;
            HitType           = HitChannel::None;
            CollisionBehavior = CollisionResponse::Ignore;
            break;
        case CollisionProfile::BlockAll:
            bHasCollision     = true;
            HitType           = HitChannel::WorldStatic;
            CollisionBehavior = CollisionResponse::Block;
            break;
        case CollisionProfile::BlockAllDynamic:
            bHasCollision     = true;
            HitType           = HitChannel::WorldDynamic;
            CollisionBehavior = CollisionResponse::Block;
            break;
        case CollisionProfile::Entity:
            bHasCollision     = true;
            HitType           = HitChannel::Pawn;
            CollisionBehavior = CollisionResponse::Block;
            break;
        case CollisionProfile::Entity_Ghost:
            bHasCollision     = true;
            HitType           = HitChannel::Pawn;
            CollisionBehavior = CollisionResponse::Overlap;
            break;
        case CollisionProfile::Trigger:
            bHasCollision     = true;
            HitType           = HitChannel::Visibility;
            CollisionBehavior = CollisionResponse::Overlap;
            break;
        case CollisionProfile::Spectator:
            bHasCollision     = false;
            HitType           = HitChannel::None;
            CollisionBehavior = CollisionResponse::Ignore;
            break;
        case CollisionProfile::Custom:
            break;
    }
}

AABB C_Entity::Get_WorldAABB() const {
    return AABB(Position - Vector3D(0.5f, 0.5f, 0.5f), Position + Vector3D(0.5f, 0.5f, 0.5f));
}

BOOL C_Entity::Can_Hit_Trace(TraceChannel traceChannel) const {
    if (!bHasCollision && traceChannel != TraceChannel::Visibility) {
        return false;
    }
    if (CollisionBehavior == CollisionResponse::Ignore) {
        return false;
    }
    if (traceChannel == TraceChannel::All) {
        return true;
    }
    if (traceChannel == TraceChannel::Visibility) {
        return true;
    }
    if (traceChannel == TraceChannel::Camera) {
        return HitType == HitChannel::WorldStatic || HitType == HitChannel::WorldDynamic;
    }
    // Direct channel slot match (e.g. TraceChannel::WorldStatic matches HitChannel::WorldStatic)
    return static_cast<BYTE>(HitType) == static_cast<BYTE>(traceChannel);
}

BOOL C_Entity::Can_Hit_HitChannel(HitChannel hitChannel) const {
    if (!bHasCollision && hitChannel != HitChannel::Visibility) {
        return false;
    }
    if (CollisionBehavior == CollisionResponse::Ignore) {
        return false;
    }
    if (hitChannel == HitChannel::All) {
        return true;
    }
    return HitType == hitChannel;
}

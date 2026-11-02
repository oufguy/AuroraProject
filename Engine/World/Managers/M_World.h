#pragma once

#include <pch.h>
#include <U_Variables.h>
#include <U_Math.h>
#include <M_Log.h>
#include <M_Threads.h>
#include <M_Mesh.h>
#include <M_Texture.h>
#include <C_Entity.h>
#include <C_StaticMesh.h>
#include <C_DynamicMesh.h>
#include <C_Character.h>
#include <C_Camera.h>
#include <RHI_Manager.h>

// ============================================================================
// World Manager (Central Entity Spawning, O(1) ID Lookup, Spatial Grid & Frustum Culling)
// ============================================================================
class M_World {
public:
    static constexpr FLOAT SPATIAL_CELL_SIZE     = 32.0f;
    static constexpr FLOAT INV_SPATIAL_CELL_SIZE = 1.0f / SPATIAL_CELL_SIZE;

private:
    struct TransparentDrawItem {
        const C_StaticMesh *Entity = nullptr;
        FLOAT               DistSq = 0.0f;
    };

    static Array<UniquePtr<C_Entity>>    s_Entities;
    static Array<C_DynamicMesh*>         s_DynamicEntities;
    static uMap<INT, C_Entity*>          s_EntityByID;
    static uMap<INT64, Array<C_Entity*>> s_SpatialGrid;
    static uMap<INT, INT64>              s_EntityCellKey;
    static Array<TransparentDrawItem>    s_TransparentQueue;
    static String                        s_WorldName;
    static INT                           s_NextEntityID;
    static INT                           s_VisibleCount;
    static INT                           s_CulledCount;
    static BOOL                          s_IsLoaded;
    static BOOL                          s_IsUnloading;
    static BOOL                          s_HasPendingDestroys;

    // Packs 2D grid coordinates (cellX, cellZ) into a single 64-bit hash key
    [[nodiscard]] static constexpr INT64 Pack_CellKey(INT cellX, INT cellZ) {
        return (static_cast<INT64>(cellX) << 32) | static_cast<UINT32>(cellZ);
    }

    // Computes the 64-bit Spatial Grid cell key for any 3D world position
    [[nodiscard]] static INT64 Compute_CellKey(const Vector3D &pos);

    // Removes an Entity pointer from a specific Spatial Grid cell bucket
    static void Remove_FromSpatialCell(INT64 cellKey, C_Entity *entity);

    // Updates an Entity's bucket in the 2D Spatial Hash Grid if it crossed a 32-unit cell boundary
    static void Sync_SpatialCell(C_Entity &entity);

public:
    // ========================================================================
    // 1. Engine Lifecycle (Called Automatically by Core)
    // ========================================================================
    static void BeginPlay(StringView defaultWorldName = "DefaultWorld");
    static void Tick(FLOAT dt);
    static void Shutdown();

    // ========================================================================
    // 2. World State
    // ========================================================================
    [[nodiscard]] static inline BOOL Is_Loaded() {
        return s_IsLoaded && !s_IsUnloading;
    }

    [[nodiscard]] static inline const String &Get_Name() {
        return s_WorldName;
    }

    static BOOL Load(StringView worldName);
    static void Unload();

    // Clears all entities while keeping world initialized
    static inline void Clear() {
        Unload();
        s_IsLoaded = true;
    }

    // ========================================================================
    // 3. Central Entity Spawning, O(1) Lookup & Destruction
    // ========================================================================
    static C_Entity &Spawn_Entity(INT id, EntityType type);
    static C_Entity &Spawn_Entity(EntityType type);

    // Type-safe templated spawn: auto-generates a unique ID and returns a typed pointer!
    // Example:
    //   C_Camera *cam      = M_World::Spawn_Entity<C_Camera>();
    //   C_DynamicMesh *dyn = M_World::Spawn_Entity<C_DynamicMesh>();
    //   C_StaticMesh *mesh = M_World::Spawn_Entity<C_StaticMesh>();
    template <typename TEntity = C_Entity>
    static inline TEntity *Spawn_Entity() {
        EntityType type = EntityType::StaticMesh;
        if constexpr (std::is_same_v<TEntity, C_Camera>) {
            type = EntityType::Camera;
        } else if constexpr (std::is_same_v<TEntity, C_Character>) {
            type = EntityType::Character;
        } else if constexpr (std::is_same_v<TEntity, C_DynamicMesh>) {
            type = EntityType::DynamicMesh;
        } else if constexpr (std::is_same_v<TEntity, C_StaticMesh>) {
            type = EntityType::StaticMesh;
        }
        return static_cast<TEntity*>(&Spawn_Entity(type));
    }

    // Convenience alias: M_World::Spawn<C_DynamicMesh>()
    template <typename TEntity = C_Entity>
    static inline TEntity *Spawn() {
        return Spawn_Entity<TEntity>();
    }

    static C_Entity &Spawn_Entity_Async(
        INT id,
        EntityType type,
        const FilePath &meshPath,
        const FilePath &texturePath = "",
        Function<void(C_Entity&)> onReady = nullptr);

    static C_Entity &Spawn_Entity_Async(
        EntityType type,
        const FilePath &meshPath,
        const FilePath &texturePath = "",
        Function<void(C_Entity&)> onReady = nullptr);

    [[nodiscard]] static C_Entity *Find_Entity(INT id);

    template<typename T>
    [[nodiscard]] static inline T *Find_Entity(INT id) {
        C_Entity *entity = Find_Entity(id);
        return entity ? entity->As<T>() : nullptr;
    }

    static BOOL Destroy_Entity(INT id);
    static void Get_EntitiesInRadius(const Vector3D &center, FLOAT radius, Array<C_Entity*> &outEntities);

    [[nodiscard]] static inline INT Get_EntityCount() {
        return Is_Loaded() ? static_cast<INT>(s_EntityByID.size()) : 0;
    }

    [[nodiscard]] static inline INT Get_DynamicEntityCount() {
        return Is_Loaded() ? static_cast<INT>(s_DynamicEntities.size()) : 0;
    }

    [[nodiscard]] static inline INT Get_VisibleEntityCount() {
        return s_VisibleCount;
    }

    [[nodiscard]] static inline INT Get_CulledEntityCount() {
        return s_CulledCount;
    }

    static inline C_Camera *Get_Camera() {
        return &C_Camera::Get_Active();
    }

    [[nodiscard]] static inline const Array<UniquePtr<C_Entity>> &Get_Entities() {
        return s_Entities;
    }

    // ========================================================================
    // 4. World Rendering
    // ========================================================================
    static void Draw_Entity(const C_Entity &entity);
    static void Render();
};

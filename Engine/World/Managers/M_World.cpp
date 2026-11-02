#include <M_World.h>
#include <M_Window.h>
#include <M_Collision.h>

Array<UniquePtr<C_Entity>>          M_World::s_Entities;
Array<C_DynamicMesh*>               M_World::s_DynamicEntities;
uMap<INT, C_Entity*>                M_World::s_EntityByID;
uMap<INT64, Array<C_Entity*>>       M_World::s_SpatialGrid;
uMap<INT, INT64>                    M_World::s_EntityCellKey;
Array<M_World::TransparentDrawItem> M_World::s_TransparentQueue;
String                              M_World::s_WorldName          = "None";
INT                                 M_World::s_NextEntityID       = 0;
INT                                 M_World::s_VisibleCount       = 0;
INT                                 M_World::s_CulledCount        = 0;
BOOL                                M_World::s_IsLoaded           = false;
BOOL                                M_World::s_IsUnloading        = false;
BOOL                                M_World::s_HasPendingDestroys = false;

INT64 M_World::Compute_CellKey(const Vector3D &pos) {
    INT cellX = static_cast<INT>(U_Math::Round(pos.x * INV_SPATIAL_CELL_SIZE, U_Math::Down));
    INT cellZ = static_cast<INT>(U_Math::Round(pos.z * INV_SPATIAL_CELL_SIZE, U_Math::Down));
    return Pack_CellKey(cellX, cellZ);
}

void M_World::Remove_FromSpatialCell(INT64 cellKey, C_Entity *entity) {
    auto it = s_SpatialGrid.find(cellKey);
    if (it != s_SpatialGrid.end()) {
        std::erase(it->second, entity);
        if (it->second.empty()) {
            s_SpatialGrid.erase(it);
        }
    }
}

void M_World::Sync_SpatialCell(C_Entity &entity) {
    const INT64 newKey = Compute_CellKey(entity.Position);
    auto it = s_EntityCellKey.find(entity.ID);
    if (it != s_EntityCellKey.end()) {
        if (it->second == newKey) return;
        Remove_FromSpatialCell(it->second, &entity);
        it->second = newKey;
    } else {
        s_EntityCellKey[entity.ID] = newKey;
    }
    s_SpatialGrid[newKey].push_back(&entity);
}

void M_World::BeginPlay(StringView defaultWorldName) {
    if (s_IsLoaded) return;

    s_WorldName          = String(defaultWorldName);
    s_NextEntityID       = 0;
    s_VisibleCount       = 0;
    s_CulledCount        = 0;
    s_IsLoaded           = true;
    s_IsUnloading        = false;
    s_HasPendingDestroys = false;

    s_Entities.reserve(256);
    s_DynamicEntities.reserve(64);
    s_EntityByID.reserve(256);
    s_SpatialGrid.reserve(64);
    s_EntityCellKey.reserve(256);
    s_TransparentQueue.reserve(64);

    LOG(Info, LogLevel::Core, "M_World::BeginPlay -> World [{}] is ready.", s_WorldName);
}

void M_World::Tick(FLOAT dt) {
    if (!s_IsLoaded || s_IsUnloading) return;

    U_Raycast::Tick_Debug(dt);

    const size_t dynCount = s_DynamicEntities.size();
    if (dynCount == 0) return;

    if (dynCount > 64) {
        M_Threads::Parallel_For(0, static_cast<UINT>(dynCount), [dt](UINT i) {
            C_DynamicMesh *dyn = s_DynamicEntities[i];
            if (!dyn || !dyn->bIsActive || dyn->bPendingDestroy) return;

            dyn->Tick(dt);
        });

        for (C_DynamicMesh *dyn : s_DynamicEntities) {
            if (dyn && dyn->bIsActive && !dyn->bPendingDestroy) {
                Sync_SpatialCell(*dyn);
            }
        }
    } else {
        for (C_DynamicMesh *dyn : s_DynamicEntities) {
            if (!dyn || !dyn->bIsActive || dyn->bPendingDestroy) continue;

            dyn->Tick(dt);
            Sync_SpatialCell(*dyn);
        }
    }

    if (s_HasPendingDestroys) {
        std::erase_if(s_DynamicEntities, [](const C_DynamicMesh *dyn) {
            return !dyn || dyn->bPendingDestroy;
        });
        std::erase_if(s_Entities, [](const UniquePtr<C_Entity> &ptr) {
            return !ptr || ptr->bPendingDestroy;
        });
        s_HasPendingDestroys = false;
    }
}

void M_World::Shutdown() {
    Unload();
    LOG(Info, LogLevel::Core, "M_World::Shutdown completed.");
}

BOOL M_World::Load(StringView worldName) {
    if (worldName.empty()) {
        LOG(Warn, LogLevel::Core, "M_World::Load called with empty worldName!");
        return false;
    }

    if (s_IsLoaded) {
        Unload();
    }

    s_WorldName          = String(worldName);
    s_NextEntityID       = 0;
    s_VisibleCount       = 0;
    s_CulledCount        = 0;
    s_IsLoaded           = true;
    s_IsUnloading        = false;
    s_HasPendingDestroys = false;

    s_Entities.reserve(256);
    s_DynamicEntities.reserve(64);
    s_EntityByID.reserve(256);
    s_SpatialGrid.reserve(64);
    s_EntityCellKey.reserve(256);
    s_TransparentQueue.reserve(64);

    LOG(Info, LogLevel::Core, "M_World::Load -> Loaded World [{}]", s_WorldName);
    return true;
}

void M_World::Unload() {
    if (!s_IsLoaded && s_Entities.empty()) return;

    s_IsUnloading = true;
    s_TransparentQueue.clear();
    s_SpatialGrid.clear();
    s_EntityCellKey.clear();
    s_DynamicEntities.clear();
    s_EntityByID.clear();
    s_Entities.clear();

    s_NextEntityID       = 0;
    s_VisibleCount       = 0;
    s_CulledCount        = 0;
    s_IsLoaded           = false;
    s_IsUnloading        = false;
    s_HasPendingDestroys = false;
    s_WorldName          = "None";
}

static inline BOOL IsMeshEntity(EntityType type) {
    return type == EntityType::StaticMesh ||
           type == EntityType::DynamicMesh ||
           type == EntityType::Character ||
           type == EntityType::Monster ||
           type == EntityType::NPC ||
           type == EntityType::Projectile;
}

C_Entity &M_World::Spawn_Entity(INT id, EntityType type) {
    if (!s_IsLoaded) {
        LOG(Warn, LogLevel::Core, "M_World::Spawn_Entity called before World loaded! Auto-starting DefaultWorld.");
        BeginPlay("DefaultWorld");
    }

    if (Find_Entity(id) != nullptr) {
        INT fallbackID = ++s_NextEntityID;
        LOG(Warn, LogLevel::Core, "M_World::Spawn_Entity -> Unique ID [{}] already exists! Assigned [{}] instead.", id, fallbackID);
        id = fallbackID;
    } else if (id > s_NextEntityID) {
        s_NextEntityID = id;
    }

    UniquePtr<C_Entity> ptr;
    switch (type) {
        case EntityType::Camera:
            ptr = MakeUnique<C_Camera>();
            break;
        case EntityType::StaticMesh:
            ptr = MakeUnique<C_StaticMesh>();
            break;
        case EntityType::Character:
            ptr = MakeUnique<C_Character>();
            break;
        case EntityType::DynamicMesh:
        case EntityType::Monster:
        case EntityType::NPC:
        case EntityType::Projectile:
            ptr = MakeUnique<C_DynamicMesh>();
            break;
        default:
            ptr = MakeUnique<C_Entity>();
            break;
    }

    C_Entity &entity       = *ptr;
    entity.ID              = id;
    entity.Type            = type;
    entity.bIsActive       = true;
    entity.bPendingDestroy = false;

    switch (type) {
        case EntityType::Camera:
        case EntityType::Trigger:
            entity.bIsVisible = false;
            break;
        default:
            entity.bIsVisible = true;
            break;
    }

    s_EntityByID[id] = &entity;
    if (type != EntityType::Camera) {
        Sync_SpatialCell(entity);
    }

    switch (type) {
        case EntityType::DynamicMesh:
        case EntityType::Character:
        case EntityType::Monster:
        case EntityType::NPC:
        case EntityType::Projectile:
            s_DynamicEntities.push_back(static_cast<C_DynamicMesh*>(&entity));
            break;
        default:
            break;
    }

    s_Entities.push_back(std::move(ptr));
    return entity;
}

C_Entity &M_World::Spawn_Entity(EntityType type) {
    return Spawn_Entity(++s_NextEntityID, type);
}

C_Entity &M_World::Spawn_Entity_Async(
    INT id,
    EntityType type,
    const FilePath &meshPath,
    const FilePath &texturePath,
    Function<void(C_Entity&)> onReady)
{
    C_Entity &entity = Spawn_Entity(id, type);
    entity.bIsVisible = false;

    struct StreamState {
        MeshData Mesh;
        UINT     TextureID = 0;
        BOOL     bMeshDone = false;
        BOOL     bTexDone  = false;
    };

    auto state = std::make_shared<StreamState>();
    state->bMeshDone = meshPath.empty();
    state->bTexDone  = texturePath.empty();

    auto CheckDone = [id, state, onReady]() {
        if (!state->bMeshDone || !state->bTexDone) return;
        C_Entity *ent = Find_Entity(id);
        if (!ent) return;

        if (IsMeshEntity(ent->Type)) {
            auto &meshEnt = static_cast<C_StaticMesh&>(*ent);
            if (state->Mesh.Is_Valid()) {
                meshEnt.Mesh = std::move(state->Mesh);
            }
            if (state->TextureID != 0) {
                meshEnt.TextureID = state->TextureID;
            }
        }
        ent->bIsVisible = true;

        if (onReady) {
            onReady(*ent);
        }
    };

    if (!meshPath.empty()) {
        M_Mesh::Stream_OBJ(meshPath, [state, CheckDone](MeshData mesh) {
            state->Mesh      = std::move(mesh);
            state->bMeshDone = true;
            CheckDone();
        });
    }

    if (!texturePath.empty()) {
        M_Texture::Stream(texturePath, [state, CheckDone](UINT texID) {
            state->TextureID = texID;
            state->bTexDone  = true;
            CheckDone();
        });
    }

    return entity;
}

C_Entity &M_World::Spawn_Entity_Async(
    EntityType type,
    const FilePath &meshPath,
    const FilePath &texturePath,
    Function<void(C_Entity&)> onReady)
{
    return Spawn_Entity_Async(++s_NextEntityID, type, meshPath, texturePath, std::move(onReady));
}

C_Entity *M_World::Find_Entity(INT id) {
    if (!Is_Loaded()) return nullptr;

    auto it = s_EntityByID.find(id);
    if (it == s_EntityByID.end()) return nullptr;

    C_Entity *entity = it->second;
    if (!entity || entity->bPendingDestroy) return nullptr;
    return entity;
}

BOOL M_World::Destroy_Entity(INT id) {
    C_Entity *entity = Find_Entity(id);
    if (!entity) return false;

    auto cellIt = s_EntityCellKey.find(id);
    if (cellIt != s_EntityCellKey.end()) {
        Remove_FromSpatialCell(cellIt->second, entity);
        s_EntityCellKey.erase(cellIt);
    }

    entity->bPendingDestroy = true;
    entity->bIsActive       = false;
    entity->bIsVisible      = false;
    s_EntityByID.erase(id);
    s_HasPendingDestroys    = true;
    return true;
}

void M_World::Get_EntitiesInRadius(const Vector3D &center, FLOAT radius, Array<C_Entity*> &outEntities) {
    outEntities.clear();
    if (!Is_Loaded() || radius <= 0.0f) return;

    const INT minCellX = static_cast<INT>(U_Math::Round((center.x - radius) * INV_SPATIAL_CELL_SIZE, U_Math::Down));
    const INT maxCellX = static_cast<INT>(U_Math::Round((center.x + radius) * INV_SPATIAL_CELL_SIZE, U_Math::Down));
    const INT minCellZ = static_cast<INT>(U_Math::Round((center.z - radius) * INV_SPATIAL_CELL_SIZE, U_Math::Down));
    const INT maxCellZ = static_cast<INT>(U_Math::Round((center.z + radius) * INV_SPATIAL_CELL_SIZE, U_Math::Down));

    for (INT cz = minCellZ; cz <= maxCellZ; ++cz) {
        for (INT cx = minCellX; cx <= maxCellX; ++cx) {
            auto it = s_SpatialGrid.find(Pack_CellKey(cx, cz));
            if (it == s_SpatialGrid.end()) continue;

            for (C_Entity *entity : it->second) {
                if (!entity || !entity->bIsActive || entity->bPendingDestroy) continue;
                if (U_Math::Extended::Get_Distance(entity->Position, center, U_Math::Extended::_3D) <= radius) {
                    outEntities.push_back(entity);
                }
            }
        }
    }
}

void M_World::Draw_Entity(const C_Entity &entity) {
    if (IsMeshEntity(entity.Type)) {
        const auto &meshEnt = static_cast<const C_StaticMesh&>(entity);
        COLOR renderColor = meshEnt.Color;
        if (meshEnt.Material.DiffuseTint.r != 255 || meshEnt.Material.DiffuseTint.g != 255 || meshEnt.Material.DiffuseTint.b != 255) {
            renderColor = M_Material::Evaluate_SurfaceColor(meshEnt.Material, COLOR_WHITE, meshEnt.Color);
        }
        RHI_Manager::Draw_Mesh(meshEnt.Mesh, meshEnt.Position, meshEnt.Rotation, meshEnt.Size, renderColor, meshEnt.Transparency, meshEnt.TextureID);
    }
}

void M_World::Render() {
    if (!Is_Loaded()) return;

    C_Camera &activeCam = C_Camera::Get_Active();
    activeCam.Update_CameraView();

    const C_Camera &cam      = activeCam;
    const Vector3D  camPos   = cam.Position;
    const Vector3D  camRight = M_Mesh::Get_CameraRight();
    const Vector3D  camUp    = M_Mesh::Get_CameraUp();
    const Vector3D  camFwd   = M_Mesh::Get_CameraForward();
    const FLOAT     nearClip = cam.NearClip;
    const FLOAT     farClip  = cam.FarClip;

    static FLOAT s_LastAspect = 0.0f;
    static FLOAT s_LastFocal  = 0.0f;
    static FLOAT s_CachedTanY = 0.57735f;
    static FLOAT s_CachedTanX = 0.57735f * (16.0f / 9.0f);
    static FLOAT s_CachedSecY = 1.1547f;
    static FLOAT s_CachedSecX = 1.436f;

    const Vector2D px     = M_Window::Get_PixelSize();
    const FLOAT    aspect = (px.y > 0.0f) ? (px.x / px.y) : (16.0f / 9.0f);
    const FLOAT    focal  = M_Mesh::Get_FocalLength();

    if (aspect != s_LastAspect || focal != s_LastFocal) {
        s_LastAspect = aspect;
        s_LastFocal  = focal;
        s_CachedTanY = (focal > 0.001f) ? (1.0f / focal) : 0.57735f;
        s_CachedTanX = s_CachedTanY * aspect;
        s_CachedSecY = U_Math::SquareRoot(1.0f + s_CachedTanY * s_CachedTanY);
        s_CachedSecX = U_Math::SquareRoot(1.0f + s_CachedTanX * s_CachedTanX);
    }

    const FLOAT tanHalfFovY = s_CachedTanY;
    const FLOAT tanHalfFovX = s_CachedTanX;
    const FLOAT secHalfFovY = s_CachedSecY;
    const FLOAT secHalfFovX = s_CachedSecX;

    if (s_TransparentQueue.capacity() < 256) s_TransparentQueue.reserve(256);
    s_TransparentQueue.clear();
    static std::vector<const C_StaticMesh*> s_OpaqueQueue;
    if (s_OpaqueQueue.capacity() < 256) s_OpaqueQueue.reserve(256);
    s_OpaqueQueue.clear();
    s_VisibleCount = 0;
    s_CulledCount  = 0;

    const size_t totalEntities = s_Entities.size();

    if (totalEntities > 128) {
        static Array<BYTE>  s_VisFlags;
        static Array<FLOAT> s_DistSqArray;
        if (s_VisFlags.size() < totalEntities) {
            s_VisFlags.resize(totalEntities);
            s_DistSqArray.resize(totalEntities);
        }

        std::atomic<INT> parallelVis{ 0 };
        std::atomic<INT> parallelCulled{ 0 };

        M_Threads::Parallel_For(0, static_cast<UINT>(totalEntities), [&](UINT i) {
            const auto &ptr = s_Entities[i];
            if (!ptr || !ptr->bIsActive || !ptr->bIsVisible || ptr->bPendingDestroy) {
                s_VisFlags[i] = 0;
                return;
            }
            if (!IsMeshEntity(ptr->Type)) {
                s_VisFlags[i] = 0;
                return;
            }

            const auto &meshEnt = static_cast<const C_StaticMesh&>(*ptr);
            if (meshEnt.Transparency <= 0.001f || !meshEnt.Mesh.Is_Valid()) {
                s_VisFlags[i] = 0;
                return;
            }

            const FLOAT dx = meshEnt.Position.x - camPos.x;
            const FLOAT dy = meshEnt.Position.y - camPos.y;
            const FLOAT dz = meshEnt.Position.z - camPos.z;

            const FLOAT maxScale    = U_Math::Clamp_Min(U_Math::Clamp_Min(meshEnt.Size.x, meshEnt.Size.y), meshEnt.Size.z);
            const FLOAT boundRadius = maxScale * (meshEnt.Mesh.BoundingRadius > 0.1f ? meshEnt.Mesh.BoundingRadius : 1.0f);

            const FLOAT viewZ = dx * camFwd.x + dy * camFwd.y + dz * camFwd.z;
            if (viewZ + boundRadius < nearClip || viewZ - boundRadius > farClip) {
                s_VisFlags[i] = 0;
                ++parallelCulled;
                return;
            }

            const FLOAT viewX = dx * camRight.x + dy * camRight.y + dz * camRight.z;
            if (U_Math::Absolute(viewX) > viewZ * tanHalfFovX + boundRadius * secHalfFovX) {
                s_VisFlags[i] = 0;
                ++parallelCulled;
                return;
            }

            const FLOAT viewY = dx * camUp.x + dy * camUp.y + dz * camUp.z;
            if (U_Math::Absolute(viewY) > viewZ * tanHalfFovY + boundRadius * secHalfFovY) {
                s_VisFlags[i] = 0;
                ++parallelCulled;
                return;
            }

            s_VisFlags[i]    = (meshEnt.Transparency >= 0.999f) ? 1 : 2;
            s_DistSqArray[i] = dx * dx + dy * dy + dz * dz;
            ++parallelVis;
        });

        s_VisibleCount = parallelVis.load();
        s_CulledCount  = parallelCulled.load();

        for (size_t i = 0; i < totalEntities; ++i) {
            if (s_VisFlags[i] == 1) {
                s_OpaqueQueue.push_back(static_cast<const C_StaticMesh*>(s_Entities[i].get()));
            } else if (s_VisFlags[i] == 2) {
                s_TransparentQueue.push_back({ static_cast<const C_StaticMesh*>(s_Entities[i].get()), s_DistSqArray[i] });
            }
        }
    } else {
        for (const auto &ptr : s_Entities) {
            if (!ptr) continue;
            if (!ptr->bIsActive || !ptr->bIsVisible || ptr->bPendingDestroy) continue;
            if (!IsMeshEntity(ptr->Type)) continue;

            const auto &meshEnt = static_cast<const C_StaticMesh&>(*ptr);
            if (meshEnt.Transparency <= 0.001f || !meshEnt.Mesh.Is_Valid()) continue;

            const FLOAT dx = meshEnt.Position.x - camPos.x;
            const FLOAT dy = meshEnt.Position.y - camPos.y;
            const FLOAT dz = meshEnt.Position.z - camPos.z;

            const FLOAT maxScale    = U_Math::Clamp_Min(U_Math::Clamp_Min(meshEnt.Size.x, meshEnt.Size.y), meshEnt.Size.z);
            const FLOAT boundRadius = maxScale * (meshEnt.Mesh.BoundingRadius > 0.1f ? meshEnt.Mesh.BoundingRadius : 1.0f);

            const FLOAT viewZ = dx * camFwd.x + dy * camFwd.y + dz * camFwd.z;
            if (viewZ + boundRadius < nearClip || viewZ - boundRadius > farClip) {
                ++s_CulledCount;
                continue;
            }

            const FLOAT viewX = dx * camRight.x + dy * camRight.y + dz * camRight.z;
            if (U_Math::Absolute(viewX) > viewZ * tanHalfFovX + boundRadius * secHalfFovX) {
                ++s_CulledCount;
                continue;
            }

            const FLOAT viewY = dx * camUp.x + dy * camUp.y + dz * camUp.z;
            if (U_Math::Absolute(viewY) > viewZ * tanHalfFovY + boundRadius * secHalfFovY) {
                ++s_CulledCount;
                continue;
            }

            ++s_VisibleCount;

            if (meshEnt.Transparency >= 0.999f) {
                s_OpaqueQueue.push_back(&meshEnt);
            } else {
                const FLOAT distSq = dx * dx + dy * dy + dz * dz;
                s_TransparentQueue.push_back({ &meshEnt, distSq });
            }
        }
    }

    // 1. Render opaque meshes batched by MeshID & TextureID (minimizes GPU cache & state churn)
    if (!s_OpaqueQueue.empty()) {
        if (s_OpaqueQueue.size() > 1) {
            std::sort(s_OpaqueQueue.begin(), s_OpaqueQueue.end(),
                [](const C_StaticMesh *a, const C_StaticMesh *b) {
                    if (a->Mesh.MeshID != b->Mesh.MeshID) return a->Mesh.MeshID < b->Mesh.MeshID;
                    return a->TextureID < b->TextureID;
                });
        }

        for (const C_StaticMesh *ent : s_OpaqueQueue) {
            Draw_Entity(*ent);
        }
    }

    if (!s_TransparentQueue.empty()) {
        if (s_TransparentQueue.size() > 1) {
            std::sort(s_TransparentQueue.begin(), s_TransparentQueue.end(),
                [](const TransparentDrawItem &a, const TransparentDrawItem &b) {
                    return a.DistSq > b.DistSq;
                });
        }

        for (const TransparentDrawItem &item : s_TransparentQueue) {
            Draw_Entity(*item.Entity);
        }
    }

    // If F1 Collision Debug is active, draw wireframe AABB bounding boxes of all solid world entities!
    if (M_Collision::Is_DebugDrawing()) {
        for (const auto &entPtr : s_Entities) {
            if (!entPtr || !entPtr->bIsActive || entPtr->bPendingDestroy) continue;
            if (entPtr->Has_Collision()) {
                U_Raycast::Draw_DebugBox(entPtr->Get_WorldAABB(), COLOR_GREEN, 0.0f);
            }
        }
    }

    // Render 3D visual debug rays and bounding boxes
    U_Raycast::Render_Debug();
}

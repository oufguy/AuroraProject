#pragma once

// ============================================================================
// Aurora Engine — World Module (3D World, Entities, Camera, Simulation)
// ============================================================================
// Include this header for 3D game projects:
//   #include <World.h>
//
// Automatically includes Core.h, plus:
//   - M_World: Spawning, O(1) ID lookup, spatial grid, 6-plane frustum culling
//   - C_Entity (Mother of All: ID, Transform, Position, Rotation)
//   - C_StaticMesh (Static 3D Mesh & Texture in World)
//   - C_DynamicMesh (Moving 3D Mesh with Velocity & Physics)
//   - C_Camera (3D Viewport & Projection)
// ============================================================================

#include <Core.h>
#include <C_Entity.h>
#include <C_StaticMesh.h>
#include <C_DynamicMesh.h>
#include <C_Character.h>
#include <C_Camera.h>
#include <U_Raycast.h>
#include <M_Collision.h>
#include <M_Physics.h>
#include <M_Lighting.h>
#include <M_Material.h>
#include <M_World.h>

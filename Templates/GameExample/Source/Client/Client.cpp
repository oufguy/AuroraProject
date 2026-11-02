#include <Client.h>
#include <GameChannels.h>
#include <M_SceneSerializer.h>
#include <M_File.h>

void Client::BeginPlay() {
	// 0. Setup World Lighting & Sunlight (Warm daylight sun + town lantern point light)
	M_Lighting::Init();
	M_Lighting::Set_SunAngles(50.0f, 35.0f);
	M_Lighting::Set_SunColor(COLOR(255, 248, 225, 255));
	M_Lighting::Set_AmbientColor(COLOR(85, 95, 115, 255));
	M_Lighting::Set_AmbientIntensity(0.45f);
	M_Lighting::Add_PointLight(Vector3D(0.0f, 3.5f, 25.0f), 18.0f, COLOR(255, 195, 110, 255), 1.25f);

	// 1. Mount any compiled .pak archives in Packs/ (virtual filesystem)
	if (M_File::Exists("Packs/GameData.pak")) M_File::Mount_Pak("Packs/GameData.pak");
	if (M_File::Exists("Packs/Audio.pak"))    M_File::Mount_Pak("Packs/Audio.pak");

	// 2. Load 3D World Scene dynamically from Scenes/World.scene (created/edited in Aurora Studio!)
	FilePath scenePath = "Scenes/World.scene";
	if (M_File::Exists(scenePath)) {
		M_SceneSerializer::Load(scenePath);

		for (const auto &ent : M_World::Get_Entities()) {
			if (!ent) continue;
			if (ent->Type == EntityType::Character && !m_Player) {
				m_Player = ent->As<C_Character>();
			} else if (ent->Type == EntityType::StaticMesh) {
				auto *sm = ent->As<C_StaticMesh>();
				if (sm && (sm->Position.x != 0.0f || sm->Position.z != 0.0f)) {
					m_TownBuildings.push_back(sm);
				} else if (sm && !m_Ground) {
					m_Ground = sm;
				}
			}
		}
	} else {
		// Fallback initial scene setup
		m_Ground = M_World::Spawn<C_StaticMesh>();
		m_Ground->Mesh     = M_Asset::Load_Mesh("Assets/Models/Town_Ground.obj");
		m_Ground->Color    = COLOR(55, 115, 60, 255);
		m_Ground->Position = { 0.0f, 0.0f, 0.0f };
		m_Ground->Rotation = { 0.0f, 0.0f, 180.0f };
		m_Ground->Material = M_Material::Create_Matte(COLOR_WHITE);

		m_Player = M_World::Spawn<C_Character>();
		m_Player->Mesh     = M_Asset::Load_Mesh("Assets/Models/Character.obj");
		m_Player->Color    = COLOR(52, 152, 219, 255);
		m_Player->Position = { 0.0f, 0.0f, 0.0f };
		m_Player->GroundY  = 0.0f;
		m_Player->Material = M_Material::Create_Metallic(COLOR_WHITE, 48.0f);

		MeshData buildingMesh = M_Asset::Load_Mesh("Assets/Models/Building.obj");
		static const struct { Vector3D pos; Vector3D rot; COLOR tint; } townProps[] = {
			{ {  18.0f, 0.0f,  14.0f }, { 0.0f, -30.0f, 0.0f }, COLOR(180, 140, 100, 255) },
			{ { -18.0f, 0.0f,  14.0f }, { 0.0f,  30.0f, 0.0f }, COLOR(160, 130,  95, 255) },
			{ {   0.0f, 0.0f,  32.0f }, { 0.0f,   0.0f, 0.0f }, COLOR(200, 155, 110, 255) },
			{ { -22.0f, 0.0f, -16.0f }, { 0.0f,  45.0f, 0.0f }, COLOR(170, 135, 100, 255) },
			{ {  22.0f, 0.0f, -16.0f }, { 0.0f, -45.0f, 0.0f }, COLOR(190, 150, 105, 255) }
		};
		for (const auto &prop : townProps) {
			auto *b = M_World::Spawn<C_StaticMesh>();
			b->Mesh     = buildingMesh;
			b->Position = prop.pos;
			b->Rotation = prop.rot;
			b->Color    = prop.tint;
			b->Material = M_Material::Create_Matte(COLOR_WHITE);
			b->Set_CollisionProfile(CollisionProfile::BlockAll);
			m_TownBuildings.push_back(b);
		}
	}

	// 4. Setup 3rd-Person Camera (WoW / MMO Style Chase Camera)
	m_Camera = M_World::Get_Camera();
	m_Camera->Rotation = { 20.0f, 0.0f, 0.0f }; // Look 20 degrees down at character
	m_Camera->Follow(m_Player->Get_CameraTarget(), m_CameraDistance);

	// 5. Setup 2D UI Demo Elements (Ace Online Style 4-State UI + Manual Render)
	m_HUDPanel = UI::Create_Box(20.0f, 20.0f, 260.0f, 120.0f, COLOR(25, 30, 40, 220));
	m_HUDPanel.Transparency = 0.90f;

	m_TestButton.SetPosition(35.0f, 40.0f);
	m_TestButton.SetSize(230.0f, 45.0f);
	m_TestButton.SetTextures("Assets/UI/Btn_Normal.bmp", "Assets/UI/Btn_Hover.bmp", "Assets/UI/Btn_Pressed.bmp");
	m_TestButton.SetTint(COLOR(52, 152, 219, 255));
	m_TestButton.Transparency = 1.0f;

	// 6. Play a sound in 3D world on a random town building every 1 second (looping interval)
	M_Time::Set_Interval(1.0f, [this]() {
		if (m_TownBuildings.empty()) return;
		INT index = std::rand() % m_TownBuildings.size();
		if (index >= 0 && index < static_cast<INT>(m_TownBuildings.size())) {
			C_Entity *b = m_TownBuildings[index];
			if (b) {
				M_Sound::Play("Assets/Audio/Chime.wav", /*bIsLocal=*/false, b->Position, 45.0f, 1.0f);
			}
		}
	});
}

void Client::Tick(FLOAT deltaTime) {
	// 0. Toggle Visual Debug & Performance Diagnostics
	if (I_Keyboard::Is_Pressed(Key::F1)) {
		M_Collision::Toggle_DebugDrawing();
		PRINT("[Debug] Collision View (F1): {}", M_Collision::Is_DebugDrawing() ? "ENABLED [ON]" : "DISABLED [OFF]");
	}
	if (I_Keyboard::Is_Pressed(Key::F2)) {
		UI::Toggle_FPS();
		PRINT("[Debug] Performance FPS HUD (F2): {}", UI_FPSCounter::Is_Visible() ? "ENABLED [ON]" : "DISABLED [OFF]");
	}

	// 1. Ground Player Character Controls (WASD Walking + Space Jump + Turning)
	if (m_Player) {
		// WASD
		if (I_Keyboard::Is_Down(Key::W)) m_Player->Move_Forward(1.0f, deltaTime);
		if (I_Keyboard::Is_Down(Key::S)) m_Player->Move_Forward(-1.0f, deltaTime);
		if (I_Keyboard::Is_Down(Key::D)) m_Player->Turn(m_Player->TurnSpeed * deltaTime);
		if (I_Keyboard::Is_Down(Key::A)) m_Player->Turn(-m_Player->TurnSpeed * deltaTime);

		// Jump with Sound Effect (Local 2D user sound)
		if (I_Keyboard::Is_Pressed(Key::Space)) {
			m_Player->Jump();
			M_Sound::Play("Assets/Audio/Jump.wav", /*bIsLocal=*/true, 0.9f);
		}
	}

	// 2. 3rd-Person Chase Camera Controls (Right Click Drag to Orbit, Mouse Wheel to Zoom)
	if (m_Camera && m_Player) {
		BOOL bRightHeld = I_Mouse::Is_Down(MouseButton::MB_Right);
		M_Window::Set_MouseLocked(bRightHeld);
		if (bRightHeld) {
			Vector2D delta = I_Mouse::Get_Delta();
			m_Camera->Add_Rotation(delta.y * 0.15f, delta.x * 0.15f);

			// Holding Right Click turns character to face camera look direction (Classic WoW MMO style!)
			m_Player->Rotation.y = m_Camera->Rotation.y;
		}

		// Mouse Wheel Zoom
		FLOAT scroll = I_Mouse::Get_Scroll();
		if (std::abs(scroll) > 0.01f) {
			m_CameraDistance = U_Math::Clamp_MinMax(m_CameraDistance - scroll * 0.75f, 2.0f, 25.0f);
		}

		// Keep camera locked behind character
		m_Camera->Follow(m_Player->Get_CameraTarget(), m_CameraDistance);
	}

	// 3. 3D Mouse Ray Picking & Ground Raycast (Left Click with Unreal Trace_Line!)
	if (I_Mouse::Is_Pressed(MouseButton::MB_Left)) {
		Vector2D mousePos = I_Mouse::Get_Position();
		if (!m_HUDPanel.Contains(mousePos)) {
			if (m_Camera) {
				Ray3D ray = U_Raycast::Screen_To_World(mousePos, m_Camera->Position, m_Camera->Rotation, m_Camera->FOV);
				RayHit hit;
				// Cast ray with visual 3D debug drawing (stays visible for 1.5 seconds!)
				if (Ray_Trace(ray.Origin, ray.Direction, 500.0f, hit, TraceChannel::Visibility, nullptr, /*bDebugDraw=*/true, COLOR_CYAN, 1.5f)) {
					m_SelectedEntity = hit.Entity;
					M_Sound::Play("Assets/Audio/Click.wav", /*bIsLocal=*/false, hit.Point, 45.0f, 1.0f);
					// Spawn 3D Floating Combat Damage Numbers on hit!
					INT damage = 145 + (std::rand() % 130);
					BOOL bCrit = (std::rand() % 4 == 0);
					if (bCrit) damage = static_cast<INT>(damage * 1.8f);
					UI::Spawn_Damage(hit.Point, damage, bCrit);
					PRINT("Ray_Trace hit Entity ID: {} - Dealt {} damage! at ({:.1f}, {:.1f}, {:.1f})", hit.EntityID, damage, hit.Point.x, hit.Point.y, hit.Point.z);
				} else {
					Vector3D groundHit;
					if (Ground_Trace(ray.Origin, ray.Direction, groundHit, 0.0f)) {
						m_LastGroundClick = groundHit;
						m_HasGroundClick  = true;
						m_SelectedEntity  = nullptr;
						if (m_Player) {
							// Draw visible debug line from player to ground target!
							U_Raycast::Draw_DebugLine(m_Player->Position, groundHit, COLOR_GOLD, 1.0f);
						}
					}
				}
			}
		}
	}

	// 4. UI Button Click interaction with Sound Effect
	if (m_TestButton.Is_Clicked()) {
		PRINT("UI Button Clicked! Player Jump!");
		if (m_Player) {
			m_Player->Jump();
		}
		M_Sound::Play("Assets/Audio/Click.wav", /*bIsLocal=*/true, 1.0f);
	}

	// 5. Update 3D Floating Combat Text Physics & Lifetimes
	UI::Tick_FloatingText(deltaTime);
}

void Client::DrawUI() {
	// 0. Render 3D Floating Combat Damage Numbers in World Space
	if (m_Camera) {
		UI::Render_FloatingText(*m_Camera);
	}

	// 1. 3D World-to-Screen Floating Nameplates & Bars (Rendered underneath 2D HUD)
	if (m_Player && m_Camera) {
		Vector2D screenPos;
		if (m_Camera->World_To_Screen(m_Player->Position + Vector3D(0.0f, 2.1f, 0.0f), screenPos)) {
			// Overhead character title & nameplate (centered dynamically)
			const StringView charName = "Li Wei [Lv. 1]";
			Vector2D nameSize = UI::Measure_Text(charName, 0.95f);
			UI::Draw_Text_Shadowed(charName, screenPos.x - (nameSize.x * 0.5f), screenPos.y - 20.0f, COLOR_CYAN, COLOR_BLACK, 0.95f);
			// 3D Floating Health Bar (WoW / MMO Style)
			UI::Draw_Box(screenPos.x - 30.0f, screenPos.y - 4.0f, 60.0f, 6.0f, COLOR(20, 20, 20, 220));
			UI::Draw_Box(screenPos.x - 29.0f, screenPos.y - 3.0f, 58.0f * 0.85f, 4.0f, COLOR(46, 204, 113, 255)); // 85% HP Green
		}
	}

	if (m_Camera) {
		for (size_t i = 0; i < m_TownBuildings.size(); ++i) {
			auto *b = m_TownBuildings[i];
			if (!b) continue;
			Vector2D bScreenPos;
			if (m_Camera->World_To_Screen(b->Position + Vector3D(0.0f, 6.8f, 0.0f), bScreenPos)) {
				String label = std::format("Town Building #{}", i + 1);
				Vector2D labelSize = UI::Measure_Text(label, 0.85f);
				COLOR textColor = (m_SelectedEntity == b) ? COLOR_CYAN : COLOR_GOLD;
				UI::Draw_Text_Shadowed(label, bScreenPos.x - (labelSize.x * 0.5f), bScreenPos.y, textColor, COLOR_BLACK, 0.85f);
			}
		}
	}

	// 2. Selected Target Info Window (Classic MMORPG Target Frame)
	if (m_SelectedEntity) {
		UI::Draw_Box(450.0f, 20.0f, 240.0f, 50.0f, COLOR(25, 30, 40, 220));
		String targetName = (m_SelectedEntity == m_Player) ? "Target: Li Wei (Player)" : std::format("Target: Building #{}", m_SelectedEntity->ID);
		UI::Draw_Text_Shadowed(targetName, 465.0f, 28.0f, COLOR_GOLD, COLOR_BLACK, 0.95f);
		UI::Draw_Text_Shadowed("Solid Obstacle [AABB]", 465.0f, 48.0f, COLOR(180, 220, 255, 255), COLOR_BLACK, 0.75f);
	}

	// 3. 2D Screen-space HUD & Buttons
	m_HUDPanel.Render();
	m_TestButton.Render();

	// Button label
	UI::Draw_Text_Shadowed("Jump [Space]", 80.0f, 52.0f, COLOR_WHITE, COLOR_BLACK, 0.85f);

	// In-Game MMORPG HUD Overlay (stb_truetype font rendering)
	UI::Draw_Text_Shadowed("Aurora MMORPG Client Demo", 20.0f, 155.0f, COLOR_GOLD, COLOR_BLACK, 0.95f);
	UI::Draw_Text_Shadowed("WASD: Move/Turn | Space: Jump | Left-Click: Attack/Pick Target | Right-Drag: Cam | Wheel: Zoom", 20.0f, 185.0f, COLOR_WHITE, COLOR_BLACK, 0.75f);

	if (m_Player) {
		String stats = std::format("Pos: ({:.1f}, {:.1f}, {:.1f}) | Grounded: {}",
			m_Player->Position.x, m_Player->Position.y, m_Player->Position.z,
			m_Player->bIsGrounded ? "YES" : "AIR");
		UI::Draw_Text_Shadowed(stats, 20.0f, 210.0f, COLOR_CYAN, COLOR_BLACK, 0.75f);
	}

	// 4. On-Screen Real-Time Performance & Diagnostics HUD (Toggle with F2)
	UI::Draw_FPS(20.0f, 245.0f);
}

void Client::EndPlay() {
	PRINT("Client game session closed gracefully.");
}

AURORA_GAME(Client)

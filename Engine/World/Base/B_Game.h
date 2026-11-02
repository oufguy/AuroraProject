#pragma once

#include <Core.h>
#include <M_Config.h>
#include <World.h>

// ============================================================================
// B_Game (Aurora Game Instance Base Class)
// ============================================================================
// The single unified base class for all Aurora games.
// Automatically manages the window, RHI, configuration settings, 3D world,
// camera, and main loop.
//
// Usage:
//   class MyGame : public B_Game {
//   public:
//       void BeginPlay() override { ... }
//       void Tick(FLOAT dt) override { ... }
//       void EndPlay() override { ... }
//   };
//   AURORA_GAME(MyGame)
// ============================================================================
class B_Game {
public:
    virtual ~B_Game() = default;

    // Called once when the Engine and 3D World are initialized
    virtual void BeginPlay();

    // Called once per frame for game logic and entity updates
    virtual void Tick(FLOAT deltaTime);

    // Called once per frame inside active RHI frame after 3D world is rendered.
    // Perfect for drawing 2D UI elements, HUD, overlays, and debug text!
    virtual void DrawUI();

    // Called once when the game is shutting down
    virtual void EndPlay();

    // Configuration file path (can be overridden)
    virtual FilePath Get_SettingsPath() const;

    // Default configuration content created on first run
    virtual String Get_DefaultSettings() const;

    // In-memory configuration store
    M_Config Settings;

    // Internal game runner (implemented in B_Game.cpp)
    static INT RunGameInternal(B_Game &game);

    // ------------------------------------------------------------------------
    // Engine Launch Runner for B_Game
    // ------------------------------------------------------------------------
    template <typename TGame>
    static INT RunGame() {
        static_assert(std::is_base_of_v<B_Game, TGame>, "Class must inherit from B_Game!");
        TGame game;
        return RunGameInternal(game);
    }
};

// Main Game Entry Macro: Hides int main() and runs B_Game
#define AURORA_GAME(GameClass) \
    int main(int argc, char** argv) { \
        (void)argc; (void)argv; \
        return B_Game::RunGame<GameClass>(); \
    }

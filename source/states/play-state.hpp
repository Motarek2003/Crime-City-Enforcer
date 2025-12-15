#pragma once

#include <application.hpp>

#include <ecs/world.hpp>
#include <systems/forward-renderer.hpp>
#include <systems/free-camera-controller.hpp>
#include <systems/movement.hpp>
#include <asset-loader.hpp>
#include <systems/character-controller.hpp>

#include <systems/physics-system.hpp>
#include <systems/lifetime-system.hpp>

#include <iostream>
#include <systems/inventory-controller.hpp>
#include <systems/animation-system.hpp>
#include <systems/bone-attachment-system.hpp>

#include <systems/enemy-controller.hpp>
#include <systems/boss-controller.hpp>
#include <systems/game-manager.hpp>

#include <texture/texture-utils.hpp>


// This state shows how to use the ECS framework and deserialization.
class Playstate: public our::State {

    our::World world;
    our::ForwardRenderer renderer;
    our::FreeCameraControllerSystem cameraController;
    our::MovementSystem movementSystem;
    our::CharacterControllerSystem characterController;

    our::PhysicsSystem physicsSystem;

    our::InventoryControllerSystem inventoryController;
    our::AnimationSystem animationSystem;
    our::BoneAttachmentSystem boneAttachmentSystem;

    our::EnemyControllerSystem enemyController;
    //our::BossControllerSystem bossController;
    our::GameManager gameManager;

    // HUD Elements
    our::Texture2D* crosshairTexture = nullptr;


    void onInitialize() override {
        // First of all, we get the scene configuration from the app config
        auto& config = getApp()->getConfig()["scene"];
        // If we have assets in the scene config, we deserialize them
        if(config.contains("assets")){
            our::deserializeAllAssets(config["assets"]);
        }
        // If we have a world in the scene config, we use it to populate our world
        if(config.contains("world")){
            world.deserialize(config["world"]);
        }

        // 1. Disable everything first
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_FALSE);

        // 2. Re-enable only Errors and Warnings (High and Medium severity)
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_HIGH, 0, nullptr, GL_TRUE);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, nullptr, GL_TRUE);

        // Optional: Enable Low severity (sometimes useful, sometimes spammy)
        // glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, nullptr, GL_TRUE);

        physicsSystem.initialize();
        
        physicsSystem.setDebugDrawEnabled(true);
        // We initialize the camera controller system since it needs a pointer to the app
        cameraController.enter(getApp());
        characterController.enter(getApp());
        inventoryController.enter(getApp());
        enemyController.enter(getApp());
        //bossController.enter(getApp());
        gameManager.enter(getApp(), &enemyController);
        // Then we initialize the renderer
        auto size = getApp()->getFrameBufferSize();
        renderer.initialize(size, config["renderer"]);
        
        // Load HUD textures
        crosshairTexture = our::texture_utils::loadImage("assets/textures/Crosshair.png", false);
        
        std::cout << "init done" << std::endl;
    }

    void onImmediateGui() override {
        // Get display size for HUD positioning
        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        
        // === CROSSHAIR ===
        if (crosshairTexture) {
            float crosshairSize = 64.0f;
            ImVec2 crosshairPos(
                displaySize.x * 0.5f - crosshairSize * 0.5f,
                displaySize.y * 0.5f - crosshairSize * 0.5f
            );
            
            ImGui::SetNextWindowPos(crosshairPos);
            ImGui::SetNextWindowSize(ImVec2(crosshairSize, crosshairSize));
            ImGui::Begin("Crosshair", nullptr, 
                ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs);
            
            ImGui::Image((ImTextureID)(intptr_t)crosshairTexture->getOpenGLName(), 
                         ImVec2(crosshairSize, crosshairSize));
            ImGui::End();
        }
        
        // === Find Player and Boss entities ===
        our::Entity* playerEntity = nullptr;
        our::Entity* bossEntity = nullptr;
        our::CharacterComponent* playerCharacter = nullptr;
        our::CharacterComponent* bossCharacter = nullptr;
        our::InventoryComponent* inventory = nullptr;
        
        for(auto entity : world.getEntities()){
            if (entity->name == "deadpool") {
                playerEntity = entity;
                playerCharacter = entity->getComponent<our::CharacterComponent>();
                inventory = entity->getComponent<our::InventoryComponent>();
            } else if (entity->name == "taskmaster") {
                bossEntity = entity;
                bossCharacter = entity->getComponent<our::CharacterComponent>();
            }
        }
        
        // === PLAYER HUD (Bottom Left) ===
        {
            ImGui::SetNextWindowPos(ImVec2(20, displaySize.y - 150), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(250, 130), ImGuiCond_Always);
            
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | 
                                      ImGuiWindowFlags_NoResize | 
                                      ImGuiWindowFlags_NoMove |
                                      ImGuiWindowFlags_NoScrollbar;
            
            ImGui::Begin("PlayerHUD", nullptr, flags);
            
            // Player Health Bar
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "HEALTH");
            int playerHealth = playerCharacter ? playerCharacter->getHealth() : 0;
            float healthPercent = playerHealth / 100.0f;
            
            // Color based on health
            ImVec4 healthColor;
            if (healthPercent > 0.6f) healthColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);       // Green
            else if (healthPercent > 0.3f) healthColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);  // Yellow
            else healthColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);                             // Red
            
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, healthColor);
            ImGui::ProgressBar(healthPercent, ImVec2(230, 20), "");
            ImGui::PopStyleColor();
            
            // Health text overlay
            char healthText[32];
            snprintf(healthText, sizeof(healthText), "%d / 100", playerHealth);
            ImGui::SameLine(0, -230);
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 90);
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", healthText);
            
            ImGui::Spacing();
            
            // Ammo Display
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "AMMO");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "INF");  // Infinite ammo for now
            
            ImGui::End();
        }
        
        // === BOSS HUD (Top Center - only during boss fight) ===
        if (gameManager.getPhase() == our::GamePhase::BOSS_FIGHT && bossCharacter && bossCharacter->getAlive()) {
            ImGui::SetNextWindowPos(ImVec2(displaySize.x * 0.5f, 60), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
            ImGui::SetNextWindowSize(ImVec2(400, 100), ImGuiCond_Always);
            
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | 
                                      ImGuiWindowFlags_NoResize | 
                                      ImGuiWindowFlags_NoMove |
                                      ImGuiWindowFlags_NoScrollbar;
            
            ImGui::Begin("BossHUD", nullptr, flags);
            
            // Boss Name and Phase
            int bossPhase = enemyController.getBossPhase();
            char bossTitle[64];
            snprintf(bossTitle, sizeof(bossTitle), "TASKMASTER - PHASE %d", bossPhase);
            
            float titleWidth = ImGui::CalcTextSize(bossTitle).x;
            ImGui::SetCursorPosX((400 - titleWidth) * 0.5f);
            
            // Phase colors
            ImVec4 phaseColor;
            switch (bossPhase) {
                case 1: phaseColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f); break;  // Yellow
                case 2: phaseColor = ImVec4(1.0f, 0.5f, 0.0f, 1.0f); break;  // Orange
                case 3: phaseColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f); break;  // Red
                default: phaseColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); break;
            }
            ImGui::TextColored(phaseColor, "%s", bossTitle);
            
            // Boss blocking indicator
            if (enemyController.isBossBlocking()) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.0f, 0.7f, 1.0f, 1.0f), " [BLOCKING]");
            }
            
            // Boss Health Bar
            int bossHealth = bossCharacter->getHealth();
            float bossHealthPercent = bossHealth / 100.0f;
            
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, phaseColor);
            ImGui::SetCursorPosX(10);
            ImGui::ProgressBar(bossHealthPercent, ImVec2(380, 25), "");
            ImGui::PopStyleColor();
            
            // Health text
            char bossHealthText[32];
            snprintf(bossHealthText, sizeof(bossHealthText), "%d / 100", bossHealth);
            float textWidth = ImGui::CalcTextSize(bossHealthText).x;
            ImGui::SetCursorPosX((400 - textWidth) * 0.5f);
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "%s", bossHealthText);
            
            ImGui::End();
        }
        
        // === INVENTORY (Hide during combat for cleaner HUD) ===
        if (inventory && gameManager.getPhase() != our::GamePhase::BOSS_FIGHT) {
            ImGui::Begin("Inventory");
            for(int i = 0; i < inventory->slots.size(); ++i){
                std::string label = "Slot " + std::to_string(i + 1);
                if(i == inventory->activeSlot){
                    label += " (Active)";
                    ImGui::TextColored(ImVec4(0, 1, 0, 1), "%s", label.c_str());
                } else {
                    ImGui::Text("%s", label.c_str());
                }
                
                // List items in slot
                if(!inventory->slots[i].empty()){
                    ImGui::SameLine();
                    ImGui::Text(": ");
                    for(size_t j = 0; j < inventory->slots[i].size(); ++j){
                        ImGui::SameLine();
                        ImGui::Text("%s", inventory->slots[i][j].c_str());
                        if(j < inventory->slots[i].size() - 1) {
                            ImGui::SameLine();
                            ImGui::Text(",");
                        }
                    }
                } else {
                     ImGui::SameLine();
                     ImGui::Text(": Empty");
                }
            }
            ImGui::End();
        }
        
        // Game Status UI
        gameManager.drawUI();
    }

    void onDraw(double deltaTime) override {
        if (physicsSystem.getDebugRenderer()) {
            physicsSystem.getDebugRenderer()->Clear();
        }
        // Here, we just run a bunch of systems to control the world logic
        movementSystem.update(&world, (float)deltaTime);
        cameraController.update(&world, (float)deltaTime);
        characterController.update(&world, (float)deltaTime, &physicsSystem);
        physicsSystem.update(&world, (float)deltaTime);
        our::LifetimeSystem::update(&world, (float)deltaTime, physicsSystem.getBodyInterface());
        inventoryController.update(&world, (float)deltaTime);
        animationSystem.update(&world, (float)deltaTime);
        boneAttachmentSystem.update(&world, (float)deltaTime);
        enemyController.update(&world, (float)deltaTime, &physicsSystem);
        //bossController.update(&world, (float)deltaTime, &physicsSystem);
        gameManager.update(&world, (float)deltaTime, getApp());
        // And finally we use the renderer system to draw the scene
        renderer.render(&world, &physicsSystem);

        // Get a reference to the keyboard object
        auto& keyboard = getApp()->getKeyboard();

        if(keyboard.justPressed(GLFW_KEY_F3)) {
            bool currentState = physicsSystem.isDebugDrawEnabled();
            physicsSystem.setDebugDrawEnabled(!currentState);
            std::cout << "Physics Debug Draw: " << (!currentState ? "ON" : "OFF") << std::endl;
        }

        world.deleteMarkedEntities();

        if(keyboard.justPressed(GLFW_KEY_ESCAPE)){
            // If the escape  key is pressed in this frame, go to the play state
            getApp()->changeState("menu");
        }

        if(keyboard.justPressed(GLFW_KEY_F4))
        {
            renderer.togglePostProcessing();
        }
    }

    void onDestroy() override {
        // Don't forget to destroy the renderer
        renderer.destroy();
        // On exit, we call exit for the camera controller system to make sure that the mouse is unlocked
        cameraController.exit();
        characterController.exit();
        // Reset the animation system for next play
        animationSystem.reset();
        physicsSystem.cleanup();

        enemyController.exit();
        //bossController.exit();
        gameManager.exit();
        
        // Clean up HUD textures
        if (crosshairTexture) {
            delete crosshairTexture;
            crosshairTexture = nullptr;
        }
        
        // Clear the world
        world.clear();
        // and we delete all the loaded assets to free memory on the RAM and the VRAM
        our::clearAllAssets();
    }
};
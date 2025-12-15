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
        std::cout << "init done" << std::endl;
    }

    void onImmediateGui() override {
        // Find the player entity with inventory
        our::InventoryComponent* inventory = nullptr;
        for(auto entity : world.getEntities()){
            inventory = entity->getComponent<our::InventoryComponent>();
            if(inventory) break;
        }

        if(!inventory) return;

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
        // Clear the world
        world.clear();
        // and we delete all the loaded assets to free memory on the RAM and the VRAM
        our::clearAllAssets();
    }
};
#pragma once

#include <application.hpp>

#include <ecs/world.hpp>
#include <systems/forward-renderer.hpp>
#include <systems/free-camera-controller.hpp>
#include <systems/movement.hpp>
#include <asset-loader.hpp>
#include <systems/character-controller.hpp>
#include <systems/inventory-controller.hpp>

// This state shows how to use the ECS framework and deserialization.
class Playstate: public our::State {

    our::World world;
    our::ForwardRenderer renderer;
    our::FreeCameraControllerSystem cameraController;
    our::MovementSystem movementSystem;
    our::CharacterControllerSystem characterController;
    our::InventoryControllerSystem inventoryController;

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
        // We initialize the camera controller system since it needs a pointer to the app
        cameraController.enter(getApp());
        characterController.enter(getApp());
        inventoryController.enter(getApp());
        // Then we initialize the renderer
        auto size = getApp()->getFrameBufferSize();
        renderer.initialize(size, config["renderer"]);
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
    }

    void onDraw(double deltaTime) override {
        // Here, we just run a bunch of systems to control the world logic
        movementSystem.update(&world, (float)deltaTime);
        cameraController.update(&world, (float)deltaTime);
        characterController.update(&world, (float)deltaTime);
        inventoryController.update(&world, (float)deltaTime);
        // And finally we use the renderer system to draw the scene
        renderer.render(&world);

        // Get a reference to the keyboard object
        auto& keyboard = getApp()->getKeyboard();

        if(keyboard.justPressed(GLFW_KEY_ESCAPE)){
            // If the escape  key is pressed in this frame, go to the play state
            getApp()->changeState("menu");
        }
    }

    void onDestroy() override {
        // Don't forget to destroy the renderer
        renderer.destroy();
        // On exit, we call exit for the camera controller system to make sure that the mouse is unlocked
        cameraController.exit();
        characterController.exit();
        // Clear the world
        world.clear();
        // and we delete all the loaded assets to free memory on the RAM and the VRAM
        our::clearAllAssets();
    }
};
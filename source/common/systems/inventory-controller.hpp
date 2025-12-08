#pragma once

#include "../ecs/world.hpp"
#include "../components/inventory.hpp"
#include "../components/mesh-renderer.hpp"
#include "../application.hpp"

namespace our {

    class InventoryControllerSystem {
        Application* app;

    public:
        void enter(Application* app){
            this->app = app;
        }

        void update(World* world, float deltaTime) {
            for(auto entity : world->getEntities()){
                auto inventory = entity->getComponent<InventoryComponent>();
                if(!inventory) continue;

                int prevSlot = inventory->activeSlot;

                // Keyboard Input
                if(app->getKeyboard().justPressed(GLFW_KEY_1)) inventory->activeSlot = 0;
                if(app->getKeyboard().justPressed(GLFW_KEY_2)) inventory->activeSlot = 1;
                if(app->getKeyboard().justPressed(GLFW_KEY_3)) inventory->activeSlot = 2;
                if(app->getKeyboard().justPressed(GLFW_KEY_4)) inventory->activeSlot = 3;
                if(app->getKeyboard().justPressed(GLFW_KEY_5)) inventory->activeSlot = 4;

                // Mouse Wheel Input
                float scroll = app->getMouse().getScrollOffset().y;
                if(scroll > 0) {
                    inventory->activeSlot = (inventory->activeSlot + 1) % 5; // Cycle up
                } else if(scroll < 0) {
                    inventory->activeSlot = (inventory->activeSlot - 1 + 5) % 5; // Cycle down
                }
                
                // Clamp slot
                if(inventory->activeSlot >= 5) inventory->activeSlot = 4;
                if(inventory->activeSlot < 0) inventory->activeSlot = 0;

                // Update visibility
                // First, disable ALL weapons in ALL slots to ensure no conflicts
                for(const auto& slot : inventory->slots){
                    for(const auto& weaponName : slot){
                        for(auto e : world->getEntities()){
                            if(e->name == weaponName){
                                auto meshRenderer = e->getComponent<MeshRendererComponent>();
                                if(meshRenderer) meshRenderer->enabled = false;
                                break;
                            }
                        }
                    }
                }

                // Then, enable ONLY the weapons in the active slot
                if(inventory->activeSlot >= 0 && inventory->activeSlot < inventory->slots.size()){
                    for(const auto& weaponName : inventory->slots[inventory->activeSlot]){
                        for(auto e : world->getEntities()){
                            if(e->name == weaponName){
                                auto meshRenderer = e->getComponent<MeshRendererComponent>();
                                if(meshRenderer) meshRenderer->enabled = true;
                                break;
                            }
                        }
                    }
                }
            }
        }
        
        void exit(){}
    };
}

#pragma once

#include <iostream>
#include <fstream>
#include <flags/flags.h>
#include <json/json.hpp>

#include "../components/character.hpp"
#include "../components/free-camera-controller.hpp"
#include "../components/animator.hpp"
#include "../components/inventory.hpp"
#include "../ecs/world.hpp"
#include "../components/rigidbody.hpp"
#include "../components/camera.hpp"
#include "./physics-system.hpp"
#include "./object-spawner.hpp"

#include "../application.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
#include <glm/gtx/fast_trigonometry.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <Jolt/Physics/Body/BodyInterface.h>

namespace our
{

    // The free character controller system is responsible for moving every entity which contains a CharacterComponent.
    // This system is added as a slightly complex example for how use the ECS framework to implement logic. 
    // For more information, see "common/components/character-controller.hpp"
    class NPCControllerSystem {
        Application* app; // The application in which the state runs

    public:
        // When a state enters, it should call this function and give it the pointer to the application
        void enter(Application* app){

        }

        // This should be called every frame to update all entities containing a CharacterComponent 
        void update(World* world, float deltaTime, our::PhysicsSystem* physicsSystem) {
            JPH::BodyInterface* bodyInterface = physicsSystem->getBodyInterface();
            CharacterComponent* character = nullptr;
            for(auto entity : world->getEntities()){
                character = entity->getComponent<CharacterComponent>();
                if(character && entity->name == "enemy")
                {
                    glm::mat4 worldTransform = entity->getLocalToWorldMatrix();
                    glm::vec3 startPos = glm::vec3(worldTransform[3]);
                    startPos.y += 1.5f; 

                    glm::vec3 direction = glm::vec3(worldTransform * glm::vec4(0, 0, 1, 0));

                    LayerFilter myFilter({Layers::ENEMY});
                    physicsSystem->Raycast(startPos, direction, 200, myFilter);
                }
             }

        }

        static void onCollision(Entity* self, Entity* other) {
            if (!other) return;
            
            // Only take damage from player bullets (projectiles have timeRemaining > 0)
            // timeRemaining > 0 means it's an active projectile, not expired
            if (other->timeRemaining <= 0) {
                return; // Not a projectile or already expired
            }
            
            // Take damage from the projectile
            CharacterComponent* character = self->getComponent<CharacterComponent>();
            if (!character) return;
            
            character->setHealth(-10);
            std::cout << "Enemy Health: " << character->getHealth() << std::endl;
            
            // Mark the bullet as used so it doesn't hit multiple times
            other->timeRemaining = -1; // This will cause the bullet to be destroyed
            
            if (character->getHealth() <= 0) {
                self->timeRemaining = -1; // Destroy the enemy
            }
        }
            
        // When the state exits, it should call this function to ensure the mouse is unlocked
        void exit(){}

    };

}

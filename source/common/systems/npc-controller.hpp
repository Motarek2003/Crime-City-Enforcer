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
        nlohmann::json object_data;
    public:
        // When a state enters, it should call this function and give it the pointer to the application
        void enter(Application* app){
            this->app = app;
            std::string config_path = ("config/bullet.jsonc");

            // Open the config file and exit if failed
            std::ifstream file_in(config_path);
            if(!file_in)
                std::cerr << "Couldn't open file: " << config_path << std::endl;
            else
                object_data = nlohmann::json::parse(file_in, nullptr, true, true);
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

                    glm::vec3 direction = glm::normalize(glm::vec3(worldTransform * glm::vec4(0, 0, 1, 0)));

                    LayerFilter myFilter({Layers::ENEMY, Layers::ENEMY_ATTACK, Layers::PLAYER_ATTACK});
                    RaycastHit hit;
                    hit.hasHit = false;
                    for(int i = -9; i <= 9; i++)
                    {
                        float angle = glm::radians(i * 10.0f); 

                        // 3. Rotate the vector around the Y axis (Up)
                        glm::vec3 currentDir = glm::rotate(direction, angle, glm::vec3(0, 1, 0));

                        hit = physicsSystem->Raycast(startPos, currentDir, 200, myFilter);

                        if(hit.hasHit) break;
                    }

                    States state = character->getState();
                    // if(state == States::PATROL)
                    //     std::cout << "patrol" <<std::endl;
                    // else if(state == States::PURSUIT)
                    //     std::cout << "pursuit" <<std::endl;
                    // else
                    //     std::cout << "investigation" <<std::endl;
                    

                    if(character->getState() == States::PATROL)
                    {
                        glm::vec3 target = character->getTarget();
                        //std::cout << target.x << target.y << target.z << std::endl;
                        //std::cout << character->getIndex() << std::endl;
                        float distance = glm::length(glm::abs(startPos - target));
                        //std::cout<< distance <<std::endl;
                        if(distance < 0.9f)
                        {
                            character->updateTarget();
                        }

                    }

                    if(hit.hasHit && hit.entity->layer == "player")
                    {
                        character->setState(States::PURSUIT);
                        //std::cout << hit.position.x << hit.position.y << hit.position.z << std::endl;
                        character->updateTarget(hit.position);
                        float timer = character->getTimer();
                        timer -= deltaTime;
                        //std::cout<<timer<<std::endl;
                        if (timer <= 0)
                        {
                            glm::vec3 shot_dir = glm::normalize(hit.position - startPos);
                            JPH::Vec3 forwardImpulse = JPH::Vec3(shot_dir.x, shot_dir.y, shot_dir.z) * 100.0f;
                            ObjectSpawner::spawnObject(
                                world,
                                nullptr,
                                object_data,
                                startPos,
                                glm::vec3(0.0f),
                                glm::vec3(0.1f),
                                forwardImpulse,
                                10.0f,
                                "enemy_attack"
                            );
                            character->setTimer(0, true);
                        } else
                            character->setTimer(timer, false);
                    } else if(character->getState() == States::INVESTIGATION){
                        glm::vec3 target = character->getTarget();
                        float distance = glm::length(glm::abs(startPos - target));
                        //std::cout<< distance <<std::endl;
                        if(distance < 0.5f)
                        {
                            character->updateTarget();
                            character->setState(States::PATROL);
                        }
   
                    } else if(character->getState() == States::PURSUIT) {
                        character->setState(States::INVESTIGATION);
                    }

                    glm::vec3 target = character->getTarget();
                    //std::cout << target.x << target.y << target.z << std::endl;

                    auto rb = entity->getComponent<RigidBodyComponent>();

                    JPH::Vec3 currentVel = bodyInterface->GetLinearVelocity(rb->runtimeBodyID);
                    glm::vec3 new_Direction = glm::normalize(glm::vec3(target - startPos));

                    float speed = 2.0f;
            
                    JPH::Vec3 newVel(
                        new_Direction.x * speed, 
                        currentVel.GetY(), // gravity
                        new_Direction.z * speed
                    );

                    bodyInterface->SetLinearVelocity(rb->runtimeBodyID, newVel);

                    glm::vec3& rotation = entity->localTransform.rotation;

                    if(glm::length(new_Direction) > 0) 
                    {
                        new_Direction = glm::normalize(new_Direction);
                        float targetAngle = glm::atan(new_Direction.x, new_Direction.z);

                        rotation.y = targetAngle; 
                    }


                }
             }

        }

        static void onCollision(Entity* self, Entity* other) {
            if(other->layer == "player_attack")
            {
                CharacterComponent* character = self->getComponent<CharacterComponent>();
                character->setHealth(-10);
                std::cout << "Enemy Health: " << character->getHealth() << std::endl; 
                if(character->getHealth() == 0)
                    self->timeRemaining = -1;
            }
        }
            
        // When the state exits, it should call this function to ensure the mouse is unlocked
        void exit(){}

    };

}

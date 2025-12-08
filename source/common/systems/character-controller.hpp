#pragma once

#include <iostream>
#include <fstream>
#include <flags/flags.h>
#include <json/json.hpp>

#include "../components/character.hpp"
#include "../components/free-camera-controller.hpp"
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
    class CharacterControllerSystem {
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

            //std::cout << object_data.dump(4) << std::endl;
        }

        // This should be called every frame to update all entities containing a CharacterComponent 
        void update(World* world, float deltaTime, our::PhysicsSystem* physicsSystem) {
            JPH::BodyInterface* bodyInterface = physicsSystem->getBodyInterface();
            CharacterComponent* character = nullptr;
            CameraComponent* camera = nullptr;
            for(auto entity : world->getEntities()){
                if(!character)
                character = entity->getComponent<CharacterComponent>();
                if(!camera)
                camera = entity->getComponent<CameraComponent>();
                if(camera && character) break;
             }
            Entity* entity = character->getOwner();
            // We get a reference to the entity's position and rotation
            glm::vec3& position = entity->localTransform.position;
            glm::vec3& rotation = entity->localTransform.rotation;
            Entity* cameraEntity = camera->getOwner();

            // We get the character model matrix (relative to its parent) to compute the front, up and right directions
            Transform  characterTransform = entity->localTransform;
            characterTransform.rotation.y = cameraEntity->localTransform.rotation.y;
            glm::mat4 matrix = characterTransform.toMat4();

            glm::vec3 front = glm::vec3(matrix * glm::vec4(0, 0, -1, 0)),
                      up = glm::vec3(matrix * glm::vec4(0, 1, 0, 0)), 
                      right = glm::vec3(matrix * glm::vec4(1, 0, 0, 0));

            glm::vec3 new_Direction = glm::vec3(0.0f, 0.0f, 0.0f);
            // We change the character position based on the keys WASD/QE
            // S & W moves the player back and forth
            if(app->getKeyboard().isPressed(GLFW_KEY_W))
            {
                new_Direction += front;
            } 
            if(app->getKeyboard().isPressed(GLFW_KEY_S))
            {
                new_Direction -= front;
            } 
            // Q & E moves the player up and down
            if(app->getKeyboard().isPressed(GLFW_KEY_Q))
            {
                new_Direction += up;
            } 
            if(app->getKeyboard().isPressed(GLFW_KEY_E))
            {
                new_Direction -= up;
            } 
            if(app->getKeyboard().isPressed(GLFW_KEY_D))
            {
                new_Direction += right;
            } 
            if(app->getKeyboard().isPressed(GLFW_KEY_A))
            {
                new_Direction -= right;
            } 

            auto rb = entity->getComponent<RigidBodyComponent>();

            JPH::Vec3 currentVel = bodyInterface->GetLinearVelocity(rb->runtimeBodyID);

            float speed = 20.0f;

            if(app->getKeyboard().isPressed(GLFW_KEY_LEFT_SHIFT)) speed *= 5;
    
            JPH::Vec3 newVel(
                new_Direction.x * speed, 
                currentVel.GetY(), // gravity
                new_Direction.z * speed
            );

            bodyInterface->SetLinearVelocity(rb->runtimeBodyID, newVel);


            if(glm::length(new_Direction) > 0) 
            {
                new_Direction = glm::normalize(new_Direction);
                float targetAngle = glm::atan(new_Direction.x, new_Direction.z);

                rotation.y = targetAngle; 
            }
            
            LayerFilter myFilter({Layers::MOVING});
            RaycastHit hit =  physicsSystem->Raycast(entity->localTransform.position, -1.0f * up, 10, myFilter);

            if(app->getKeyboard().justPressed(GLFW_KEY_SPACE) && hit.hasHit) {
                JPH::Vec3 jumpImpulse = JPH::Vec3(0, 5.0f, 0);
                bodyInterface->AddImpulse(rb->runtimeBodyID, jumpImpulse);
            }

            if(app->getMouse().justPressed(GLFW_MOUSE_BUTTON_RIGHT)) {

                //std::cout << "Spawning Object!" << std::endl;

                JPH::Vec3 forwardImpulse = JPH::Vec3(front.x, front.y, front.z) * 1000000.0f;
                ObjectSpawner::spawnObject(
                    world,
                    entity,
                    object_data,
                    position + front * 2.0f + glm::vec3(0,1.0f,0),
                    glm::vec3(0.0f),
                    glm::vec3(0.1f),
                    forwardImpulse,
                    10.0f
                );
            }

        }

        static void onCollision(Entity* self, Entity* other) {
            // You can access the CharacterComponent like this:
            CharacterComponent* character = self->getComponent<CharacterComponent>();
            character->setHealth(-10);
            std::cout << "Character Health: " << character->getHealth() << std::endl;
        }

        // When the state exits, it should call this function to ensure the mouse is unlocked
        void exit(){}

    };

}

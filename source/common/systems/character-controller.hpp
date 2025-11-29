#pragma once

#include "../components/character.hpp"
#include "../components/free-camera-controller.hpp"
#include "../ecs/world.hpp"

#include "../application.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
#include <glm/gtx/fast_trigonometry.hpp>
#include <glm/gtx/vector_angle.hpp>

namespace our
{

    // The free character controller system is responsible for moving every entity which contains a CharacterComponent.
    // This system is added as a slightly complex example for how use the ECS framework to implement logic. 
    // For more information, see "common/components/character-controller.hpp"
    class CharacterControllerSystem {
        Application* app; // The application in which the state runs

    public:
        // When a state enters, it should call this function and give it the pointer to the application
        void enter(Application* app){
            this->app = app;
        }

        // This should be called every frame to update all entities containing a CharacterComponent 
        void update(World* world, float deltaTime) {
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

            glm::vec3 front = glm::vec3(matrix * glm::vec4(0, 0, 5, 0)),
                      up = glm::vec3(matrix * glm::vec4(0, -5, 0, 0)), 
                      right = glm::vec3(matrix * glm::vec4(-5, 0, 0, 0));

            glm::vec3 current_sensitivity = {-30.0f, -30.0f, -30.0f};
            // If the LEFT SHIFT key is pressed, we multiply the position sensitivity by the speed up factor
            if(app->getKeyboard().isPressed(GLFW_KEY_LEFT_SHIFT)) current_sensitivity *= 5;

            glm::vec3 new_Direction = glm::vec3(0.0f, 0.0f, 0.0f);
            // We change the character position based on the keys WASD/QE
            // S & W moves the player back and forth
            if(app->getKeyboard().isPressed(GLFW_KEY_W))
            {
                position += front * (deltaTime * current_sensitivity.z);
                new_Direction += front;
            } 
            if(app->getKeyboard().isPressed(GLFW_KEY_S))
            {
                position -= front * (deltaTime * current_sensitivity.z);
                new_Direction -= front;
            } 
            // Q & E moves the player up and down
            if(app->getKeyboard().isPressed(GLFW_KEY_Q))
            {
                position += up * (deltaTime * current_sensitivity.y);
                new_Direction += up;
            } 
            if(app->getKeyboard().isPressed(GLFW_KEY_E))
            {
                position -= up * (deltaTime * current_sensitivity.y);
                new_Direction -= up;
            } 
            // A & D moves the player left or right 
            if(app->getKeyboard().isPressed(GLFW_KEY_D))
            {
                position += right * (deltaTime * current_sensitivity.x);
                new_Direction += right;
            } 
            if(app->getKeyboard().isPressed(GLFW_KEY_A))
            {
                position -= right * (deltaTime * current_sensitivity.x);
                new_Direction -= right;
            } 


            if(glm::length(new_Direction) > 0) 
            {
                new_Direction = glm::normalize(new_Direction);
                float targetAngle = glm::atan(-new_Direction.x, -new_Direction.z);

                rotation.y = targetAngle; 
            }

        }

        // When the state exits, it should call this function to ensure the mouse is unlocked
        void exit(){}

    };

}

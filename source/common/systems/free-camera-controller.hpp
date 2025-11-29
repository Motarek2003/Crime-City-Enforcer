#pragma once

#include "../ecs/world.hpp"
#include "../components/camera.hpp"
#include "../components/free-camera-controller.hpp"
#include "../components/character.hpp"

#include "../application.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
#include <glm/gtx/fast_trigonometry.hpp>

namespace our
{

    // The free camera controller system is responsible for moving every entity which contains a FreeCameraControllerComponent.
    // This system is added as a slightly complex example for how use the ECS framework to implement logic. 
    // For more information, see "common/components/free-camera-controller.hpp"
    class FreeCameraControllerSystem {
        Application* app; // The application in which the state runs
        bool mouse_locked = false; // Is the mouse locked

    public:
        // When a state enters, it should call this function and give it the pointer to the application
        void enter(Application* app){
            this->app = app;
        }

        // This should be called every frame to update all entities containing a FreeCameraControllerComponent 
        void update(World* world, float deltaTime) {
            // First of all, we search for an entity containing both a CameraComponent and a FreeCameraControllerComponent
            // As soon as we find one, we break
            CameraComponent* camera = nullptr;
            FreeCameraControllerComponent *controller = nullptr;
            for(auto entity : world->getEntities()){
                camera = entity->getComponent<CameraComponent>();
                controller = entity->getComponent<FreeCameraControllerComponent>();
                if(camera && controller) break;
            }
            // If there is no entity with both a CameraComponent and a FreeCameraControllerComponent, we can do nothing so we return
            if(!(camera && controller)) return;
            // Get the entity that we found via getOwner of camera (we could use controller->getOwner())
            Entity* entity = camera->getOwner();

            CharacterComponent* character = nullptr;
            for(auto entity : world->getEntities()){
                 character = entity->getComponent<CharacterComponent>();
                 if(character) break;
             }
            Entity* characterEntity = character->getOwner();

            // If the left mouse button is pressed, we lock and hide the mouse. This common in First Person Games.
            if(app->getMouse().isPressed(GLFW_MOUSE_BUTTON_1) && !mouse_locked){
                app->getMouse().lockMouse(app->getWindow());
                mouse_locked = true;
            // If the ESCAPE key is pressed, we unlock and unhide the mouse.
            } else if(app->getKeyboard().justPressed(GLFW_KEY_Y) && mouse_locked) {
                app->getMouse().unlockMouse(app->getWindow());
                mouse_locked = false;
            }

            // We get a reference to the entity's position and rotation
            glm::vec3& position = entity->localTransform.position;
            glm::vec3& rotation = entity->localTransform.rotation;

            // If the mouse is locked, we get the change in the mouse location
            // and use it to update the camera rotation
            if(mouse_locked){
                glm::vec2 delta = app->getMouse().getMouseDelta();
                rotation.x -= delta.y * controller->rotationSensitivity; // The y-axis controls the pitch
                rotation.y -= delta.x * controller->rotationSensitivity; // The x-axis controls the yaw
            }

            // We prevent the pitch from exceeding a certain angle from the XZ plane to prevent gimbal locks
            if(rotation.x < -glm::half_pi<float>() * 0.99f) rotation.x = -glm::half_pi<float>() * 0.99f;
            if(rotation.x >  glm::half_pi<float>() * 0.99f) rotation.x  = glm::half_pi<float>() * 0.99f;
            // This is not necessary, but whenever the rotation goes outside the 0 to 2*PI range, we wrap it back inside.
            // This could prevent floating point error if the player rotates in single direction for an extremely long time. 
            rotation.y = glm::wrapAngle(rotation.y);

            const float minAngle = -glm::pi<float>() / 3.0f ;
            const float maxAngle =  glm::pi<float>() / 3.0f ;

            // 2. Clamp the value
            rotation.x = glm::clamp(rotation.x, minAngle, maxAngle);

            int distance = 1;
            position.x = (distance * glm::sin(rotation.y) * glm::cos(rotation.x)) + characterEntity->localTransform.position.x;
            position.z = (distance * glm::cos(rotation.y) * glm::cos(rotation.x)) + characterEntity->localTransform.position.z;
            position.y = (distance * glm::sin(-rotation.x)) + characterEntity->localTransform.position.y + 2;

            // Aiming Logic
            if(app->getMouse().isPressed(GLFW_MOUSE_BUTTON_2)){
                // Smoothly interpolate to aim FOV
                camera->fovY = glm::mix(camera->fovY, controller->aimFov, deltaTime * 10.0f);
            } else {
                // Smoothly interpolate back to base FOV
                camera->fovY = glm::mix(camera->fovY, controller->baseFov, deltaTime * 10.0f);
            }
        }

        // When the state exits, it should call this function to ensure the mouse is unlocked
        void exit(){
            if(mouse_locked) {
                mouse_locked = false;
                app->getMouse().unlockMouse(app->getWindow());
            }
        }

    };

}

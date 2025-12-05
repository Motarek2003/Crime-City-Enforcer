#pragma once

#include "../components/character.hpp"
#include "../components/free-camera-controller.hpp"
#include "../components/animator.hpp"
#include "../components/inventory.hpp"
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
        bool wasWalking = false;  // Track previous walking state
        bool isPlayingAttackAnimation = false;  // Track if attack animation is playing

    public:
        // When a state enters, it should call this function and give it the pointer to the application
        void enter(Application* app){
            this->app = app;
            wasWalking = false;
            isPlayingAttackAnimation = false;
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

            glm::vec3 front = glm::vec3(matrix * glm::vec4(0, 0, 4, 0)),
                      up = glm::vec3(matrix * glm::vec4(0, -4, 0, 0)), 
                      right = glm::vec3(matrix * glm::vec4(-4, 0, 0, 0));

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

            if (app->getMouse().isPressed(GLFW_MOUSE_BUTTON_2)) {
                rotation.y = cameraEntity->localTransform.rotation.y + glm::pi<float>();
            }
            else if(glm::length(new_Direction) > 0) 
            {
                new_Direction = glm::normalize(new_Direction);
                float targetAngle = glm::atan(-new_Direction.x, -new_Direction.z);

                rotation.y = targetAngle; 
            }

            // Control walking animation based on movement
            bool isWalking = app->getKeyboard().isPressed(GLFW_KEY_W) || 
                             app->getKeyboard().isPressed(GLFW_KEY_S) ||
                             app->getKeyboard().isPressed(GLFW_KEY_A) ||
                             app->getKeyboard().isPressed(GLFW_KEY_D);
            
            // Get animator component and switch animations based on state
            auto animator = entity->getComponent<AnimatorComponent>();
            auto inventory = entity->getComponent<InventoryComponent>();
            bool isAttacking = app->getMouse().isPressed(GLFW_MOUSE_BUTTON_1);
            static bool leftAttackPressed = false;
            
            if (animator && animator->enabled) {
                // Check if attack animation has finished
                if (isPlayingAttackAnimation) {
                    if (animator->isAnimationFinished()) {
                        isPlayingAttackAnimation = false;
                        // Reset to allow state machine to pick next animation
                    }
                }
                
                // Don't interrupt attack animations until they finish
                if (isPlayingAttackAnimation && !animator->isAnimationFinished()) {
                    // Let attack animation continue playing
                }
                else if (isAttacking) {
                    // Switch to attack animation if available
                    if (inventory->slots[inventory->activeSlot].empty()) {
                        if (animator->hasAnimation("attack")) {
                            animator->setAnimation("attack");
                            animator->play();
                            isPlayingAttackAnimation = true;
                        }
                    } else if (inventory->slots[inventory->activeSlot][0] == "player_gun") {
                        if (animator->hasAnimation("GunAttack")) {
                            animator->setAnimation("GunAttack");
                            animator->play();
                            isPlayingAttackAnimation = true;
                        }
                    } else if (inventory->slots[inventory->activeSlot][0] == "player_katana") {
                        if (!leftAttackPressed && animator->hasAnimation("KatanaSlashL")) {
                            animator->setAnimation("KatanaSlashL");
                            animator->play();
                            isPlayingAttackAnimation = true;
                            leftAttackPressed = true;
                        }
                        else if (leftAttackPressed && animator->hasAnimation("KatanaSlashR")) {
                            animator->setAnimation("KatanaSlashR");
                            animator->play();
                            isPlayingAttackAnimation = true;
                            leftAttackPressed = false;
                        }
                    } else if (inventory->slots[inventory->activeSlot][0] == "player_rifle") {
                        if (animator->hasAnimation("RifleShoot")) {
                            animator->setAnimation("RifleShoot");
                            animator->play();
                            isPlayingAttackAnimation = true;
                        }
                    }
                }
                else if (isWalking) {
                    // Switch to walk animation if available
                    // if inventory active slots does not contain weapons use walk animation
                    if (inventory->slots[inventory->activeSlot].empty()) {  
                        if (app->getKeyboard().isPressed(GLFW_KEY_LEFT_SHIFT)) {
                            if (animator->hasAnimation("run") && animator->getCurrentAnimationName() != "run") {
                                animator->setAnimation("run");
                                animator->play();
                            }
                        }
                        else  {
                            if (animator->hasAnimation("walk") && animator->getCurrentAnimationName() != "walk") {
                            animator->setAnimation("walk");
                            animator->play();
                            }
                        } 
                    }
                    else if (inventory->slots[inventory->activeSlot][0] == "player_gun"){
                        if (app->getKeyboard().isPressed(GLFW_KEY_LEFT_SHIFT)) {
                            if (animator->hasAnimation("GunRun") && animator->getCurrentAnimationName() != "GunRun"){
                                animator->setAnimation("GunRun");
                                animator->play();
                            }
                        }
                        else {
                            if (animator->hasAnimation("GunWalk") && animator->getCurrentAnimationName() != "GunWalk"){
                                animator->setAnimation("GunWalk");
                                animator->play();
                            }
                        }
                    } else if (inventory->slots[inventory->activeSlot][0] == "player_katana"){
                        if(app->getKeyboard().isPressed(GLFW_KEY_LEFT_SHIFT)) {
                            if (animator->hasAnimation("KatanaRun") && animator->getCurrentAnimationName() != "KatanaRun"){
                                animator->setAnimation("KatanaRun");
                                animator->play();
                            }
                        }
                        else {
                            if (animator->hasAnimation("KatanaWalk") && animator->getCurrentAnimationName() != "KatanaWalk"){
                                animator->setAnimation("KatanaWalk");
                                animator->play();
                            }
                        }
                    }
                    else if (inventory->slots[inventory->activeSlot][0] == "player_rifle") {
                        if (app->getKeyboard().isPressed(GLFW_KEY_LEFT_SHIFT)) {
                            if (animator->hasAnimation("RifleRun") && animator->getCurrentAnimationName() != "RifleRun") {
                                animator->setAnimation("RifleRun");
                                animator->play();
                            }
                        }
                        else {
                            if (animator->hasAnimation("RifleWalk") && animator->getCurrentAnimationName() != "RifleWalk") {
                                animator->setAnimation("RifleWalk");
                                animator->play();
                            }
                        }
                    }
                } else {
                    // Switch to idle animation if available
                    if(inventory->slots[inventory->activeSlot].empty()) {
                        if (animator->hasAnimation("idle") && animator->getCurrentAnimationName() != "idle") {
                            animator->setAnimation("idle");
                            animator->play();
                        }                        
                    } else if (inventory->slots[inventory->activeSlot][0] == "player_gun") {
                        if (animator->hasAnimation("GunIdle") && animator->getCurrentAnimationName() != "GunIdle") {
                            animator->setAnimation("GunIdle");
                            animator->play();
                        }
                    } else if (inventory->slots[inventory->activeSlot][0] == "player_katana") {
                        if (animator->hasAnimation("KatanaIdle") && animator->getCurrentAnimationName() != "KatanaIdle") {
                            animator->setAnimation("KatanaIdle");
                            animator->play();
                        }
                    } else if (inventory->slots[inventory->activeSlot][0] == "player_rifle") {
                        if (animator->hasAnimation("RifleIdle") && animator->getCurrentAnimationName() != "RifleIdle") {
                            animator->setAnimation("RifleIdle");
                            animator->play();
                        }
                    }
                }
            }
            wasWalking = isWalking;
        }

        // When the state exits, it should call this function to ensure the mouse is unlocked
        void exit(){}

    };

}

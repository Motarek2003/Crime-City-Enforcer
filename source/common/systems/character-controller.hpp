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
    class CharacterControllerSystem {
        Application* app; // The application in which the state runs
        nlohmann::json object_data;
        bool wasWalking = false;  // Track previous walking state
        bool isPlayingAttackAnimation = false;  // Track if attack animation is playing
        static inline bool leftAttackPressed = false;  // Track katana attack alternation
        int healCounter = 0;  // Track number of heals used

        // Constants
        static constexpr float BASE_SPEED = 20.0f;
        static constexpr float SPRINT_MULTIPLIER = 5.0f;
        static constexpr float JUMP_FORCE = 5.0f;
        static constexpr float BULLET_SPEED = 100.0f;
        static constexpr int MAX_HEALS = 5;
        static constexpr int HEAL_AMOUNT = 20;

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
            wasWalking = false;
            isPlayingAttackAnimation = false;
        }
        // This should be called every frame to update all entities containing a CharacterComponent 
        void update(World* world, float deltaTime, our::PhysicsSystem* physicsSystem) {
            // Find player and camera
            auto [character, camera] = findPlayerAndCamera(world);
            if (!character || !camera) return;

            Entity* playerEntity = character->getOwner();
            Entity* cameraEntity = camera->getOwner();
            JPH::BodyInterface* bodyInterface = physicsSystem->getBodyInterface();

            if (character->getAlive()) {
                handleAlivePlayer(world, physicsSystem, bodyInterface, playerEntity, cameraEntity, character);
            } else {
                handleDeadPlayer(playerEntity);
            }
        }

    private:
        // ==================== Entity Finding ====================
        
        std::pair<CharacterComponent*, CameraComponent*> findPlayerAndCamera(World* world) {
            CharacterComponent* character = nullptr;
            CameraComponent* camera = nullptr;
            
            for (auto entity : world->getEntities()) {
                if (!character && entity->name != "enemy") {
                    character = entity->getComponent<CharacterComponent>();
                }
                if (!camera) {
                    camera = entity->getComponent<CameraComponent>();
                }
                if (character && camera) break;
            }
            return {character, camera};
        }

        Entity* findActiveWeapon(Entity* playerEntity, InventoryComponent* inventory) {
            if (!inventory || inventory->slots[inventory->activeSlot].empty()) return nullptr;
            
            const std::string& weaponName = inventory->slots[inventory->activeSlot][0];
            for (Entity* child : playerEntity->children) {
                if (!child->name.empty() && child->name == weaponName) {
                    return child;
                }
            }
            return nullptr;
        }

        std::string getActiveWeaponName(InventoryComponent* inventory) {
            if (!inventory || inventory->slots[inventory->activeSlot].empty()) return "";
            return inventory->slots[inventory->activeSlot][0];
        }

        // ==================== Movement ====================
        
        struct MovementVectors {
            glm::vec3 front, up, right;
        };

        MovementVectors calculateMovementVectors(Entity* playerEntity, Entity* cameraEntity) {
            Transform characterTransform = playerEntity->localTransform;
            characterTransform.rotation.y = cameraEntity->localTransform.rotation.y;
            glm::mat4 matrix = characterTransform.toMat4();

            return {
                glm::vec3(matrix * glm::vec4(0, 0, -1, 0)),  // front
                glm::vec3(matrix * glm::vec4(0, 1, 0, 0)),   // up
                glm::vec3(matrix * glm::vec4(1, 0, 0, 0))    // right
            };
        }

        glm::vec3 getInputDirection(const MovementVectors& mv) {
            glm::vec3 direction(0.0f);
            if (app->getKeyboard().isPressed(GLFW_KEY_W)) direction += mv.front;
            if (app->getKeyboard().isPressed(GLFW_KEY_S)) direction -= mv.front;
            if (app->getKeyboard().isPressed(GLFW_KEY_D)) direction += mv.right;
            if (app->getKeyboard().isPressed(GLFW_KEY_A)) direction -= mv.right;
            if (app->getKeyboard().isPressed(GLFW_KEY_Q)) direction += mv.up;
            if (app->getKeyboard().isPressed(GLFW_KEY_E)) direction -= mv.up;
            return direction;
        }

        bool isMoving() {
            return app->getKeyboard().isPressed(GLFW_KEY_W) ||
                   app->getKeyboard().isPressed(GLFW_KEY_S) ||
                   app->getKeyboard().isPressed(GLFW_KEY_A) ||
                   app->getKeyboard().isPressed(GLFW_KEY_D);
        }

        bool isSprinting() {
            return app->getKeyboard().isPressed(GLFW_KEY_LEFT_SHIFT);
        }

        void applyMovement(JPH::BodyInterface* bodyInterface, RigidBodyComponent* rb, const glm::vec3& direction) {
            float speed = BASE_SPEED * (isSprinting() ? SPRINT_MULTIPLIER : 1.0f);
            JPH::Vec3 currentVel = bodyInterface->GetLinearVelocity(rb->runtimeBodyID);
            JPH::Vec3 newVel(direction.x * speed, currentVel.GetY(), direction.z * speed);
            bodyInterface->SetLinearVelocity(rb->runtimeBodyID, newVel);
        }

        void updateRotation(Entity* playerEntity, Entity* cameraEntity, const glm::vec3& direction) {
            glm::vec3& rotation = playerEntity->localTransform.rotation;
            if (app->getMouse().isPressed(GLFW_MOUSE_BUTTON_2)) {
                rotation.y = cameraEntity->localTransform.rotation.y + glm::pi<float>();
            } else if (glm::length(direction) > 0) {
                glm::vec3 normalized = glm::normalize(direction);
                rotation.y = glm::atan(normalized.x, normalized.z);
            }
        }

        void handleJump(PhysicsSystem* physicsSystem, JPH::BodyInterface* bodyInterface,
                       Entity* playerEntity, RigidBodyComponent* rb, const glm::vec3& up) {
            if (!app->getKeyboard().justPressed(GLFW_KEY_SPACE)) return;

            LayerFilter groundFilter({Layers::PLAYER, Layers::PLAYER_ATTACK});
            float rayLength = 1.0f / glm::length(playerEntity->getLocalToWorldMatrix()[1]);
            RaycastHit hit = physicsSystem->Raycast(playerEntity->getLocalToWorldMatrix()[3], -up, rayLength, groundFilter);
            
            if (hit.hasHit) {
                std::cout << "Jump!" << std::endl;
                bodyInterface->AddImpulse(rb->runtimeBodyID, JPH::Vec3(0, JUMP_FORCE, 0));
            }
        }

        // ==================== Combat ====================
        
        void handleShooting(World* world, Entity* playerEntity, Entity* cameraEntity, Entity* weapon) {
            if (!app->getMouse().justPressed(GLFW_MOUSE_BUTTON_1) || !weapon) return;

            glm::vec3 shotDir = glm::normalize(glm::vec3(playerEntity->getLocalToWorldMatrix() * glm::vec4(0, 0, 1, 0)));
            glm::vec3 shotHeight = glm::normalize(glm::vec3(cameraEntity->getLocalToWorldMatrix() * glm::vec4(0, 0, -1, 0)));
            JPH::Vec3 impulse = JPH::Vec3(shotDir.x, shotHeight.y, shotDir.z) * BULLET_SPEED;
            
            ObjectSpawner::spawnObject(
                world, nullptr, object_data,
                glm::vec3(weapon->getLocalToWorldMatrix()[3]),
                glm::vec3(0, 0, 0), glm::vec3(0.02f),
                impulse, 10.0f, "player_attack"
            );
        }

        void handleHealing(Entity* playerEntity, CharacterComponent* character) {
            if (!app->getKeyboard().justPressed(GLFW_KEY_H)) return;

            if (character->getHealth() >= 100) {
                std::cout << "Health is already full." << std::endl;
            } else if (healCounter >= MAX_HEALS) {
                std::cout << "Heal limit reached. Cannot heal more than " << MAX_HEALS << " times." << std::endl;
            } else {
                HealCharacter(playerEntity, HEAL_AMOUNT);
                healCounter++;
            }
        }

        // ==================== Animation ====================
        
        void tryPlayAnimation(AnimatorComponent* animator, const std::string& animName) {
            if (animator->hasAnimation(animName) && animator->getCurrentAnimationName() != animName) {
                animator->setAnimation(animName);
                animator->play();
            }
        }

        void handleAttackAnimation(AnimatorComponent* animator, const std::string& weaponName) {
            if (weaponName.empty()) {
                if (animator->hasAnimation("attack")) {
                    animator->setAnimation("attack");
                    animator->play();
                    isPlayingAttackAnimation = true;
                }
            } else if (weaponName == "player_gun") {
                if (animator->hasAnimation("GunAttack")) {
                    animator->setAnimation("GunAttack");
                    animator->play();
                    isPlayingAttackAnimation = true;
                }
            } else if (weaponName == "player_katana") {
                const char* anim = leftAttackPressed ? "KatanaSlashR" : "KatanaSlashL";
                if (animator->hasAnimation(anim)) {
                    animator->setAnimation(anim);
                    animator->play();
                    isPlayingAttackAnimation = true;
                    leftAttackPressed = !leftAttackPressed;
                }
            } else if (weaponName == "player_rifle") {
                if (animator->hasAnimation("RifleShoot")) {
                    animator->setAnimation("RifleShoot");
                    animator->play();
                    isPlayingAttackAnimation = true;
                }
            }
        }

        void handleMovementAnimation(AnimatorComponent* animator, const std::string& weaponName) {
            bool sprinting = isSprinting();
            if (weaponName.empty()) {
                tryPlayAnimation(animator, sprinting ? "run" : "walk");
            } else if (weaponName == "player_gun") {
                tryPlayAnimation(animator, sprinting ? "GunRun" : "GunWalk");
            } else if (weaponName == "player_katana") {
                tryPlayAnimation(animator, sprinting ? "KatanaRun" : "KatanaWalk");
            } else if (weaponName == "player_rifle") {
                tryPlayAnimation(animator, sprinting ? "RifleRun" : "RifleWalk");
            }
        }

        void handleIdleAnimation(AnimatorComponent* animator, const std::string& weaponName) {
            if (weaponName.empty()) {
                tryPlayAnimation(animator, "idle");
            } else if (weaponName == "player_gun") {
                tryPlayAnimation(animator, "GunIdle");
            } else if (weaponName == "player_katana") {
                tryPlayAnimation(animator, "KatanaIdle");
            } else if (weaponName == "player_rifle") {
                tryPlayAnimation(animator, "RifleIdle");
            }
        }

        void updateAnimations(AnimatorComponent* animator, InventoryComponent* inventory) {
            if (!animator || !animator->enabled) return;

            std::string weaponName = getActiveWeaponName(inventory);
            bool walking = isMoving();
            bool attacking = app->getMouse().isPressed(GLFW_MOUSE_BUTTON_1);

            // Check if attack animation finished
            if (isPlayingAttackAnimation && animator->isAnimationFinished()) {
                isPlayingAttackAnimation = false;
            }

            // Don't interrupt attack animations
            if (isPlayingAttackAnimation && !animator->isAnimationFinished()) {
                return;
            }

            // Priority: Attack > Walk > Idle
            if (attacking) {
                handleAttackAnimation(animator, weaponName);
            } else if (walking) {
                handleMovementAnimation(animator, weaponName);
            } else {
                handleIdleAnimation(animator, weaponName);
            }

            wasWalking = walking;
        }

        // ==================== Main Update Handlers ====================
        
        void handleAlivePlayer(World* world, PhysicsSystem* physicsSystem, JPH::BodyInterface* bodyInterface,
                              Entity* playerEntity, Entity* cameraEntity, CharacterComponent* character) {
            auto rb = playerEntity->getComponent<RigidBodyComponent>();
            auto animator = playerEntity->getComponent<AnimatorComponent>();
            auto inventory = playerEntity->getComponent<InventoryComponent>();

            // Calculate movement
            MovementVectors mv = calculateMovementVectors(playerEntity, cameraEntity);
            glm::vec3 inputDir = getInputDirection(mv);

            // Apply movement and rotation
            applyMovement(bodyInterface, rb, inputDir);
            updateRotation(playerEntity, cameraEntity, inputDir);

            // Handle actions
            handleJump(physicsSystem, bodyInterface, playerEntity, rb, mv.up);
            handleHealing(playerEntity, character);

            // Handle combat
            Entity* weapon = findActiveWeapon(playerEntity, inventory);
            handleShooting(world, playerEntity, cameraEntity, weapon);

            // Update animations
            updateAnimations(animator, inventory);
        }

        void handleDeadPlayer(Entity* playerEntity) {
            auto animator = playerEntity->getComponent<AnimatorComponent>();
            if (animator && animator->enabled && animator->isAnimationFinished()) {
                animator->stop();
                app->changeState("dead");
            }
        }

    public:

        static void onCollision(Entity* self, Entity* other) {
            if(other->layer == "enemy_attack") 
            {
                // Only process damage from active projectiles
                if (other->timeRemaining <= 0) return;
                
                CharacterComponent* character = self->getComponent<CharacterComponent>();
                
                // Don't take damage if already dead
                if (!character->getAlive()) return;
                
                character->setHealth(-10);
                std::cout << "Character Health: " << character->getHealth() << std::endl;
                
                // Mark bullet as used
                other->timeRemaining = 0;
                
                if(character->getHealth() == 0){
                    character->setAlive(false);
                    if (auto animator = self->getComponent<AnimatorComponent>()) {
                        if (animator->hasAnimation("Death")) {
                            animator->setAnimation("Death");
                            animator->play();
                        }
                    }
                }
            }
        }

        static void HealCharacter (Entity* self, int amount) {
            CharacterComponent* character = self->getComponent<CharacterComponent>();
            if (character->getAlive()) {
                if (character->getHealth() + amount > 100) {
                    character->setHealth(100 - character->getHealth()); // Cap health at 100
                }
                else {
                    character->setHealth(amount);
                }
                std::cout << "Character Healed. Current Health: " << character->getHealth() << std::endl;
            }
        }   
        // When the state exits, it should call this function to ensure the mouse is unlocked
        void exit(){}
    };

}

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
    // Boss states - more complex than regular NPC
    enum class BossState {
        IDLE,           // Standing still, waiting
        PATROL,         // Walking around patrol points
        PURSUIT,        // Chasing the player
        ATTACK,         // Performing an attack
        BLOCK,          // Blocking incoming attacks
        STUNNED,        // Temporarily stunned after taking damage
        DEAD            // Boss is defeated
    };

    // The Boss Controller handles the Taskmaster boss enemy
    // Similar to NPC but with animations and more complex combat logic
    class BossControllerSystem {
        Application* app;
        nlohmann::json object_data;
        
        // Animation tracking
        bool isPlayingAnimation = false;
        std::string currentAnimState = "idle";
        
        // Boss activation state (controlled by GameManager) - static so onCollision can check it
        static inline bool bossActivated = false;
        
        // Attack cooldown
        static constexpr float ATTACK_COOLDOWN = 2.0f;  // 2 seconds between attacks
        float attackTimer = 0.0f;
        
    public:
        void enter(Application* app) {
            this->app = app;
            std::string config_path = ("config/bullet.jsonc");

            std::ifstream file_in(config_path);
            if (!file_in)
                std::cerr << "Couldn't open file: " << config_path << std::endl;
            else
                object_data = nlohmann::json::parse(file_in, nullptr, true, true);
                
            isPlayingAnimation = false;
            currentAnimState = "idle";
            bossActivated = false;
            attackTimer = ATTACK_COOLDOWN;  // Start ready to attack
        }
        
        // Called by GameManager to activate the boss
        void activateBoss() { 
            bossActivated = true; 
            std::cout << "Taskmaster Activated!" << std::endl;
        }
        
        bool isBossActivated() const { return bossActivated; }

        void update(World* world, float deltaTime, our::PhysicsSystem* physicsSystem) {
            JPH::BodyInterface* bodyInterface = physicsSystem->getBodyInterface();
            if (!bodyInterface) return;
            
            // Update attack cooldown
            if (attackTimer < ATTACK_COOLDOWN) {
                attackTimer += deltaTime;
            }
            
            for (auto entity : world->getEntities()) {
                // Only process the taskmaster boss
                if (entity->name != "taskmaster") continue;
                
                CharacterComponent* character = entity->getComponent<CharacterComponent>();
                if (!character) continue;
                

                
                AnimatorComponent* animator = entity->getComponent<AnimatorComponent>();
                RigidBodyComponent* rb = entity->getComponent<RigidBodyComponent>();
                
                // Skip if boss is dead
                if (!character->getAlive()) {
                    handleDeathAnimation(entity);
                    if (animator && animator->isAnimationFinished()) animator->stop();
                    continue;
                }

                // Skip if no rigidbody or body not yet created
                if (!rb || rb->runtimeBodyID.IsInvalid()) continue;
                
                // If boss is not activated yet, just stay in idle
                if (!bossActivated) {
                    setAnimation(animator, "idle");
                    // Stop any movement
                    JPH::Vec3 currentVel = bodyInterface->GetLinearVelocity(rb->runtimeBodyID);
                    bodyInterface->SetLinearVelocity(rb->runtimeBodyID, JPH::Vec3(0, currentVel.GetY(), 0));
                    continue;
                }
                
                // Get boss world position
                glm::mat4 worldTransform = entity->getLocalToWorldMatrix();
                glm::vec3 bossPos = glm::vec3(worldTransform[3]);
                bossPos.y += 1.5f;
                
                // Get forward direction (with safety check)
                glm::vec3 forwardRaw = glm::vec3(worldTransform * glm::vec4(0, 0, 1, 0));
                float forwardLen = glm::length(forwardRaw);
                glm::vec3 forward = (forwardLen > 0.001f) ? forwardRaw / forwardLen : glm::vec3(0, 0, 1);
                
                // Scan for player with wide cone
                LayerFilter myFilter({Layers::ENEMY, Layers::ENEMY_ATTACK, Layers::PLAYER_ATTACK, Layers::ENEMY_AWARENESS});
                RaycastHit hit;
                hit.hasHit = false;
                
                // Wider detection cone for boss (more aware)
                for (int i = -10; i <= 10; i++) {
                    float angle = glm::radians(i * 9.0f);  // 180 degree cone
                    glm::vec3 currentDir = glm::rotate(forward, angle, glm::vec3(0, 1, 0));
                    hit = physicsSystem->Raycast(bossPos, currentDir, 250, myFilter);
                    
                    if (hit.hasHit && hit.entity->layer == "player") break;
                }
                
                // Get current state
                States state = character->getState();
                float stoppingDistance = 0.8f;
                bool playerVisible = hit.hasHit && hit.entity->layer == "player";
                float distanceToPlayer = playerVisible ? glm::length(bossPos - hit.position) : 999.0f;
                
                // State machine logic
                if (playerVisible) {
                    character->setState(States::PURSUIT);
                    
                    // Determine if we should attack or block based on distance
                    if (distanceToPlayer < 3.0f) {
                        // Close range - attack or block
                        stoppingDistance = 2.5f;
                        
                        // Handle attack timer
                        float timer = character->getTimer();
                        timer -= deltaTime;
                        
                        if (timer <= 0) {
                            // Attack!
                            performAttack(world, entity, bossPos, hit.position);
                            character->setTimer(0, true);  // Reset timer
                            setAnimation(animator, "attack");
                        } else {
                            character->setTimer(timer, false);
                            // While waiting to attack, use idle or block
                            setAnimation(animator, "block");
                        }
                    } else if (distanceToPlayer < 8.0f) {
                        // Medium range - run towards player
                        stoppingDistance = 2.5f;
                        character->updateTarget(hit.position);
                        setAnimation(animator, "run");
                    } else {
                        // Long range - walk towards player
                        stoppingDistance = 2.5f;
                        character->updateTarget(hit.position);
                        setAnimation(animator, "walk");
                    }
                } else if (state == States::PURSUIT) {
                    // Lost sight of player, go to investigation
                    character->setState(States::INVESTIGATION);
                    setAnimation(animator, "walk");
                } else if (state == States::INVESTIGATION) {
                    // Investigate last known position
                    glm::vec3 target = character->getTarget();
                    target.y = bossPos.y;
                    float distance = glm::length(glm::abs(bossPos - target));
                    stoppingDistance = 0.8f;
                    
                    if (distance < 0.9f) {
                        character->updateTarget();
                        character->setState(States::PATROL);
                        setAnimation(animator, "idle");
                    } else {
                        setAnimation(animator, "walk");
                    }
                } else if (state == States::PATROL) {
                    // Patrol between waypoints
                    glm::vec3 target = character->getTarget();
                    target.y = bossPos.y;
                    float distance = glm::length(glm::abs(bossPos - target));
                    
                    if (distance < 0.9f) {
                        character->updateTarget();
                    }
                    stoppingDistance = 0.8f;
                    setAnimation(animator, "walk");
                }
                
                // Movement
                glm::vec3 target = character->getTarget();
                glm::vec3 toTarget = target - bossPos;
                float distance = glm::length(toTarget);
                
                JPH::Vec3 currentVel = bodyInterface->GetLinearVelocity(rb->runtimeBodyID);
                
                // Avoid normalizing zero vector
                glm::vec3 moveDir = (distance > 0.01f) ? glm::normalize(toTarget) : glm::vec3(0, 0, 0);
                
                if (distance > stoppingDistance && glm::length(moveDir) > 0.01f) {
                    // Speed based on state
                    float speed = (state == States::PURSUIT && distanceToPlayer > 3.0f) ? 5.0f : 2.5f;
                    
                    JPH::Vec3 newVel(
                        moveDir.x * speed,
                        currentVel.GetY(),
                        moveDir.z * speed
                    );
                    
                    bodyInterface->SetLinearVelocity(rb->runtimeBodyID, newVel);
                } else {
                    // Stop horizontal movement
                    JPH::Vec3 newVel(0, currentVel.GetY(), 0);
                    bodyInterface->SetLinearVelocity(rb->runtimeBodyID, newVel);
                }
                
                // Face movement direction
                glm::vec3& rotation = entity->localTransform.rotation;
                if (glm::length(moveDir) > 0.01f) {
                    float targetAngle = glm::atan(moveDir.x, moveDir.z);
                    rotation.y = targetAngle;
                }
            }
        }
        
    private:
        void setAnimation(AnimatorComponent* animator, const std::string& animName) {
            if (!animator || !animator->enabled) return;
            
            // Don't interrupt if same animation
            if (currentAnimState == animName) return;
            
            if (animator->hasAnimation(animName)) {
                animator->setAnimation(animName);
                animator->play();
                currentAnimState = animName;
            }
        }
        
        void performAttack(World* world, Entity* boss, glm::vec3 bossPos, glm::vec3 targetPos) {
            // Boss throws a projectile or performs melee
            glm::vec3 attackDir = glm::normalize(targetPos - bossPos);
            JPH::Vec3 impulse = JPH::Vec3(attackDir.x, attackDir.y, attackDir.z) * 120.0f;
            
            ObjectSpawner::spawnObject(
                world,
                nullptr,
                object_data,
                bossPos,
                glm::vec3(0.0f),
                glm::vec3(0.15f),  // Slightly bigger projectile
                impulse,
                10.0f,
                "enemy_attack"
            );
            
            std::cout << "Taskmaster attacks!" << std::endl;
        }
        
        void handleDeathAnimation(Entity* entity) {
            auto animator = entity->getComponent<AnimatorComponent>();
            if (animator && animator->enabled) {
                // Play death animation if available
                setAnimation(animator, "death");
                // The boss entity should remain in the world during victory sequence
            }
        }
        
    public:
        static void onCollision(Entity* self, Entity* other) {
            if (other->layer == "player_attack") {
                // Only process damage from active projectiles
                if (other->timeRemaining <= 0) return;
                
                // Don't take damage if boss fight hasn't started yet
                if (!bossActivated) {
                    // Just destroy the bullet without dealing damage
                    other->timeRemaining = 0;
                    return;
                }
                
                CharacterComponent* character = self->getComponent<CharacterComponent>();
                if (!character) return;
                
                // Don't take damage if already dead
                if (!character->getAlive()) return;
                
                // Boss takes less damage (more health)
                character->setHealth(-5);  // Half damage compared to regular enemies
                std::cout << "Taskmaster Health: " << character->getHealth() << std::endl;
                
                // Mark bullet as used
                other->timeRemaining = 0;
                
                // TODO: Check if blocking - if so, reduce damage further or negate
                
                if (character->getHealth() <= 0) {
                    character->setAlive(false);
                    std::cout << "Taskmaster defeated!" << std::endl;
                }
            }
        }
        
        void exit() {
            // Cleanup if needed
        }
    };
}

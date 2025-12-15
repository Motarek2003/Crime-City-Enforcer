#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <flags/flags.h>
#include <json/json.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>
#include <glm/gtx/fast_trigonometry.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <Jolt/Physics/Body/BodyInterface.h>

#include "../application.hpp"
#include "../components/character.hpp"
#include "../components/animator.hpp"
#include "../components/rigidbody.hpp"
#include "../ecs/world.hpp"
#include "./physics-system.hpp"
#include "./object-spawner.hpp"

namespace our
{
    // The Boss Controller handles the Taskmaster boss enemy
    // Similar to NPC but with animations and more complex combat logic
    class BossControllerSystem {
    private:
        // Constants
        static constexpr float ATTACK_COOLDOWN = 2.0f;
        static constexpr float DETECTION_RANGE = 250.0f;
        static constexpr float CLOSE_RANGE = 3.0f;
        static constexpr float MEDIUM_RANGE = 8.0f;
        static constexpr float STOPPING_DISTANCE_COMBAT = 2.5f;
        static constexpr float STOPPING_DISTANCE_PATROL = 0.8f;
        static constexpr float RUN_SPEED = 5.0f;
        static constexpr float WALK_SPEED = 2.5f;
        static constexpr float PROJECTILE_SPEED = 120.0f;
        static constexpr int BOSS_DAMAGE_TAKEN = 5;

        Application* app = nullptr;
        nlohmann::json object_data;
        
        bool isPlayingAnimation = false;
        std::string currentAnimState = "idle";
        
        static inline bool bossActivated = false;
        float attackTimer = 0.0f;

    public:
        void enter(Application* app) {
            this->app = app;
            loadConfig();
            resetState();
        }
        
        void activateBoss() { 
            bossActivated = true; 
            std::cout << "Taskmaster Activated!" << std::endl;
        }
        
        bool isBossActivated() const { return bossActivated; }

        void update(World* world, float deltaTime, our::PhysicsSystem* physicsSystem) {
            JPH::BodyInterface* bodyInterface = physicsSystem->getBodyInterface();
            if (!bodyInterface) return;
            
            updateTimers(deltaTime);
            
            for (auto entity : world->getEntities()) {
                if (entity->name != "taskmaster") continue;
                
                processBossEntity(world, entity, deltaTime, physicsSystem, bodyInterface);
            }
        }

        static void onCollision(Entity* self, Entity* other) {
            if (other->layer != "player_attack" || other->timeRemaining <= 0) return;
            
            if (!bossActivated) {
                other->timeRemaining = 0;
                return;
            }
            
            CharacterComponent* character = self->getComponent<CharacterComponent>();
            if (!character || !character->getAlive()) return;
            
            character->setHealth(-BOSS_DAMAGE_TAKEN);
            std::cout << "Taskmaster Health: " << character->getHealth() << std::endl;
            
            other->timeRemaining = 0;
            
            if (character->getHealth() <= 0) {
                character->setAlive(false);
                std::cout << "Taskmaster defeated!" << std::endl;
            }
        }
        
        void exit() {}

    private:
        void loadConfig() {
            std::string config_path = "config/bullet.jsonc";
            std::ifstream file_in(config_path);
            if (!file_in) {
                std::cerr << "Couldn't open file: " << config_path << std::endl;
                return;
            }
            object_data = nlohmann::json::parse(file_in, nullptr, true, true);
        }

        void resetState() {
            isPlayingAnimation = false;
            currentAnimState = "idle";
            bossActivated = false;
            attackTimer = ATTACK_COOLDOWN;
        }

        void updateTimers(float deltaTime) {
            if (attackTimer < ATTACK_COOLDOWN) {
                attackTimer += deltaTime;
            }
        }

        void processBossEntity(World* world, Entity* entity, float deltaTime, PhysicsSystem* physicsSystem, JPH::BodyInterface* bodyInterface) {
            CharacterComponent* character = entity->getComponent<CharacterComponent>();
            AnimatorComponent* animator = entity->getComponent<AnimatorComponent>();
            RigidBodyComponent* rb = entity->getComponent<RigidBodyComponent>();

            if (!character || !rb || rb->runtimeBodyID.IsInvalid()) return;

            if (!character->getAlive()) {
                handleDeath(entity, animator);
                return;
            }

            if (!bossActivated) {
                handleIdle(animator, rb, bodyInterface);
                return;
            }

            glm::vec3 bossPos = getBossPosition(entity);
            glm::vec3 forward = getForwardDirection(entity);
            
            RaycastHit hit = scanForPlayer(physicsSystem, bossPos, forward);
            bool playerVisible = hit.hasHit && hit.entity->layer == "player";
            float distanceToPlayer = playerVisible ? glm::length(bossPos - hit.position) : 999.0f;

            updateAIState(character, animator, playerVisible, distanceToPlayer, hit.position, deltaTime, world, entity, bossPos);
            
            handleMovement(character, rb, bodyInterface, bossPos, distanceToPlayer, entity);
        }

        void handleDeath(Entity* entity, AnimatorComponent* animator) {
            if (animator && animator->enabled) {
                setAnimation(animator, "death");
                if (animator->isAnimationFinished()) animator->stop();
            }
        }

        void handleIdle(AnimatorComponent* animator, RigidBodyComponent* rb, JPH::BodyInterface* bodyInterface) {
            setAnimation(animator, "idle");
            JPH::Vec3 currentVel = bodyInterface->GetLinearVelocity(rb->runtimeBodyID);
            bodyInterface->SetLinearVelocity(rb->runtimeBodyID, JPH::Vec3(0, currentVel.GetY(), 0));
        }

        glm::vec3 getBossPosition(Entity* entity) {
            glm::mat4 worldTransform = entity->getLocalToWorldMatrix();
            glm::vec3 pos = glm::vec3(worldTransform[3]);
            pos.y += 1.5f;
            return pos;
        }

        glm::vec3 getForwardDirection(Entity* entity) {
            glm::mat4 worldTransform = entity->getLocalToWorldMatrix();
            glm::vec3 forwardRaw = glm::vec3(worldTransform * glm::vec4(0, 0, 1, 0));
            float forwardLen = glm::length(forwardRaw);
            return (forwardLen > 0.001f) ? forwardRaw / forwardLen : glm::vec3(0, 0, 1);
        }

        RaycastHit scanForPlayer(PhysicsSystem* physicsSystem, const glm::vec3& origin, const glm::vec3& forward) {
            LayerFilter filter({Layers::ENEMY, Layers::ENEMY_ATTACK, Layers::PLAYER_ATTACK, Layers::ENEMY_AWARENESS});
            RaycastHit hit;
            hit.hasHit = false;

            for (int i = -10; i <= 10; i++) {
                float angle = glm::radians(i * 9.0f);
                glm::vec3 currentDir = glm::rotate(forward, angle, glm::vec3(0, 1, 0));
                hit = physicsSystem->Raycast(origin, currentDir, DETECTION_RANGE, filter);
                
                if (hit.hasHit && hit.entity->layer == "player") return hit;
            }
            return hit;
        }

        void updateAIState(CharacterComponent* character, AnimatorComponent* animator, bool playerVisible, float distanceToPlayer, const glm::vec3& targetPos, float deltaTime, World* world, Entity* entity, const glm::vec3& bossPos) {
            States state = character->getState();

            if (playerVisible) {
                character->setState(States::PURSUIT);
                handleCombat(character, animator, distanceToPlayer, targetPos, deltaTime, world, entity, bossPos);
            } else if (state == States::PURSUIT) {
                character->setState(States::INVESTIGATION);
                setAnimation(animator, "walk");
            } else if (state == States::INVESTIGATION || state == States::PATROL) {
                handlePatrolOrInvestigate(character, animator, bossPos, state);
            }
        }

        void handleCombat(CharacterComponent* character, AnimatorComponent* animator, float distanceToPlayer, const glm::vec3& targetPos, float deltaTime, World* world, Entity* entity, const glm::vec3& bossPos) {
            if (distanceToPlayer < CLOSE_RANGE) {
                float timer = character->getTimer();
                timer -= deltaTime;
                
                if (timer <= 0) {
                    performAttack(world, entity, bossPos, targetPos);
                    character->setTimer(0, true);
                    setAnimation(animator, "attack");
                } else {
                    character->setTimer(timer, false);
                    setAnimation(animator, "block");
                }
            } else if (distanceToPlayer < MEDIUM_RANGE) {
                character->updateTarget(targetPos);
                setAnimation(animator, "run");
            } else {
                character->updateTarget(targetPos);
                setAnimation(animator, "walk");
            }
        }

        void handlePatrolOrInvestigate(CharacterComponent* character, AnimatorComponent* animator, const glm::vec3& bossPos, States state) {
            glm::vec3 target = character->getTarget();
            target.y = bossPos.y;
            float distance = glm::length(glm::abs(bossPos - target));
            
            if (distance < 0.9f) {
                character->updateTarget();
                if (state == States::INVESTIGATION) {
                    character->setState(States::PATROL);
                    setAnimation(animator, "idle");
                }
            } else {
                setAnimation(animator, "walk");
            }
        }

        void handleMovement(CharacterComponent* character, RigidBodyComponent* rb, JPH::BodyInterface* bodyInterface, const glm::vec3& bossPos, float distanceToPlayer, Entity* entity) {
            glm::vec3 target = character->getTarget();
            glm::vec3 toTarget = target - bossPos;
            float distance = glm::length(toTarget);
            
            JPH::Vec3 currentVel = bodyInterface->GetLinearVelocity(rb->runtimeBodyID);
            
            glm::vec3 moveDir = (distance > 0.01f) ? glm::normalize(toTarget) : glm::vec3(0);
            
            float stoppingDistance = (character->getState() == States::PURSUIT) ? STOPPING_DISTANCE_COMBAT : STOPPING_DISTANCE_PATROL;

            if (distance > stoppingDistance && glm::length(moveDir) > 0.01f) {
                float speed = (character->getState() == States::PURSUIT && distanceToPlayer > CLOSE_RANGE) ? RUN_SPEED : WALK_SPEED;
                
                bodyInterface->SetLinearVelocity(rb->runtimeBodyID, JPH::Vec3(moveDir.x * speed, currentVel.GetY(), moveDir.z * speed));
                
                // Face movement direction
                entity->localTransform.rotation.y = glm::atan(moveDir.x, moveDir.z);
            } else {
                bodyInterface->SetLinearVelocity(rb->runtimeBodyID, JPH::Vec3(0, currentVel.GetY(), 0));
            }
        }

        void setAnimation(AnimatorComponent* animator, const std::string& animName) {
            if (!animator || !animator->enabled || currentAnimState == animName) return;
            
            if (animator->hasAnimation(animName)) {
                animator->setAnimation(animName);
                animator->play();
                currentAnimState = animName;
            }
        }

        void performAttack(World* world, Entity* boss, glm::vec3 bossPos, glm::vec3 targetPos) {
            glm::vec3 attackDir = glm::normalize(targetPos - bossPos);
            JPH::Vec3 impulse = JPH::Vec3(attackDir.x, attackDir.y, attackDir.z) * PROJECTILE_SPEED;
            
            ObjectSpawner::spawnObject(
                world,
                nullptr,
                object_data,
                bossPos,
                glm::vec3(0.0f),
                glm::vec3(0.15f),
                impulse,
                10.0f,
                "enemy_attack"
            );
            
            std::cout << "Taskmaster attacks!" << std::endl;
        }
    };
}

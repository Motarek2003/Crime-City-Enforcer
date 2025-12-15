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
    class EnemyControllerSystem {
    private:
        // Constants
        static constexpr float BOSS_ATTACK_COOLDOWN = 2.0f;
        static constexpr float GRUNT_ATTACK_COOLDOWN = 3.0f;
        
        static constexpr float BOSS_DETECTION_RANGE = 250.0f;
        static constexpr float GRUNT_DETECTION_RANGE = 200.0f;
        
        static constexpr int BOSS_FOV = 80;
        static constexpr int GRUNT_FOV = 60;
        
        static constexpr int BOSS_RAYS = 32;
        static constexpr int GRUNT_RAYS = 12;
        
        static constexpr int BOSS_DAMAGE_TAKEN = 5;
        static constexpr int GRUNT_DAMAGE_TAKEN = 10;
        
        static constexpr float PROJECTILE_SPEED = 100.0f;
        static constexpr float BASE_MOVE_SPEED = 2.0f;

        Application* app = nullptr;
        nlohmann::json bullet_config;

        // Global Boss State
        static inline bool bossActivated = false;

    public:
        void enter(Application* app) {
            this->app = app;
            loadConfig();
            bossActivated = false;
        }

        void activateBoss() { 
            bossActivated = true; 
            std::cout << "BOSS ACTIVATED!" << std::endl; 
        }
        
        bool isBossActivated() const { return bossActivated; }

        void update(World* world, float deltaTime, our::PhysicsSystem* physicsSystem) {
            JPH::BodyInterface* bodyInterface = physicsSystem->getBodyInterface();
            if (!bodyInterface) return;

            for (auto entity : world->getEntities()) {
                bool isBoss = (entity->name == "taskmaster");
                bool isGrunt = (entity->name == "enemy");

                if (!isBoss && !isGrunt) continue;

                processEntity(world, entity, deltaTime, physicsSystem, bodyInterface, isBoss);
            }
        }

        static void onCollision(Entity* self, Entity* other) {
            if (other->layer != "player_attack" || other->timeRemaining <= 0) return;

            CharacterComponent* character = self->getComponent<CharacterComponent>();
            if (!character || !character->getAlive()) return;

            bool isBoss = (self->name == "taskmaster");
            if (isBoss && !bossActivated) return;

            int damage = isBoss ? -BOSS_DAMAGE_TAKEN : -GRUNT_DAMAGE_TAKEN;
            character->setHealth(damage);
            other->timeRemaining = 0; // Destroy bullet

            std::cout << self->name << " Health: " << character->getHealth() << std::endl;

            if (character->getHealth() <= 0) {
                character->setAlive(false);
                if (!isBoss) {
                    self->timeRemaining = -1; // Grunts disappear instantly
                } else {
                    std::cout << "Boss Defeated!" << std::endl;
                }
            }
        }

        static void onTrigger(Entity* self, Entity* other) {
            if (other->layer != "player_attack" || other->timeRemaining <= 0) return;
            
            CharacterComponent* character = self->getComponent<CharacterComponent>();
            if (!character || !character->getAlive()) return;

            // Turn to face the attacker
            glm::vec3 selfPos = glm::vec3(self->getLocalToWorldMatrix()[3]);
            glm::vec3 otherPos = glm::vec3(other->getLocalToWorldMatrix()[3]);
            glm::vec3 dirToAttacker = glm::normalize(otherPos - selfPos);

            if (glm::length(dirToAttacker) > 0.001f) {
                float targetAngle = glm::atan(dirToAttacker.x, dirToAttacker.z);
                self->localTransform.rotation.y = targetAngle;
            }

            if (character->getState() != States::PURSUIT) {
                character->setState(States::INVESTIGATION);
                character->updateTarget(otherPos);
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
            bullet_config = nlohmann::json::parse(file_in, nullptr, true, true);
        }

        void processEntity(World* world, Entity* entity, float deltaTime, PhysicsSystem* physicsSystem, JPH::BodyInterface* bodyInterface, bool isBoss) {
            CharacterComponent* character = entity->getComponent<CharacterComponent>();
            AnimatorComponent* animator = entity->getComponent<AnimatorComponent>();
            RigidBodyComponent* rb = entity->getComponent<RigidBodyComponent>();

            if (!character || !rb || rb->runtimeBodyID.IsInvalid()) return;

            if (!character->getAlive()) {
                if (isBoss) handleBossDeath(entity, animator);
                return;
            }

            if (isBoss && !bossActivated) {
                handleIdle(animator, rb, bodyInterface);
                return;
            }

            glm::vec3 position = getEntityPosition(entity);
            glm::vec3 forward = getForwardDirection(entity);

            RaycastHit hit = scanForPlayer(physicsSystem, position, forward, isBoss);
            bool playerFound = (hit.hasHit && hit.entity->layer == "player");

            updateAIState(world, entity, character, animator, playerFound, hit.position, position, deltaTime, isBoss);
            
            handleMovement(bodyInterface, rb, entity, position, character->getTarget(), character->getState(), isBoss);
        }

        void handleIdle(AnimatorComponent* animator, RigidBodyComponent* rb, JPH::BodyInterface* bodyInterface) {
            setAnimation(animator, "idle");
            JPH::Vec3 currentVel = bodyInterface->GetLinearVelocity(rb->runtimeBodyID);
            bodyInterface->SetLinearVelocity(rb->runtimeBodyID, JPH::Vec3(0, currentVel.GetY(), 0));
        }

        void handleBossDeath(Entity* entity, AnimatorComponent* animator) {
            if (animator) setAnimation(animator, "death");
        }

        glm::vec3 getEntityPosition(Entity* entity) {
            glm::mat4 worldTransform = entity->getLocalToWorldMatrix();
            glm::vec3 pos = glm::vec3(worldTransform[3]);
            pos.y += 1.5f;
            return pos;
        }

        glm::vec3 getForwardDirection(Entity* entity) {
            glm::mat4 worldTransform = entity->getLocalToWorldMatrix();
            return glm::normalize(glm::vec3(worldTransform * glm::vec4(0, 0, 1, 0)));
        }

        RaycastHit scanForPlayer(PhysicsSystem* physicsSystem, const glm::vec3& startPos, const glm::vec3& forward, bool isBoss) {
            int maxAngle = isBoss ? BOSS_FOV : GRUNT_FOV;
            int numRays = isBoss ? BOSS_RAYS : GRUNT_RAYS;
            float range = isBoss ? BOSS_DETECTION_RANGE : GRUNT_DETECTION_RANGE;

            LayerFilter filter({Layers::ENEMY, Layers::ENEMY_ATTACK, Layers::PLAYER_ATTACK, Layers::ENEMY_AWARENESS});
            RaycastHit hit;
            hit.hasHit = false;

            float step = (float)(maxAngle * 2) / numRays;
            for (float angle_deg = -maxAngle; angle_deg <= maxAngle; angle_deg += step) {
                float angle = glm::radians(angle_deg);
                glm::vec3 dir = glm::rotate(forward, angle, glm::vec3(0, 1, 0));
                
                hit = physicsSystem->Raycast(startPos, dir, range, filter);
                if (hit.hasHit && hit.entity->layer == "player") return hit;
            }
            return hit;
        }

        void updateAIState(World* world, Entity* entity, CharacterComponent* character, AnimatorComponent* animator, bool playerFound, const glm::vec3& targetPos, const glm::vec3& currentPos, float deltaTime, bool isBoss) {
            if (playerFound) {
                character->setState(States::PURSUIT);
                character->updateTarget(targetPos);
                handleCombat(world, character, animator, currentPos, targetPos, deltaTime, isBoss);
            }
            else if (character->getState() == States::PURSUIT) {
                character->setState(States::INVESTIGATION);
                if (isBoss) setAnimation(animator, "run");
            }
            else if (character->getState() == States::INVESTIGATION) {
                handleInvestigation(character, animator, currentPos);
            }
            else if (character->getState() == States::PATROL) {
                handlePatrol(character, animator, currentPos);
            }
        }

        void handleCombat(World* world, CharacterComponent* character, AnimatorComponent* animator, const glm::vec3& currentPos, const glm::vec3& targetPos, float deltaTime, bool isBoss) {
            float dist = glm::length(currentPos - targetPos);
            float cooldown = isBoss ? BOSS_ATTACK_COOLDOWN : GRUNT_ATTACK_COOLDOWN;

            if (isBoss) {
                if (dist < 3.0f) {
                    attack(world, character, animator, currentPos, targetPos, cooldown, deltaTime, "enemy_attack");
                } 
                else if (dist < 8.0f) {
                    setAnimation(animator, "walk");
                    attack(world, character, animator, currentPos, targetPos, cooldown, deltaTime, "enemy_attack");
                } 
                else {
                    setAnimation(animator, "run");
                }
            } else {
                // Grunt logic
                attack(world, character, nullptr, currentPos, targetPos, cooldown, deltaTime, "enemy_attack");
            }
        }

        void handleInvestigation(CharacterComponent* character, AnimatorComponent* animator, const glm::vec3& currentPos) {
            glm::vec3 target = character->getTarget();
            target.y = currentPos.y;
            
            if (glm::length(currentPos - target) < 0.9f) {
                character->updateTarget();
                character->setState(States::PATROL);
            }
            
            if (glm::length(currentPos - target) < 3.0f) {
                setAnimation(animator, "walk");
            }
        }

        void handlePatrol(CharacterComponent* character, AnimatorComponent* animator, const glm::vec3& currentPos) {
            glm::vec3 target = character->getTarget();
            target.y = currentPos.y;
            
            if (glm::length(currentPos - target) < 0.9f) {
                character->updateTarget();
            }
            setAnimation(animator, "walk");
        }

        void handleMovement(JPH::BodyInterface* bodyInterface, RigidBodyComponent* rb, Entity* entity, const glm::vec3& pos, const glm::vec3& target, States state, bool isBoss) {
            float stoppingDistance = 0.8f;
            float speedMultiplier = 1.0f;

            if (state == States::PURSUIT) {
                if (isBoss) {
                    float dist = glm::length(pos - target);
                    if (dist < 3.0f || (dist < 8.0f && dist >= 3.0f)) stoppingDistance = 2.5f;
                    if (dist >= 8.0f) speedMultiplier = 1.5f;
                } else {
                    stoppingDistance = 5.0f;
                }
            } else if (state == States::INVESTIGATION && !isBoss) {
                 // Grunt investigation speed up? Original code had speedUp = 1.5 for investigation
                 // But only if it was coming from PURSUIT. 
                 // Let's keep it simple for now or match original logic if needed.
                 // Original logic: if (character->getState() == States::PURSUIT) { ... speedUp = 1.5; ... }
                 // Wait, the original logic set speedUp = 1.5 when transitioning FROM pursuit TO investigation.
                 // And then in the next frame, if state is INVESTIGATION, it didn't explicitly set speedUp, so it would be 1.0.
                 // Actually, the original code defined `float speedUp = 1;` at the start of the loop.
                 // So it only sped up during the transition frame? That seems like a bug or negligible.
                 // However, for Boss, `speedUp = 1.5` was set in the `else` block of `dist < 8.0f`.
            }

            // Re-evaluating speed multiplier based on original logic more carefully
            if (isBoss && state == States::PURSUIT) {
                 float dist = glm::length(pos - target);
                 if (dist >= 8.0f) speedMultiplier = 1.5f;
            }
            
            // Move logic
            float distance = glm::length(glm::abs(target - pos));
            glm::vec3 moveDir = glm::normalize(target - pos);
            
            JPH::Vec3 currentVel = bodyInterface->GetLinearVelocity(rb->runtimeBodyID);

            if (distance > stoppingDistance) {
                float speed = BASE_MOVE_SPEED * speedMultiplier;
                JPH::Vec3 newVel(moveDir.x * speed, currentVel.GetY(), moveDir.z * speed);
                bodyInterface->SetLinearVelocity(rb->runtimeBodyID, newVel);
            } else {
                // Stop horizontal movement if reached target
                 bodyInterface->SetLinearVelocity(rb->runtimeBodyID, JPH::Vec3(0, currentVel.GetY(), 0));
            }

            // Rotation
            if (glm::length(moveDir) > 0.001f) {
                float targetAngle = glm::atan(moveDir.x, moveDir.z);
                entity->localTransform.rotation.y = targetAngle;
            }
        }

        void attack(World* world, CharacterComponent* character, AnimatorComponent* animator, const glm::vec3& start, const glm::vec3& target, float cooldown, float dt, const std::string& projectileName) {
            float timer = character->getTimer();
            timer -= dt;

            if (timer <= 0) {
                glm::vec3 dir = glm::normalize(target - start);
                JPH::Vec3 impulse(dir.x, 0, dir.z);
                impulse = impulse * PROJECTILE_SPEED;

                ObjectSpawner::spawnObject(world, nullptr, bullet_config, start, glm::vec3(0), glm::vec3(0.1f), impulse, 10.0f, projectileName);
                
                character->setTimer(cooldown, true);
                if (animator) setAnimation(animator, "attack");
                
                std::cout << (animator ? "Boss Attack!" : "Enemy Shoot!") << std::endl;
            } else {
                character->setTimer(timer, false);
            }
        }

        void setAnimation(AnimatorComponent* animator, const std::string& animName) {
            if (animator && animator->enabled && animator->hasAnimation(animName)) {
                if (animator->getCurrentAnimationName() != animName) {
                    animator->setAnimation(animName);
                    animator->play();
                }
            }
        }
    };
}

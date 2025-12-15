#pragma once

#include <iostream>
#include <fstream>
#include <flags/flags.h>
#include <json/json.hpp>
#include <random>

#include "../components/character.hpp"
#include "../components/animator.hpp"
#include "../components/rigidbody.hpp"
#include "./physics-system.hpp"
#include "./object-spawner.hpp"
#include "../application.hpp"

#include <glm/glm.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <Jolt/Physics/Body/BodyInterface.h>

namespace our
{
    class EnemyControllerSystem {
        Application* app;
        nlohmann::json bullet;

        // Global Boss State
        static inline bool bossActivated = false;
        static inline bool bossBlocking = false;  // For collision check

        const float BOSS_ATTACK_COOLDOWN = 2.0f;
        const float GRUNT_ATTACK_COOLDOWN = 3.0f;
        
        // === BOSS ENHANCEMENT: Phase System ===
        int bossPhase = 1;
        
        // === BOSS ENHANCEMENT: Shield Block ===
        float blockDuration = 2.0f;
        float blockCooldown = 5.0f;
        float blockTimer = 0.0f;
        bool isBlocking = false;
        float blockStateTimer = 0.0f;
        
        // === BOSS ENHANCEMENT: Strafe Movement ===
        float strafeDuration = 1.2f;
        float strafeCooldown = 2.5f;
        float strafeTimer = 0.0f;
        float strafeDirection = 1.0f;  // 1 = right, -1 = left
        bool isStrafing = false;
        float strafeStateTimer = 0.0f;
        
        // === BOSS ENHANCEMENT: Charge Attack ===
        float chargeWindup = 1.0f;
        float chargeDuration = 1.2f;
        float chargeCooldown = 8.0f;
        float chargeTimer = 0.0f;
        float chargeSpeed = 12.0f;
        bool isCharging = false;
        bool chargeWindingUp = false;
        float chargeStateTimer = 0.0f;
        glm::vec3 chargeDirection = glm::vec3(0);
        
        // Random generator for boss behaviors
        std::mt19937 rng{std::random_device{}()};

    public:
        void enter(Application* app) {
            this->app = app;
            std::string config_path = ("config/bullet.jsonc");
            std::ifstream file_in(config_path);
            if (!file_in) std::cerr << "Couldn't open file: " << config_path << std::endl;
            else bullet = nlohmann::json::parse(file_in, nullptr, true, true);
            
            bossActivated = false;
            bossBlocking = false;
            resetBossState();
        }
        
        void resetBossState() {
            bossPhase = 1;
            isBlocking = false;
            blockStateTimer = 0.0f;
            blockTimer = 0.0f;
            isStrafing = false;
            strafeStateTimer = 0.0f;
            strafeTimer = 0.0f;
            isCharging = false;
            chargeWindingUp = false;
            chargeStateTimer = 0.0f;
            chargeTimer = 0.0f;
        }

        void activateBoss() { bossActivated = true; std::cout << "=== BOSS ACTIVATED! TASKMASTER ENGAGED! ===" << std::endl; }
        bool isBossActivated() const { return bossActivated; }
        int getBossPhase() const { return bossPhase; }
        bool isBossBlocking() const { return bossBlocking; }

        void update(World* world, float deltaTime, our::PhysicsSystem* physicsSystem) {
            JPH::BodyInterface* bodyInterface = physicsSystem->getBodyInterface();

            for (auto entity : world->getEntities()) {
                
                bool isBoss = (entity->name == "taskmaster");
                bool isGrunt = (entity->name == "enemy");

                if (!isBoss && !isGrunt) continue;

                auto character = entity->getComponent<CharacterComponent>();
                auto animator = entity->getComponent<AnimatorComponent>();
                auto rb = entity->getComponent<RigidBodyComponent>();

                if (!character || !rb || rb->runtimeBodyID.IsInvalid()) continue;
                if (!character->getAlive()) {
                    if(isBoss) handleBossDeath(entity);
                    continue; 
                }

                if (isBoss && !bossActivated) {
                    setAnimation(animator, "idle");
                    bodyInterface->SetLinearVelocity(rb->runtimeBodyID, JPH::Vec3(0, bodyInterface->GetLinearVelocity(rb->runtimeBodyID).GetY(), 0));
                    continue;
                }
                
                // === BOSS ENHANCEMENT: Update phase based on health ===
                if (isBoss) {
                    updateBossPhase(character);
                    updateBossTimers(deltaTime);
                }

                glm::mat4 worldTransform = entity->getLocalToWorldMatrix();
                glm::vec3 position = glm::vec3(worldTransform[3]);
                glm::vec3 forward = glm::normalize(glm::vec3(worldTransform * glm::vec4(0, 0, 1, 0)));
                position.y += 1.5f;

                int maxAngle = isBoss ? 80 : 60; 
                int numRays = isBoss ? 32 : 12;
                RaycastHit hit = see(physicsSystem, position, forward, maxAngle, numRays);
                bool playerFound = (hit.hasHit && hit.entity->layer == "player");

                float stoppingDistance = 0.8f;
                float speedUp = 1;
                
                if (playerFound) {
                    character->setState(States::PURSUIT);
                    character->updateTarget(hit.position);
                    
                    float dist = glm::length(position - hit.position);

                    if (isBoss) {
                        // === BOSS ENHANCEMENT: Handle charge attack (highest priority) ===
                        if (isCharging || chargeWindingUp) {
                            handleBossCharge(entity, bodyInterface, rb, animator, hit.position, deltaTime);
                            continue;  // Skip normal movement during charge
                        }
                        
                        // === BOSS ENHANCEMENT: Handle blocking ===
                        if (isBlocking) {
                            bossBlocking = true;
                            setAnimation(animator, "block");
                            bodyInterface->SetLinearVelocity(rb->runtimeBodyID, JPH::Vec3(0, bodyInterface->GetLinearVelocity(rb->runtimeBodyID).GetY(), 0));
                            continue;  // Don't move or attack while blocking
                        } else {
                            bossBlocking = false;
                        }
                        
                        // === BOSS ENHANCEMENT: Decide to block (medium range) ===
                        if (dist > 4.0f && dist < 12.0f && shouldBossBlock()) {
                            startBlocking();
                            continue;
                        }
                        
                        // === BOSS ENHANCEMENT: Decide to charge (Phase 2+, good range) ===
                        if (bossPhase >= 2 && dist > 6.0f && dist < 18.0f && shouldBossCharge()) {
                            startChargeWindup(position, hit.position);
                            continue;
                        }
                        
                        // Normal boss combat with enhancements
                        if (dist < 3.0f) {
                            // Close range - attack
                            stoppingDistance = 2.5f;
                            float cooldown = getPhaseAdjustedCooldown(BOSS_ATTACK_COOLDOWN);
                            attack(world, character, animator, position, hit.position, cooldown, deltaTime, "enemy_attack");
                        } 
                        else if (dist < 8.0f) {
                            // Medium range - approach with strafing, may attack
                            stoppingDistance = 2.5f;
                            
                            // === BOSS ENHANCEMENT: Strafe while approaching ===
                            if (shouldStartStrafe()) {
                                startStrafing();
                            }
                            
                            if (isStrafing) {
                                // Move with strafe component
                                glm::vec3 toPlayer = glm::normalize(hit.position - position);
                                glm::vec3 strafeDir = glm::cross(toPlayer, glm::vec3(0, 1, 0)) * strafeDirection;
                                glm::vec3 moveDir = glm::normalize(toPlayer * 0.6f + strafeDir * 0.4f);
                                
                                float speed = 2.5f * getPhaseSpeedMultiplier();
                                JPH::Vec3 currentVel = bodyInterface->GetLinearVelocity(rb->runtimeBodyID);
                                bodyInterface->SetLinearVelocity(rb->runtimeBodyID, 
                                    JPH::Vec3(moveDir.x * speed, currentVel.GetY(), moveDir.z * speed));
                                
                                // Face player
                                float targetAngle = glm::atan(toPlayer.x, toPlayer.z);
                                entity->localTransform.rotation.y = targetAngle;
                                
                                setAnimation(animator, "walk");
                            } else {
                                setAnimation(animator, "walk");
                            }
                            
                            float cooldown = getPhaseAdjustedCooldown(BOSS_ATTACK_COOLDOWN);
                            attack(world, character, animator, position, hit.position, cooldown, deltaTime, "enemy_attack");
                        } 
                        else {
                            // Long range - run towards player
                            speedUp = 1.5f * getPhaseSpeedMultiplier();
                            setAnimation(animator, "run");
                        }
                    } 
                    else {
                        stoppingDistance = 5.0f;
                        attack(world, character, nullptr, position, hit.position, GRUNT_ATTACK_COOLDOWN, deltaTime, "enemy_attack");
                    }
                }
                else if (character->getState() == States::PURSUIT) {
                    character->setState(States::INVESTIGATION);
                    if(isBoss)
                    {
                        speedUp = 1.5;
                        setAnimation(animator, "run");
                    }
                }
                else if (character->getState() == States::INVESTIGATION) {
                    glm::vec3 target = character->getTarget();
                    target.y = position.y;
                    if (glm::length(position - target) < 0.9f) {
                        character->updateTarget();
                        character->setState(States::PATROL);
                    }
                    stoppingDistance = 0.8f;
                    if (glm::length(position - target) < 3.0f)
                        setAnimation(animator, "walk");
                }
                else if (character->getState() == States::PATROL) {
                    glm::vec3 target = character->getTarget();
                    target.y = position.y;
                    if (glm::length(position - target) < 0.9f) {
                        character->updateTarget();
                    }
                    stoppingDistance = 0.8f;
                    setAnimation(animator, "walk");
                }

                move(bodyInterface, rb, entity, position, character->getTarget(), stoppingDistance, isBoss, speedUp);
            }
        }

        static void onCollision(Entity* self, Entity* other) {
            if (other->layer == "player_attack") {
                if (other->timeRemaining <= 0) return; // Bullet already used

                CharacterComponent* character = self->getComponent<CharacterComponent>();
                if (!character || !character->getAlive()) return;

                bool isBoss = (self->name == "taskmaster");
                
                // Boss invulnerable before activation
                if (isBoss && !bossActivated) {
                    other->timeRemaining = 0;  // Destroy bullet but no damage
                    return;
                }
                
                // === BOSS ENHANCEMENT: Block check ===
                if (isBoss && bossBlocking) {
                    other->timeRemaining = 0;  // Destroy bullet
                    std::cout << ">>> BOSS BLOCKED THE ATTACK! <<<" << std::endl;
                    return;  // No damage when blocking
                }

                int damage = isBoss ? -5 : -10;
                character->setHealth(damage);
                other->timeRemaining = 0; // Destroy bullet

                std::cout << self->name << " Health: " << character->getHealth() << std::endl;

                // Death Logic
                if (character->getHealth() <= 0) {
                    character->setAlive(false);
                    if (!isBoss) {
                        self->timeRemaining = -1; // Grunts disappear instantly
                    } else {
                        // Boss stays for animation
                        std::cout << "=== BOSS DEFEATED! ===" << std::endl;
                    }
                }
            }
        }

        static void onTrigger(Entity* self, Entity* other) {
            if(other->layer == "player_attack")
            {
                // Only process damage from active projectiles that haven't been "used"
                if (other->timeRemaining <= 0) return;
                
                CharacterComponent* character = self->getComponent<CharacterComponent>();
                
                if (!character->getAlive()) return;

                glm::vec3 new_Direction = glm::normalize(glm::vec3(other->getLocalToWorldMatrix()[3]) - glm::vec3(self->getLocalToWorldMatrix()[3]));

                glm::vec3& rotation = self->localTransform.rotation;

                if(glm::length(new_Direction) > 0) 
                {
                    new_Direction = glm::normalize(new_Direction);
                    float targetAngle = glm::atan(new_Direction.x, new_Direction.z);

                    rotation.y = targetAngle; 
                }
                if(character->getState() != States::PURSUIT) {
                    character->setState(States::INVESTIGATION);
                    character->updateTarget(glm::vec3(other->getLocalToWorldMatrix()[3]));
                }

                //character->block;
            }
        }
        
        void exit() {}

    private:
        RaycastHit see(our::PhysicsSystem* physicsSystem, glm::vec3 startPos, glm::vec3 forward, int maxAngle, int numRays) {
            LayerFilter filter({Layers::ENEMY, Layers::ENEMY_ATTACK, Layers::PLAYER_ATTACK, Layers::ENEMY_AWARENESS});
            RaycastHit hit;
            hit.hasHit = false;

            for (float angle_deg = -maxAngle; angle_deg <= maxAngle; angle_deg+=(maxAngle * 2 / numRays)){
                float angle = glm::radians(angle_deg);
                glm::vec3 dir = glm::rotate(forward, angle, glm::vec3(0, 1, 0));
                
                float range = (maxAngle > 60) ? 80.0f : 40.0f; 
                
                hit = physicsSystem->Raycast(startPos, dir, range, filter);
                if (hit.hasHit && hit.entity->layer == "player") return hit;
            }
            return hit; 
        }

        void move(JPH::BodyInterface* bodyInterface, RigidBodyComponent* rb, Entity* entity, glm::vec3 pos, glm::vec3 target, float stopping_distance, bool isBoss, float speedUp) {
            float distance = glm::length(glm::abs(target - pos));

            JPH::Vec3 currentVel = bodyInterface->GetLinearVelocity(rb->runtimeBodyID);
            glm::vec3 new_Direction = glm::normalize(glm::vec3(target - pos));

            if(distance > stopping_distance) {
                float speed = 2.0f * speedUp;
        
                JPH::Vec3 newVel(
                    new_Direction.x * speed, 
                    currentVel.GetY(), // gravity
                    new_Direction.z * speed
                );

                bodyInterface->SetLinearVelocity(rb->runtimeBodyID, newVel);
            }


            glm::vec3& rotation = entity->localTransform.rotation;

            if(glm::length(new_Direction) > 0) 
            {
                new_Direction = glm::normalize(new_Direction);
                float targetAngle = glm::atan(new_Direction.x, new_Direction.z);

                rotation.y = targetAngle; 
            }
        }

        void attack(World* world, CharacterComponent* character, AnimatorComponent* animator, glm::vec3 start, glm::vec3 target, float cooldown, float dt, std::string projectileName) {
            float timer = character->getTimer();
            timer -= dt;

            if (timer <= 0) {
                glm::vec3 dir = glm::normalize(target - start);
                JPH::Vec3 impulse(dir.x, 0, dir.z);
                impulse = impulse * 100.0f; // Projectile Speed

                ObjectSpawner::spawnObject(world, nullptr, bullet, start, glm::vec3(0,0,0), glm::vec3(0.02f), impulse, 10.0f, projectileName);
                
                character->setTimer(cooldown, true);
                if(animator) setAnimation(animator, "attack");
                
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

        void handleBossDeath(Entity* entity) {
             auto animator = entity->getComponent<AnimatorComponent>();
             if (animator) setAnimation(animator, "death");
        }
        
        // ============================================
        // === BOSS ENHANCEMENT HELPER FUNCTIONS ===
        // ============================================
        
        void updateBossPhase(CharacterComponent* character) {
            int health = character->getHealth();
            
            // Phase transitions only happen ONCE when health drops below threshold
            // Phases never go backwards (even after healing)
            int requiredPhase = bossPhase;
            
            if (bossPhase == 1 && health <= 30) {
                requiredPhase = 2;  // Transition to phase 2 at 30% health
            } else if (bossPhase == 2 && health <= 30) {
                requiredPhase = 3;  // Transition to phase 3 at 30% of healed HP
            }
            
            if (requiredPhase > bossPhase) {
                bossPhase = requiredPhase;
                std::cout << "=== BOSS ENTERS PHASE " << bossPhase << "! ===" << std::endl;
                
                // Heal to full on phase change
                character->setHealth(100);
                std::cout << "=== BOSS HEALS TO FULL HP! ===" << std::endl;
                
                // Adjust difficulty based on phase
                switch (bossPhase) {
                    case 2:
                        blockCooldown = 4.0f;
                        strafeCooldown = 2.0f;
                        chargeCooldown = 6.0f;
                        break;
                    case 3:
                        blockCooldown = 3.0f;
                        strafeCooldown = 1.5f;
                        chargeCooldown = 4.0f;
                        chargeSpeed = 15.0f;
                        break;
                    default:
                        break;
                }
            }
        }
        
        void updateBossTimers(float deltaTime) {
            // Update cooldown timers
            blockTimer += deltaTime;
            strafeTimer += deltaTime;
            chargeTimer += deltaTime;
            
            // Update state timers
            if (isBlocking) {
                blockStateTimer += deltaTime;
                if (blockStateTimer >= blockDuration) {
                    isBlocking = false;
                    bossBlocking = false;
                    blockStateTimer = 0.0f;
                    blockTimer = 0.0f;
                    std::cout << "Boss stops blocking" << std::endl;
                }
            }
            
            if (isStrafing) {
                strafeStateTimer += deltaTime;
                if (strafeStateTimer >= strafeDuration) {
                    isStrafing = false;
                    strafeStateTimer = 0.0f;
                    strafeTimer = 0.0f;
                }
            }
        }
        
        float getPhaseAdjustedCooldown(float baseCooldown) {
            switch (bossPhase) {
                case 2: return baseCooldown * 0.75f;
                case 3: return baseCooldown * 0.5f;
                default: return baseCooldown;
            }
        }
        
        float getPhaseSpeedMultiplier() {
            switch (bossPhase) {
                case 2: return 1.25f;
                case 3: return 1.5f;
                default: return 1.0f;
            }
        }
        
        bool shouldBossBlock() {
            if (isBlocking || blockTimer < blockCooldown) return false;
            
            std::uniform_real_distribution<float> dist(0.0f, 1.0f);
            float blockChance = 0.15f + (bossPhase * 0.08f);  // Higher chance in later phases
            return dist(rng) < blockChance;
        }
        
        void startBlocking() {
            isBlocking = true;
            bossBlocking = true;
            blockStateTimer = 0.0f;
            std::cout << ">>> Boss raises shield! <<<" << std::endl;
        }
        
        bool shouldStartStrafe() {
            if (isStrafing || strafeTimer < strafeCooldown) return false;
            
            std::uniform_real_distribution<float> dist(0.0f, 1.0f);
            float strafeChance = 0.25f + (bossPhase * 0.1f);
            return dist(rng) < strafeChance;
        }
        
        void startStrafing() {
            isStrafing = true;
            strafeStateTimer = 0.0f;
            
            std::uniform_real_distribution<float> dist(0.0f, 1.0f);
            strafeDirection = (dist(rng) > 0.5f) ? 1.0f : -1.0f;
        }
        
        bool shouldBossCharge() {
            if (isCharging || chargeWindingUp || chargeTimer < chargeCooldown) return false;
            
            std::uniform_real_distribution<float> dist(0.0f, 1.0f);
            float chargeChance = 0.1f + (bossPhase * 0.08f);
            return dist(rng) < chargeChance;
        }
        
        void startChargeWindup(glm::vec3 bossPos, glm::vec3 playerPos) {
            chargeWindingUp = true;
            chargeStateTimer = 0.0f;
            
            // Lock in the charge direction
            chargeDirection = glm::normalize(playerPos - bossPos);
            chargeDirection.y = 0;
            
            std::cout << ">>> Boss winding up CHARGE ATTACK! <<<" << std::endl;
        }
        
        void handleBossCharge(Entity* entity, JPH::BodyInterface* bodyInterface, 
                              RigidBodyComponent* rb, AnimatorComponent* animator,
                              glm::vec3 playerPos, float deltaTime) {
            chargeStateTimer += deltaTime;
            
            if (chargeWindingUp) {
                // Windup phase - stand still, face player
                bodyInterface->SetLinearVelocity(rb->runtimeBodyID, 
                    JPH::Vec3(0, bodyInterface->GetLinearVelocity(rb->runtimeBodyID).GetY(), 0));
                
                // Face the player during windup
                glm::vec3 toPlayer = playerPos - entity->localTransform.position;
                toPlayer.y = 0;
                if (glm::length(toPlayer) > 0.001f) {
                    glm::vec3 dir = glm::normalize(toPlayer);
                    entity->localTransform.rotation.y = glm::atan(dir.x, dir.z);
                    chargeDirection = dir;  // Update charge direction
                }
                
                setAnimation(animator, "block");  // Use block animation for windup
                
                if (chargeStateTimer >= chargeWindup) {
                    // Start the actual charge
                    chargeWindingUp = false;
                    isCharging = true;
                    chargeStateTimer = 0.0f;
                    std::cout << ">>> BOSS CHARGES!!! <<<" << std::endl;
                }
            } 
            else if (isCharging) {
                // Charging phase - rush forward
                setAnimation(animator, "run");
                
                float currentSpeed = chargeSpeed * getPhaseSpeedMultiplier();
                bodyInterface->SetLinearVelocity(rb->runtimeBodyID,
                    JPH::Vec3(chargeDirection.x * currentSpeed, 
                              bodyInterface->GetLinearVelocity(rb->runtimeBodyID).GetY(), 
                              chargeDirection.z * currentSpeed));
                
                if (chargeStateTimer >= chargeDuration) {
                    // Charge ended
                    isCharging = false;
                    chargeStateTimer = 0.0f;
                    chargeTimer = 0.0f;  // Reset cooldown
                    std::cout << "Boss charge attack ended" << std::endl;
                }
            }
        }
    };
}
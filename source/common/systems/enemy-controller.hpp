#pragma once

#include <iostream>
#include <fstream>
#include <flags/flags.h>
#include <json/json.hpp>

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

        const float BOSS_ATTACK_COOLDOWN = 2.0f;
        const float GRUNT_ATTACK_COOLDOWN = 3.0f;

    public:
        void enter(Application* app) {
            this->app = app;
            std::string config_path = ("config/bullet.jsonc");
            std::ifstream file_in(config_path);
            if (!file_in) std::cerr << "Couldn't open file: " << config_path << std::endl;
            else bullet = nlohmann::json::parse(file_in, nullptr, true, true);
            
            bossActivated = false;
        }

        void activateBoss() { bossActivated = true; std::cout << "BOSS ACTIVATED!" << std::endl; }
        bool isBossActivated() const { return bossActivated; }

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
                        if (dist < 3.0f) {
                            stoppingDistance = 2.5f;
                            attack(world, character, animator, position, hit.position, BOSS_ATTACK_COOLDOWN, deltaTime, "enemy_attack");
                        } 
                        else if (dist < 8.0f) {
                            stoppingDistance = 2.5f;
                            setAnimation(animator, "walk");
                            attack(world, character, animator, position, hit.position, BOSS_ATTACK_COOLDOWN, deltaTime, "enemy_attack");
                        } 
                        else {
                            speedUp = 1.5;
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
                if (isBoss && !bossActivated) return;

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
                        std::cout << "Boss Defeated!" << std::endl;
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
                
                float range = (maxAngle > 60) ? 250.0f : 200.0f; 
                
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

                ObjectSpawner::spawnObject(world, nullptr, bullet, start, glm::vec3(0), glm::vec3(0.1f), impulse, 10.0f, projectileName);
                
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
    };
}
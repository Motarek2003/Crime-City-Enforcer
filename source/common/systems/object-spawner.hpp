#pragma once

#include "../ecs/world.hpp"
#include "../ecs/entity.hpp"
#include "../components/rigidbody.hpp"

#include <Jolt/Jolt.h>

namespace our {

    class ObjectSpawner {
        public:
            static Entity* spawnObject(World* world, Entity* parent , const nlohmann::json& objectData, glm::vec3 position, glm::vec3 rotation, glm::vec3 scale,  JPH::Vec3 impulseVector = JPH::Vec3::sZero(), float timeRemaining = NULL) {
                Entity* entity = world->add();
                if(parent != nullptr) {
                    entity->parent = parent;
                }
                entity->deserialize(objectData);
                entity->localTransform.position = position;
                entity->localTransform.rotation = rotation;
                entity->localTransform.scale = scale;
                auto rb = entity->getComponent<our::RigidBodyComponent>();
                if(rb) {    
                    rb->impulseVector = impulseVector;
                }
                entity->timeRemaining = timeRemaining;
                return entity;
            }
    };
}
#pragma once

#include "../ecs/world.hpp"
#include "../components/rigidbody.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyInterface.h>
namespace our {

    class LifetimeSystem {
    public:
        static void update(World* world, float deltaTime, JPH::BodyInterface* bodyInterface) {
            for (auto entity : world->getEntities()) {
                if(entity->timeRemaining == NULL)
                    continue;
                
                entity->timeRemaining -= deltaTime;

                if (entity->timeRemaining <= 0.0f) {
                    world->markForRemoval(entity);
                    //std::cout << "Entity " << entity->name << " has been marked for removal." << std::endl;

                    auto rb = entity->getComponent<RigidBodyComponent>();
                    if (rb && !rb->runtimeBodyID.IsInvalid()) {
                        bodyInterface->RemoveBody(rb->runtimeBodyID);
                        bodyInterface->DestroyBody(rb->runtimeBodyID);
                        rb->runtimeBodyID = JPH::BodyID();
                    }
                }
            }
        }
    };
}
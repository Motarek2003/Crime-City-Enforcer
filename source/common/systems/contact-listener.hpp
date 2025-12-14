#pragma once
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Body/Body.h>
#include "../ecs/world.hpp"
#include "../components/character.hpp"
#include <iostream>

namespace our {

    class GameContactListener : public JPH::ContactListener {

    public:

        // Called when two bodies collide
        virtual void OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override {
            
            //std::cout << "Collision Detected between Body " << inBody1.GetID().GetIndex() << " and Body " << inBody2.GetID().GetIndex() << std::endl;

            JPH::uint64 body1data = inBody1.GetUserData();
            JPH::uint64 body2data = inBody2.GetUserData();

            // Cast back to Entity*
            Entity* entity1 = reinterpret_cast<Entity*>(body1data);
            Entity* entity2 = reinterpret_cast<Entity*>(body2data);

            if(entity1 && entity2) {
                if(entity1->layer == "enemy_awareness")
                    if(auto* script = entity1->parent->getComponent<CharacterComponent>())
                        script->onTriggerEnter(entity2);

                if(entity2->layer == "enemy_awareness")
                    if(auto* script = entity2->parent->getComponent<CharacterComponent>())
                        script->onTriggerEnter(entity1);

                if (auto* script = entity1->getComponent<CharacterComponent>()) {
                    script->onCollisionEnter(entity2);
                }
                
                if (auto* script = entity2->getComponent<CharacterComponent>()) {
                    script->onCollisionEnter(entity1);
                }
            }
        }

        virtual void OnContactRemoved(const JPH::SubShapeIDPair &inSubShapePair) override {
        }
    };
}
#pragma once
#include <vector>
#include <algorithm> 

#include "../ecs/world.hpp"
#include "./contact-listener.hpp"


#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>

namespace our {

    class JoltDebugRenderer; // Forward declaration

    // LAYERS
    namespace Layers {
        static constexpr JPH::ObjectLayer NON_MOVING = 0; // Static (Walls)
        static constexpr JPH::ObjectLayer MOVING = 1;     // Dynamic (Player)
        static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
    };


    // To filter raycasts
    class LayerFilter : public JPH::ObjectLayerFilter {
        std::vector<JPH::ObjectLayer> layersToIgnore;

    public:
        // Constructor takes a list of layers: { Layers::PLAYER, Layers::SENSOR }
        LayerFilter(const std::vector<JPH::ObjectLayer> layers) {
            layersToIgnore = layers;
        }

        virtual bool ShouldCollide(JPH::ObjectLayer inLayer) const override {
            for (JPH::ObjectLayer layer : layersToIgnore) {
                if (layer == inLayer)
                    return false;
            }
            return true;
        }
    };

    struct RaycastHit {
        Entity* entity = nullptr;
        glm::vec3 position = {0,0,0};
        glm::vec3 normal = {0,1,0};
        float distance = 0.0f;
        bool hasHit = false;
    };

    class PhysicsSystem {
        // Jolt Core Objects
        JPH::PhysicsSystem* physicsSystem = nullptr;
        JPH::TempAllocatorImpl* tempAllocator = nullptr;
        JPH::JobSystemThreadPool* jobSystem = nullptr;
        
        // The interface we use to talk to bodies (Move, Create, Delete)
        JPH::BodyInterface* bodyInterface = nullptr;

        JoltDebugRenderer* debugRenderer = nullptr;
        bool debugDrawEnabled = false;

        GameContactListener* contactListener = nullptr;

        // Helpers for Jolt Configuration
        class BPLayerInterfaceImpl;
        class ObjectVsBroadPhaseLayerFilterImpl;
        class ObjectLayerPairFilterImpl;
        class ContactListenerImpl;

        BPLayerInterfaceImpl* bpLayerInterface = nullptr;
        ObjectVsBroadPhaseLayerFilterImpl* objectVsBroadPhaseLayerFilter = nullptr;
        ObjectLayerPairFilterImpl* objectLayerPairFilter = nullptr;


    public:
        void initialize();
        void cleanup();
        void update(World* world, float deltaTime);

        // Debug Drawing
        void setDebugDrawEnabled(bool enabled) { debugDrawEnabled = enabled; }
        bool isDebugDrawEnabled() const { return debugDrawEnabled; }
        JoltDebugRenderer* getDebugRenderer() { return debugRenderer; }

        RaycastHit Raycast(glm::vec3 origin, glm::vec3 direction, float maxDistance, const JPH::ObjectLayerFilter& filter = JPH::ObjectLayerFilter());

        // Helper to access Jolt from Renderer
        JPH::PhysicsSystem* getPhysicsSystem() { return physicsSystem; }
        JPH::BodyInterface* getBodyInterface() { return bodyInterface; }
    };
}
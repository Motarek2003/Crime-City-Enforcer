#pragma once

#include "../ecs/component.hpp"
#include <glm/glm.hpp>

// Jolt Headers
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

namespace our {

    enum class RigidbodyType {
        DYNAMIC,   // Moves, falls, pushed by forces (Player)
        STATIC,    // Does not move(Ground, Walls)
        KINEMATIC  // Moved by script only, pushes Dynamic objects (Moving Platform)
    };

    class RigidBodyComponent : public Component {
    public:

        static std::string getID() { return "Rigid Body"; }

        RigidbodyType type = RigidbodyType::DYNAMIC;
        
        float mass = 1.0f;
        bool useGravity = true;

        // This ID allows us to talk to the physics engine about this specific body.
        JPH::BodyID runtimeBodyID;

         JPH::Vec3 impulseVector =  JPH::Vec3::sZero();

        void deserialize(const nlohmann::json& data) override;
    };

}
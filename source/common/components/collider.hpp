#pragma once

#include "../ecs/component.hpp"
#include <glm/glm.hpp>
#include <string>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>

namespace our {

    enum class ColliderType {
        BOX,
        SPHERE,
        CAPSULE
    };

    class ColliderComponent : public Component {
    public:

        static std::string getID() { return "Collider"; }

        ColliderType type = ColliderType::BOX;
        
        // Box: x,y,z
        // Sphere: Max(x,y,z) = Radius
        // Capsule: Max(x,z) = Radius, y = Height
        glm::vec3 size = {1.0f, 1.0f, 1.0f}; 
        
        // Offset from the entity center
        glm::vec3 offset = {0.0f, 0.0f, 0.0f};

        bool isTrigger = false;

        JPH::BodyID runtimeBodyID;

        void deserialize(const nlohmann::json& data) override;
    };

}
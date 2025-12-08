#include "rigidbody.hpp"

namespace our {

    void RigidBodyComponent::deserialize(const nlohmann::json& data) {
        if(data.contains("bodyType")) {
            std::string typeStr = data.value("bodyType", "Dynamic");
            if(typeStr == "Dynamic") type = RigidbodyType::DYNAMIC;
            else if(typeStr == "Static") type = RigidbodyType::STATIC;
            else if(typeStr == "Kinematic") type = RigidbodyType::KINEMATIC;
        }

        mass = data.value("mass", 1.0f);
        useGravity = data.value("useGravity", true);
        
    }

}
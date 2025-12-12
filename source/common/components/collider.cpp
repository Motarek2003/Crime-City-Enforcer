#include "collider.hpp"

namespace our {

    void ColliderComponent::deserialize(const nlohmann::json& data) {
        if(data.contains("shape")) {
            std::string typeStr = data.value("shape", "Box");
            if(typeStr == "Box") type = ColliderType::BOX;
            else if(typeStr == "Sphere") type = ColliderType::SPHERE;
            else if(typeStr == "Capsule") type = ColliderType::CAPSULE;
            else if(typeStr == "Mesh") type = ColliderType::MESH;
        }

        if(data.contains("size")) {
            size.x = data["size"][0];
            size.y = data["size"].size() > 1 ? data["size"][1] : size.x;
            size.z = data["size"].size() > 2 ? data["size"][2] : size.x;
        }

        if(data.contains("offset")) {
            offset.x = data["offset"][0];
            offset.y = data["offset"][1];
            offset.z = data["offset"][2];
        }

        isTrigger = data.value("isTrigger", false);
        collisionMeshPath = data.value("mesh", collisionMeshPath);
    }
}
#include "light-component.hpp"
#include "../ecs/entity.hpp"


namespace our {

    void LightComponent::deserialize(const nlohmann::json& data) {
        // Parse light type
        if(data.contains("lightType")) {
            std::string typeStr = data["lightType"];
            if(typeStr == "DIRECTIONAL") {
                lightType = LightType::DIRECTIONAL;
            } else if(typeStr == "POINT") {
                lightType = LightType::POINT;
            } else if(typeStr == "SPOT") {
                lightType = LightType::SPOT;
            }
        }

        // Parse Light Properties

        // parse color
        color = data.value("color", glm::vec3(1.0f));

        // parse intensity
        intensity = data.value("intensity", 1.0f);

        // parse attenuation
        attenuation = data.value("attenuation", glm::vec3(1.0f, 0.0f, 0.0f));
        // parse inner and outer cone angles
        innerCone = data.value("innerCone", 15.0f);
        outerCone = data.value("outerCone", 30.0f);

    }


    glm ::vec3 LightComponent::getWorldPosition() const {
        // Get the world space position of the light from the entity's transform 
        // M is the local to world matrix of the entity * glm::vec4(0, 0, 0, 1) gives the world position
        auto owner = getOwner();
        auto M = owner->getLocalToWorldMatrix();
        return glm::vec3(M * glm::vec4(0, 0, 0, 1));
    }


    glm ::vec3 LightComponent::getWorldDirection() const {
        // Get the world space direction of the light from the entity's transform 
        // M is the local to world matrix of the entity * glm::vec4(0, 0, -1, 0) (-Z axis direction )gives the world direction
        auto owner = getOwner();
        auto M = owner->getLocalToWorldMatrix();
        return glm::normalize(glm::vec3(M * glm::vec4(0, 0, -1, 0)));
    }

    
}
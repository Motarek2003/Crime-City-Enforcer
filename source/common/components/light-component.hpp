#ifndef LIGHT_COMPONENT_HPP
#define LIGHT_COMPONENT_HPP

#include "source/common/components/component.hpp"
#include "source/common/ecs/entity.hpp"
#include <glm/glm.hpp>

namespace our {
    class LightComponent : public Component {
    public:
       LightType lightType = LightType::DIRECTIONAL;

       // Light properties
        glm::vec3 color = glm::vec3(1.0f);
        float intensity = 1.0f;

        // Attenuation (for point and spot lights)
        glm::vec3 attuenuation = glm::vec3(1.0f, 0.0f, 0.0f);   // constant, linear, quadratic


        // Spot light specific properties
        float innerCone = 15.0f; // in degrees
        float outerCone = 30.0f; // in degrees


        //Component ID
        static std::string getID() { return "LightComponent"; }

        // Deserialize from json
        void deserialize(const nlohmann::json& data) override ;

        // Get world space position of the light from the entity's transform
        glm::vec3 getWorldPosition() const;

        // Get world space direction of the light from the entity's transform
        glm::vec3 getWorldDirection() const;
    };
}
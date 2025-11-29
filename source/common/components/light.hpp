#pragma once

#include "../ecs/component.hpp"
#include "../ecs/entity.hpp"
#include <glm/glm.hpp>

namespace our {
    enum class LightType {
        DIRECTIONAL,
        POINT,
        SPOT
    };

    class LightComponent : public Component {
    public:
       LightType lightType = LightType::DIRECTIONAL;
       glm::vec3 direction = glm::vec3(0.0f, 0.0f, 0.0f);

       // Light properties
        glm::vec3 color = glm::vec3(0.0f,0.0f,0.0f);
        float intensity = 1.0f;

        // Attenuation (for point and spot lights)
        glm::vec3 attenuation = glm::vec3(0.0f, 0.0f, 0.0f);   // constant, linear, quadratic
        // Spot light specific properties
        float innerCone = 0.0f;
        float outerCone = 0.0f;
        //Component ID
        static std::string getID() { return "Light"; }
        // Deserialize from json
        void deserialize(const nlohmann::json& data);
    };
}
#include "light.hpp"
#include "../ecs/entity.hpp"
#include "../deserialize-utils.hpp"
#include <glm/glm.hpp>

namespace our {

    void LightComponent::deserialize(const nlohmann::json& data){
        if(!data.is_object()) return;
        std::string typeStr = data.value("lightType", "directional");
        if(typeStr == "directional") lightType = LightType::DIRECTIONAL;
        else if(typeStr == "point") lightType = LightType::POINT;
        else if(typeStr == "spot") lightType = LightType::SPOT;

        color = data.value("color", glm::vec3(1.0f));
        intensity = data.value("intensity", 1.0f);
        attenuation = data.value("attenuation", glm::vec3(0.0f, 0.0f, 1.0f));
        innerCone = data.value("innerCone", glm::radians(15.0f));
        outerCone = data.value("outerCone", glm::radians(30.0f)); 
    }

}

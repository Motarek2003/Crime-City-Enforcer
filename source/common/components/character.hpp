#pragma once

#include "../ecs/component.hpp"

#include <glm/mat4x4.hpp>

namespace our {
    // This component marks an entity as a character that can be controlled by the character controller system
    class CharacterComponent : public Component {
    public:

        // The ID of this component type is "Character"
        static std::string getID() { return "Character"; }

        // Reads camera Character from the given json object
        void deserialize(const nlohmann::json& data) {return;}

    };

}
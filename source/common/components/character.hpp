#pragma once

#include "../ecs/component.hpp"

#include <glm/mat4x4.hpp>

namespace our {
    enum class States {
        PATROL,  
        PURSUIT,  
        INVESTIGATION
    };
    // This component marks an entity as a character that can be controlled by the character controller system
    class CharacterComponent : public Component {
    public:

        // The ID of this component type is "Character"
        static std::string getID() { return "Character"; }

        void onCollisionEnter(Entity* other);

        int getHealth() { return health;}
        void setHealth(int delta) { health += delta; }

        States getState() {return state;}
        void setState(States new_state) {state = new_state;}

        int getIndex() {return index;}
        void setIndex(int new_index) {index = new_index;}

        glm::vec3 getTarget() {return target;}
        void updateTarget(glm::vec3 new_target = glm::vec3(0.0f)) {
            if(new_target == glm::vec3(0.0f)) {
                index = (index + 1) % path.size();
                target = path[index];
            }
            else
                target = new_target;

        }

        float getTimer() {return shootCooldownTimer;}
        void setTimer(float new_timer, bool reset) {
            if (reset)
                shootCooldownTimer = FIRE_RATE;
            else
                shootCooldownTimer = new_timer;
        }

        // Reads camera Character from the given json object
        void deserialize(const nlohmann::json& data);
    private:
        int health = 100;
        std::vector<glm::vec3> path;
        int index = 0;
        States state = States::PATROL;
        glm::vec3 target;
        //How many seconds between shots
        const float FIRE_RATE = 0.5f;
        float shootCooldownTimer = 0; 

    };

}
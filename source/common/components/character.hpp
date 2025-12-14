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

        void onTriggerEnter(Entity* other);

        int getHealth() { return health;}
        void setHealth(int delta) { 
            health += delta; 
            if (health < 0) health = 0;  // Prevent negative health
        }

        States getState() {return state;}
        void setState(States new_state) {state = new_state;}

        int getIndex() {return index;}
        void setIndex(int new_index) {index = new_index;}

        bool getAlive(){ return isAlive; }
        void setAlive(bool aliveStatus) { isAlive = aliveStatus; }

        glm::vec3 getTarget() {return target;}
        void updateTarget(glm::vec3 new_target = glm::vec3(0.0f)) {
            if(new_target == glm::vec3(0.0f)) {
                index = (index + 1) % path.size();
                target = path[index];
            }
            else
                target = new_target;

        }

        float getTimer(int index = 1) {
            if(index==1)
                return primaryShootCooldownTimer;
            else
                return secondaryShootCooldownTimer;
            }
        void setTimer(float new_timer, bool reset, int index = 1) {
            if(index == 1) {
                if (reset)
                    primaryShootCooldownTimer = primaryFireRate;
                else
                    primaryShootCooldownTimer = new_timer;
            } else {
                if (reset)
                    secondaryShootCooldownTimer = secondaryFireRate;
                else
                    secondaryShootCooldownTimer = new_timer;
            }
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
        const float primaryFireRate = 0.5f;
        float primaryShootCooldownTimer = 0; 
        const float secondaryFireRate = 2.0f;
        float secondaryShootCooldownTimer = 0; 
        bool isAlive = true;

    };

}
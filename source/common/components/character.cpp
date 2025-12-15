#include "character.hpp"
#include "../systems/character-controller.hpp"
#include "../systems/enemy-controller.hpp"
#include "../systems/boss-controller.hpp"

namespace glm {
    void from_json(const nlohmann::json& j, glm::vec3& v) {
        if (j.is_array() && j.size() >= 3) {
            v.x = j[0];
            v.y = j[1];
            v.z = j[2];
        }
    }
}

namespace our
{
    void CharacterComponent::onCollisionEnter(Entity* other) {
        Entity* self = this->getOwner();
        if ((self->name == "enemy" || self->name == "taskmaster")&& isAlive)
            our::EnemyControllerSystem::onCollision(self, other);
        else
            our::CharacterControllerSystem::onCollision(self, other);
    }

    void CharacterComponent::onTriggerEnter(Entity* other) {
        Entity* self = this->getOwner();
        if (self->name == "taskmaster" && isAlive)
            our::EnemyControllerSystem::onTrigger(self, other);
    }

    void CharacterComponent::deserialize(const nlohmann::json& data) {
        if(data.contains("path")){ 
            path = data["path"].get<std::vector<glm::vec3>>();
            target = path[0];
        }
    }
} 


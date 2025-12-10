#include "character.hpp"
#include "../systems/character-controller.hpp"
#include "../systems/npc-controller.hpp"


namespace our
{
    void CharacterComponent::onCollisionEnter(Entity* other) {
        Entity* self = this->getOwner();
        if (self->name == "enemy")
            our::NPCControllerSystem::onCollision(self, other);
        else
            our::CharacterControllerSystem::onCollision(self, other);
    }
} 


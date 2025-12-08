#include "character.hpp"
#include "../systems/character-controller.hpp"


namespace our
{
    void CharacterComponent::onCollisionEnter(Entity* other) {
        Entity* self = this->getOwner();
        our::CharacterControllerSystem::onCollision(self, other);
    }
} 


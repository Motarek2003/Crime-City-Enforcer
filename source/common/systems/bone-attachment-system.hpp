#pragma once

#include "../ecs/world.hpp"
#include "../components/bone-attachment.hpp"
#include "../components/animator.hpp"
#include <glm/glm.hpp>

namespace our {

    // BoneAttachmentSystem updates entities with BoneAttachment components
    // to follow bones on their parent's animator
    class BoneAttachmentSystem {
    public:
        void update(World* world, float deltaTime) {
            for (auto entity : world->getEntities()) {
                auto attachment = entity->getComponent<BoneAttachmentComponent>();
                if (!attachment || attachment->boneName.empty()) continue;

                // Find parent with animator
                Entity* parent = entity->parent;
                if (!parent) continue;

                auto animator = parent->getComponent<AnimatorComponent>();
                if (!animator || !animator->enabled) continue;

                // Get bone transform
                glm::mat4 boneTransform;
                if (!animator->getBoneTransform(attachment->boneName, boneTransform)) {
                    continue;  // Bone not found
                }

                // Capture the entity's base rotation on first update
                // This preserves the rotation set in the config file
                if (!attachment->baseRotationInitialized) {
                    attachment->baseRotation = entity->localTransform.rotation;
                    attachment->baseRotationInitialized = true;
                }

                // Apply bone transform to entity's local transform
                // The bone transform is in the parent's model space
                glm::vec3 bonePosition = glm::vec3(boneTransform[3]);

                // Get the effective offsets based on current animation
                glm::vec3 posOffset, rotOffset;
                attachment->getOffsetsForAnimation(animator->getCurrentAnimationName(), posOffset, rotOffset);

                // Calculate final position: bone position + position offset
                glm::vec3 finalPosition = bonePosition + posOffset;

                // Apply rotation: base rotation (from config) + rotOffset (per-animation adjustment)
                glm::vec3 finalRotation = attachment->baseRotation + glm::radians(rotOffset);

                // Update entity transform
                entity->localTransform.position = finalPosition;
                entity->localTransform.rotation = finalRotation;
                entity->localTransform.scale = attachment->scaleMultiplier;
            }
        }
    };

}

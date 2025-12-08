#pragma once

#include "../ecs/world.hpp"
#include "../components/animator.hpp"
#include "../components/mesh-renderer.hpp"
#include "../mesh/mesh-utils.hpp"
#include <iostream>

namespace our {

    // AnimationSystem updates all animators in the world each frame
    class AnimationSystem {
    private:
        bool initialized = false;
        
    public:
        // Called each frame to update all animations
        void update(World* world, float deltaTime) {
            // First frame: initialize animators and swap meshes
            if (!initialized) {
                for (auto entity : world->getEntities()) {
                    auto animator = entity->getComponent<AnimatorComponent>();
                    if (animator && animator->enabled && !animator->initialized) {
                        // Initialize the animator (loads skeletal mesh + animation)
                        animator->initialize();
                        
                        // If animator has a skeletal mesh, update the mesh renderer to use it
                        if (animator->getSkeletalMesh()) {
                            auto meshRenderer = entity->getComponent<MeshRendererComponent>();
                            if (meshRenderer) {
                                meshRenderer->mesh = animator->getSkeletalMesh();
                                std::cout << "AnimationSystem: Swapped mesh renderer to use skeletal mesh" << std::endl;
                            }
                        }
                    }
                }
                initialized = true;
            }
            
            // Update all active animators
            for (auto entity : world->getEntities()) {
                auto animator = entity->getComponent<AnimatorComponent>();
                if (animator && animator->enabled && animator->currentAnimation) {
                    animator->update(deltaTime);
                }
            }
        }

        // Reset initialization state (call when changing scenes)
        void reset() {
            initialized = false;
        }
    };

}

#pragma once

#include "../ecs/component.hpp"
#include "../animation/animation.hpp"
#include "../animation/animator.hpp"
#include "../mesh/mesh.hpp"
#include <string>
#include <map>

namespace our {

    // AnimatorComponent - holds animation data and animator for skeletal mesh animation
    class AnimatorComponent : public Component {
    public:
        Animator animator;
        std::map<std::string, BoneInfo> boneInfoMap;
        std::map<std::string, Animation*> animations;  // Multiple animations by name
        Animation* currentAnimation = nullptr;
        std::string currentAnimationName;
        Mesh* skeletalMesh = nullptr;  // The skeletal mesh loaded with bone data
        std::string meshPath;  // Path to skeletal mesh file
        std::map<std::string, std::string> animationPaths;  // name -> path mapping
        bool enabled = true;
        bool initialized = false;

        static std::string getID() { return "Animator"; }

        void deserialize(const nlohmann::json& data) override;
        
        // Initialize - loads skeletal mesh and animations with shared bone info
        void initialize();

        // Get the final bone matrices for shader upload
        const std::vector<glm::mat4>& getBoneMatrices() const {
            return animator.getFinalBoneMatrices();
        }

        // Get the skeletal mesh (use this instead of asset loader mesh for animated entities)
        Mesh* getSkeletalMesh() const { return skeletalMesh; }

        // Switch to a different animation by name
        void setAnimation(const std::string& name) {
            if (animations.find(name) != animations.end() && name != currentAnimationName) {
                currentAnimation = animations[name];
                currentAnimationName = name;
                animator.playAnimation(currentAnimation);
            }
        }

        // Check if an animation exists
        bool hasAnimation(const std::string& name) const {
            return animations.find(name) != animations.end();
        }

        // Get current animation name
        const std::string& getCurrentAnimationName() const { return currentAnimationName; }

        // Get global transform of a bone for attaching objects
        bool getBoneTransform(const std::string& boneName, glm::mat4& outTransform) const {
            return animator.getBoneGlobalTransform(boneName, outTransform);
        }

        // Get all available bone names
        std::vector<std::string> getBoneNames() const {
            return animator.getBoneNames();
        }

        // Play the current animation
        void play() { animator.play(); }
        
        // Stop the animation
        void stop() { animator.stop(); }
        
        // Reset to the beginning
        void reset() { animator.reset(); }
        
        // Check if current animation has finished one cycle
        bool isAnimationFinished() const { return animator.isAnimationFinished(); }
        
        // Get animation progress (0.0 to 1.0)
        float getAnimationProgress() const { return animator.getAnimationProgress(); }

        // Update animation with delta time
        void update(float deltaTime) {
            animator.updateAnimation(deltaTime);
        }
        
        ~AnimatorComponent() {
            for (auto& pair : animations) {
                delete pair.second;
            }
            animations.clear();
            currentAnimation = nullptr;
            
            if (skeletalMesh) {
                delete skeletalMesh;
                skeletalMesh = nullptr;
            }
        }
    };

}

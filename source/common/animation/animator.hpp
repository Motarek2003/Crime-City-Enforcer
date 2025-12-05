#pragma once

#include "animation.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <map>
#include <string>

namespace our {

    // Maximum number of bones supported
    const int MAX_BONES = 100;

    // Animator class - controls animation playback and calculates bone transforms
    class Animator {
    private:
        std::vector<glm::mat4> finalBoneMatrices;
        std::map<std::string, glm::mat4> boneGlobalTransforms;  // For bone attachments
        Animation* currentAnimation;
        float currentTime;
        float deltaTime;
        bool playing;

    public:
        Animator(Animation* animation = nullptr) 
            : currentAnimation(animation), currentTime(0.0f), deltaTime(0.0f), playing(false) {
            finalBoneMatrices.resize(MAX_BONES, glm::mat4(1.0f));
        }

        void updateAnimation(float dt) {
            deltaTime = dt;
            if (currentAnimation && playing) {
                currentTime += currentAnimation->getTicksPerSecond() * dt;
                currentTime = fmod(currentTime, currentAnimation->getDuration());
                calculateBoneTransform(&currentAnimation->getRootNode(), glm::mat4(1.0f));
            }
        }

        void playAnimation(Animation* animation) {
            currentAnimation = animation;
            currentTime = 0.0f;
            playing = false;  // Don't auto-play, wait for explicit play() call
            // Calculate initial pose at time 0
            if (currentAnimation) {
                calculateBoneTransform(&currentAnimation->getRootNode(), glm::mat4(1.0f));
            }
        }

        void stop() { playing = false; }
        void play() { playing = true; }
        void reset() { 
            currentTime = 0.0f;
            // Recalculate pose at time 0
            if (currentAnimation) {
                calculateBoneTransform(&currentAnimation->getRootNode(), glm::mat4(1.0f));
            }
        }
        
        bool isPlaying() const { return playing; }
        float getCurrentTime() const { return currentTime; }
        
        // Check if animation has completed at least one cycle (useful for non-looping attacks)
        bool isAnimationFinished() const {
            if (!currentAnimation) return true;
            // Animation is "finished" if we're near the end (within 1 frame worth of time)
            float duration = currentAnimation->getDuration();
            float ticksPerSecond = currentAnimation->getTicksPerSecond();
            float threshold = ticksPerSecond * 0.016f; // ~1 frame at 60fps
            return currentTime >= (duration - threshold);
        }
        
        float getAnimationProgress() const {
            if (!currentAnimation) return 1.0f;
            return currentTime / currentAnimation->getDuration();
        }
        
        const std::vector<glm::mat4>& getFinalBoneMatrices() const { 
            return finalBoneMatrices; 
        }

        // Get the global transform of a bone by name (for attaching objects to bones)
        bool getBoneGlobalTransform(const std::string& boneName, glm::mat4& outTransform) const {
            auto it = boneGlobalTransforms.find(boneName);
            if (it != boneGlobalTransforms.end()) {
                outTransform = it->second;
                return true;
            }
            return false;
        }

        // Get all bone names (useful for debugging)
        std::vector<std::string> getBoneNames() const {
            std::vector<std::string> names;
            for (const auto& pair : boneGlobalTransforms) {
                names.push_back(pair.first);
            }
            return names;
        }

    private:
        void calculateBoneTransform(const AssimpNodeData* node, glm::mat4 parentTransform) {
            std::string nodeName = node->name;
            glm::mat4 nodeTransform = node->transformation;

            Bone* bone = currentAnimation->findBone(nodeName);
            if (bone) {
                bone->update(currentTime);
                nodeTransform = bone->getLocalTransform();
            }

            glm::mat4 globalTransformation = parentTransform * nodeTransform;

            // Store global transform for bone attachment feature
            boneGlobalTransforms[nodeName] = globalTransformation;

            auto& boneInfoMap = currentAnimation->getBoneIdMap();
            if (boneInfoMap.find(nodeName) != boneInfoMap.end()) {
                int index = boneInfoMap.at(nodeName).id;
                glm::mat4 offset = boneInfoMap.at(nodeName).offset;
                if (index < MAX_BONES) {
                    finalBoneMatrices[index] = globalTransformation * offset;
                }
            }

            for (int i = 0; i < node->childrenCount; ++i) {
                calculateBoneTransform(&node->children[i], globalTransformation);
            }
        }
    };

} // namespace our

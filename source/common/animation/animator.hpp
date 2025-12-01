#pragma once

#include "animation.hpp"
#include <glm/glm.hpp>
#include <vector>

namespace our {

    // Maximum number of bones supported
    const int MAX_BONES = 100;

    // Animator class - controls animation playback and calculates bone transforms
    class Animator {
    private:
        std::vector<glm::mat4> finalBoneMatrices;
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
        
        const std::vector<glm::mat4>& getFinalBoneMatrices() const { 
            return finalBoneMatrices; 
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

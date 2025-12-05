#pragma once

#include "../ecs/component.hpp"
#include <glm/glm.hpp>
#include <string>
#include <map>

namespace our {

    // Per-animation offset configuration
    struct AnimationOffset {
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 rotation = glm::vec3(0.0f);  // Euler angles in degrees
    };

    // BoneAttachment component - attaches this entity to a bone on a parent's animator
    class BoneAttachmentComponent : public Component {
    public:
        std::string boneName;           // Name of the bone to attach to
        glm::vec3 positionOffset;       // Default local position offset from bone
        glm::vec3 rotationOffset;       // Default local rotation offset (euler angles in degrees)
        glm::vec3 scaleMultiplier;      // Scale multiplier
        
        // Base rotation captured from entity's initial transform (in radians)
        glm::vec3 baseRotation;         
        bool baseRotationInitialized = false;
        
        // Per-animation offsets (animation name -> offsets)
        std::map<std::string, AnimationOffset> animationOffsets;

        BoneAttachmentComponent() 
            : positionOffset(0.0f), rotationOffset(0.0f), scaleMultiplier(1.0f), baseRotation(0.0f) {}

        static std::string getID() { return "Bone Attachment"; }

        void deserialize(const nlohmann::json& data) override {
            if (!data.is_object()) return;

            if (data.contains("bone")) {
                boneName = data["bone"].get<std::string>();
            }

            if (data.contains("positionOffset")) {
                auto& arr = data["positionOffset"];
                positionOffset = glm::vec3(arr[0].get<float>(), arr[1].get<float>(), arr[2].get<float>());
            }

            if (data.contains("rotationOffset")) {
                auto& arr = data["rotationOffset"];
                rotationOffset = glm::vec3(arr[0].get<float>(), arr[1].get<float>(), arr[2].get<float>());
            }

            if (data.contains("scale")) {
                auto& arr = data["scale"];
                scaleMultiplier = glm::vec3(arr[0].get<float>(), arr[1].get<float>(), arr[2].get<float>());
            }

            // Parse per-animation offsets
            // Format: "animationOffsets": { "walk": { "position": [x,y,z], "rotation": [x,y,z] }, ... }
            if (data.contains("animationOffsets") && data["animationOffsets"].is_object()) {
                for (auto& [animName, offsetData] : data["animationOffsets"].items()) {
                    AnimationOffset offset;
                    if (offsetData.contains("position")) {
                        auto& arr = offsetData["position"];
                        offset.position = glm::vec3(arr[0].get<float>(), arr[1].get<float>(), arr[2].get<float>());
                    }
                    if (offsetData.contains("rotation")) {
                        auto& arr = offsetData["rotation"];
                        offset.rotation = glm::vec3(arr[0].get<float>(), arr[1].get<float>(), arr[2].get<float>());
                    }
                    animationOffsets[animName] = offset;
                }
            }
        }

        // Get the effective offsets for a given animation
        void getOffsetsForAnimation(const std::string& animName, glm::vec3& outPos, glm::vec3& outRot) const {
            outPos = positionOffset;
            outRot = rotationOffset;
            
            auto it = animationOffsets.find(animName);
            if (it != animationOffsets.end()) {
                outPos += it->second.position;
                outRot += it->second.rotation;
            }
        }
    };

}

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vector>
#include <string>
#include <assimp/scene.h>

namespace our {

    // Represents a single keyframe for position
    struct KeyPosition {
        glm::vec3 position;
        float timeStamp;
    };

    // Represents a single keyframe for rotation
    struct KeyRotation {
        glm::quat orientation;
        float timeStamp;
    };

    // Represents a single keyframe for scale
    struct KeyScale {
        glm::vec3 scale;
        float timeStamp;
    };

    // Bone class - stores animation keyframes and interpolates between them
    class Bone {
    private:
        std::vector<KeyPosition> positions;
        std::vector<KeyRotation> rotations;
        std::vector<KeyScale> scales;
        int numPositions;
        int numRotations;
        int numScales;

        std::string name;
        int id;
        glm::mat4 localTransform;

    public:
        Bone(const std::string& name, int id, const aiNodeAnim* channel)
            : name(name), id(id), localTransform(1.0f) {
            
            numPositions = channel->mNumPositionKeys;
            for (int i = 0; i < numPositions; ++i) {
                aiVector3D aiPos = channel->mPositionKeys[i].mValue;
                float timeStamp = (float)channel->mPositionKeys[i].mTime;
                KeyPosition data;
                data.position = glm::vec3(aiPos.x, aiPos.y, aiPos.z);
                data.timeStamp = timeStamp;
                positions.push_back(data);
            }

            numRotations = channel->mNumRotationKeys;
            for (int i = 0; i < numRotations; ++i) {
                aiQuaternion aiQuat = channel->mRotationKeys[i].mValue;
                float timeStamp = (float)channel->mRotationKeys[i].mTime;
                KeyRotation data;
                data.orientation = glm::quat(aiQuat.w, aiQuat.x, aiQuat.y, aiQuat.z);
                data.timeStamp = timeStamp;
                rotations.push_back(data);
            }

            numScales = channel->mNumScalingKeys;
            for (int i = 0; i < numScales; ++i) {
                aiVector3D aiScale = channel->mScalingKeys[i].mValue;
                float timeStamp = (float)channel->mScalingKeys[i].mTime;
                KeyScale data;
                data.scale = glm::vec3(aiScale.x, aiScale.y, aiScale.z);
                data.timeStamp = timeStamp;
                scales.push_back(data);
            }
        }

        // Update bone's local transform based on current animation time
        void update(float animationTime) {
            glm::mat4 translation = interpolatePosition(animationTime);
            glm::mat4 rotation = interpolateRotation(animationTime);
            glm::mat4 scale = interpolateScaling(animationTime);
            localTransform = translation * rotation * scale;
        }

        const glm::mat4& getLocalTransform() const { return localTransform; }
        const std::string& getBoneName() const { return name; }
        int getBoneId() const { return id; }

        // Get index of keyframe to interpolate from
        int getPositionIndex(float animationTime) const {
            for (int i = 0; i < numPositions - 1; ++i) {
                if (animationTime < positions[i + 1].timeStamp)
                    return i;
            }
            return numPositions - 1;
        }

        int getRotationIndex(float animationTime) const {
            for (int i = 0; i < numRotations - 1; ++i) {
                if (animationTime < rotations[i + 1].timeStamp)
                    return i;
            }
            return numRotations - 1;
        }

        int getScaleIndex(float animationTime) const {
            for (int i = 0; i < numScales - 1; ++i) {
                if (animationTime < scales[i + 1].timeStamp)
                    return i;
            }
            return numScales - 1;
        }

    private:
        // Get interpolation factor between two keyframes
        float getScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime) const {
            float midWayLength = animationTime - lastTimeStamp;
            float framesDiff = nextTimeStamp - lastTimeStamp;
            return midWayLength / framesDiff;
        }

        glm::mat4 interpolatePosition(float animationTime) {
            if (numPositions == 1)
                return glm::translate(glm::mat4(1.0f), positions[0].position);

            int p0Index = getPositionIndex(animationTime);
            int p1Index = p0Index + 1;
            if (p1Index >= numPositions) p1Index = p0Index;

            float scaleFactor = getScaleFactor(
                positions[p0Index].timeStamp,
                positions[p1Index].timeStamp,
                animationTime);
            glm::vec3 finalPosition = glm::mix(
                positions[p0Index].position,
                positions[p1Index].position,
                scaleFactor);
            return glm::translate(glm::mat4(1.0f), finalPosition);
        }

        glm::mat4 interpolateRotation(float animationTime) {
            if (numRotations == 1) {
                auto rotation = glm::normalize(rotations[0].orientation);
                return glm::toMat4(rotation);
            }

            int p0Index = getRotationIndex(animationTime);
            int p1Index = p0Index + 1;
            if (p1Index >= numRotations) p1Index = p0Index;

            float scaleFactor = getScaleFactor(
                rotations[p0Index].timeStamp,
                rotations[p1Index].timeStamp,
                animationTime);
            glm::quat finalRotation = glm::slerp(
                rotations[p0Index].orientation,
                rotations[p1Index].orientation,
                scaleFactor);
            finalRotation = glm::normalize(finalRotation);
            return glm::toMat4(finalRotation);
        }

        glm::mat4 interpolateScaling(float animationTime) {
            if (numScales == 1)
                return glm::scale(glm::mat4(1.0f), scales[0].scale);

            int p0Index = getScaleIndex(animationTime);
            int p1Index = p0Index + 1;
            if (p1Index >= numScales) p1Index = p0Index;

            float scaleFactor = getScaleFactor(
                scales[p0Index].timeStamp,
                scales[p1Index].timeStamp,
                animationTime);
            glm::vec3 finalScale = glm::mix(
                scales[p0Index].scale,
                scales[p1Index].scale,
                scaleFactor);
            return glm::scale(glm::mat4(1.0f), finalScale);
        }
    };

} // namespace our

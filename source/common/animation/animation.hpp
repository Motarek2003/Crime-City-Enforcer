#pragma once

#include "bone.hpp"
#include <glm/glm.hpp>
#include <map>
#include <vector>
#include <string>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <functional>

namespace our {

    // Information about each bone for the skeleton
    struct BoneInfo {
        int id;                 // Index in final bone matrices array
        glm::mat4 offset;       // Transform from mesh space to bone space
    };

    // Node in the bone hierarchy
    struct AssimpNodeData {
        glm::mat4 transformation;
        std::string name;
        int childrenCount;
        std::vector<AssimpNodeData> children;
    };

    // Animation class - stores the animation data loaded from file
    class Animation {
    private:
        float duration;
        float ticksPerSecond;
        std::vector<Bone> bones;
        AssimpNodeData rootNode;
        std::map<std::string, BoneInfo> boneInfoMap;

    public:
        Animation() = default;

        Animation(const std::string& animationPath, std::map<std::string, BoneInfo>& boneInfoMap)
            : boneInfoMap(boneInfoMap) {
            
            Assimp::Importer importer;
            const aiScene* scene = importer.ReadFile(animationPath, 
                aiProcess_Triangulate);
            
            if (!scene || !scene->mRootNode) {
                return;
            }

            if (scene->mNumAnimations == 0) {
                return;
            }

            auto animation = scene->mAnimations[0];
            duration = (float)animation->mDuration;
            ticksPerSecond = (float)animation->mTicksPerSecond;
            if (ticksPerSecond == 0.0f) {
                ticksPerSecond = 25.0f;
            }

            readHierarchyData(rootNode, scene->mRootNode);
            readMissingBones(animation, boneInfoMap);
        }

        ~Animation() = default;

        Bone* findBone(const std::string& name) {
            auto iter = std::find_if(bones.begin(), bones.end(),
                [&](const Bone& bone) {
                    return bone.getBoneName() == name;
                });
            if (iter == bones.end()) return nullptr;
            return &(*iter);
        }

        float getTicksPerSecond() const { return ticksPerSecond; }
        float getDuration() const { return duration; }
        const AssimpNodeData& getRootNode() const { return rootNode; }
        const std::map<std::string, BoneInfo>& getBoneIdMap() const { return boneInfoMap; }

    private:
        void readMissingBones(const aiAnimation* animation, std::map<std::string, BoneInfo>& boneInfoMap) {
            int size = animation->mNumChannels;

            // Get the current bone count from existing bone info
            int boneCount = 0;
            for (auto& pair : boneInfoMap) {
                if (pair.second.id >= boneCount) {
                    boneCount = pair.second.id + 1;
                }
            }

            // Reading channels (bones engaged in animation and their keyframes)
            for (int i = 0; i < size; ++i) {
                auto channel = animation->mChannels[i];
                std::string boneName = channel->mNodeName.data;

                if (boneInfoMap.find(boneName) == boneInfoMap.end()) {
                    boneInfoMap[boneName].id = boneCount;
                    boneInfoMap[boneName].offset = glm::mat4(1.0f);
                    boneCount++;
                }
                bones.push_back(Bone(boneName, boneInfoMap[boneName].id, channel));
            }

            this->boneInfoMap = boneInfoMap;
        }

        void readHierarchyData(AssimpNodeData& dest, const aiNode* src) {
            dest.name = src->mName.data;
            dest.transformation = convertMatrix(src->mTransformation);
            dest.childrenCount = src->mNumChildren;

            for (unsigned int i = 0; i < src->mNumChildren; ++i) {
                AssimpNodeData newData;
                readHierarchyData(newData, src->mChildren[i]);
                dest.children.push_back(newData);
            }
        }

        static glm::mat4 convertMatrix(const aiMatrix4x4& from) {
            glm::mat4 to;
            to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
            to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
            to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
            to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
            return to;
        }
    };

} // namespace our

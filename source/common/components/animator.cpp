#include "animator.hpp"
#include "../asset-loader.hpp"
#include "../mesh/mesh-utils.hpp"
#include <iostream>

namespace our {

    void AnimatorComponent::deserialize(const nlohmann::json& data) {
        if (!data.is_object()) return;

        if (data.contains("mesh")) {
            meshPath = data["mesh"].get<std::string>();
        }

        // Support multiple animations: "animations": { "idle": "path/to/idle.fbx", "walk": "path/to/walk.fbx" }
        if (data.contains("animations") && data["animations"].is_object()) {
            for (auto& [name, path] : data["animations"].items()) {
                animationPaths[name] = path.get<std::string>();
            }
        }
        
        // Also support single animation for backwards compatibility
        if (data.contains("animation")) {
            animationPaths["default"] = data["animation"].get<std::string>();
        }

        if (data.contains("enabled")) {
            enabled = data["enabled"].get<bool>();
        }
    }

    void AnimatorComponent::initialize() {
        if (initialized) return;
        
        // First, load the skeletal mesh - this populates boneInfoMap
        if (!meshPath.empty()) {
            skeletalMesh = mesh_utils::loadSkeletalMesh(meshPath, boneInfoMap);
            std::cout << "AnimatorComponent: Loaded skeletal mesh with " 
                      << boneInfoMap.size() << " bones" << std::endl;
        }
        
        // Load all animations using the same boneInfoMap
        if (!boneInfoMap.empty()) {
            for (auto& [name, path] : animationPaths) {
                Animation* anim = new Animation(path, boneInfoMap);
                animations[name] = anim;
                std::cout << "AnimatorComponent: Loaded animation '" << name << "': " << path << std::endl;
            }
            
            // Set default animation (prefer "idle", then "default", then first available)
            if (animations.find("idle") != animations.end()) {
                setAnimation("idle");
            } else if (animations.find("default") != animations.end()) {
                setAnimation("default");
            } else if (!animations.empty()) {
                setAnimation(animations.begin()->first);
            }
        }
        
        initialized = true;
    }

}

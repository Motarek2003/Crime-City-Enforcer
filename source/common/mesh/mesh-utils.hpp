#pragma once

#include "mesh.hpp"
#include "../animation/animation.hpp"
#include <string>
#include <map>

namespace our::mesh_utils {
    // Load an ".obj" file into the mesh
    Mesh* loadOBJ(const std::string& filename);
    // Load a mesh using Assimp (supports OBJ, FBX, and other formats)
    Mesh* loadMesh(const std::string& filename);
    // Load a skeletal mesh with bone information
    Mesh* loadSkeletalMesh(const std::string& filename, std::map<std::string, BoneInfo>& boneInfoMap);
    // Create a sphere (the vertex order in the triangles are CCW from the outside)
    // Segments define the number of divisions on the both the latitude and the longitude
    Mesh* sphere(const glm::ivec2& segments);
}
#include "mesh-utils.hpp"

// We will use "Tiny OBJ Loader" to read and process '.obj" files
#define TINYOBJLOADER_IMPLEMENTATION
#include <tinyobj/tiny_obj_loader.h>

// Assimp for loading FBX and other formats
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>
#include <vector>
#include <unordered_map>
#include <map>

// Helper function to convert Assimp types to GLM
static glm::vec3 toGlm(const aiVector3D& v) {
    return glm::vec3(v.x, v.y, v.z);
}

static glm::vec2 toGlm2(const aiVector3D& v) {
    return glm::vec2(v.x, v.y);
}

static glm::mat4 convertMatrix(const aiMatrix4x4& from) {
    glm::mat4 to;
    to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
    to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
    to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
    to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
    return to;
}

our::Mesh* our::mesh_utils::loadMesh(const std::string& filename) {
    Assimp::Importer importer;
    
    const aiScene* scene = importer.ReadFile(filename,
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_PreTransformVertices  // Bake all node transforms into vertices
    );
    
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return nullptr;
    }
    
    std::vector<our::Vertex> vertices;
    std::vector<GLuint> elements;
    
    // Process all meshes in the scene
    for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
        aiMesh* mesh = scene->mMeshes[m];
        unsigned int baseVertex = vertices.size();
        
        // Process vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            our::Vertex vertex;
            
            vertex.position = toGlm(mesh->mVertices[i]);
            
            if (mesh->HasNormals()) {
                vertex.normal = toGlm(mesh->mNormals[i]);
            } else {
                vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            }
            
            if (mesh->mTextureCoords[0]) {
                vertex.tex_coord = toGlm2(mesh->mTextureCoords[0][i]);
            } else {
                vertex.tex_coord = glm::vec2(0.0f, 0.0f);
            }
            
            if (mesh->HasVertexColors(0)) {
                aiColor4D color = mesh->mColors[0][i];
                vertex.color = our::Color(
                    static_cast<uint8_t>(color.r * 255),
                    static_cast<uint8_t>(color.g * 255),
                    static_cast<uint8_t>(color.b * 255),
                    static_cast<uint8_t>(color.a * 255)
                );
            } else {
                vertex.color = our::Color(255, 255, 255, 255);
            }
            
            vertices.push_back(vertex);
        }
        
        // Process indices
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                elements.push_back(baseVertex + face.mIndices[j]);
            }
        }
    }
    
    std::cout << "Loaded mesh: " << filename << " with " << vertices.size() << " vertices and " << elements.size() << " indices" << std::endl;
    
    return new our::Mesh(vertices, elements);
}

// Helper function to set bone data on a vertex
static void setVertexBoneData(our::Vertex& vertex, int boneID, float weight) {
    for (int i = 0; i < 4; ++i) {
        if (vertex.boneIDs[i] < 0) {
            vertex.boneIDs[i] = boneID;
            vertex.weights[i] = weight;
            break;
        }
    }
}

// Load a skeletal mesh with bone information for animation
our::Mesh* our::mesh_utils::loadSkeletalMesh(const std::string& filename, std::map<std::string, our::BoneInfo>& boneInfoMap) {
    Assimp::Importer importer;
    
    // NOTE: We do NOT use aiProcess_PreTransformVertices here because it would 
    // bake transforms and lose the bone hierarchy
    const aiScene* scene = importer.ReadFile(filename,
        aiProcess_Triangulate |
        aiProcess_GenNormals |
        aiProcess_JoinIdenticalVertices
    );
    
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return nullptr;
    }
    
    std::vector<our::Vertex> vertices;
    std::vector<GLuint> elements;
    int boneCounter = 0;
    
    // Process all meshes in the scene
    for (unsigned int m = 0; m < scene->mNumMeshes; m++) {
        aiMesh* mesh = scene->mMeshes[m];
        unsigned int baseVertex = vertices.size();
        
        // Process vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            our::Vertex vertex;
            
            vertex.position = toGlm(mesh->mVertices[i]);
            
            if (mesh->HasNormals()) {
                vertex.normal = toGlm(mesh->mNormals[i]);
            } else {
                vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            }
            
            if (mesh->mTextureCoords[0]) {
                vertex.tex_coord = toGlm2(mesh->mTextureCoords[0][i]);
            } else {
                vertex.tex_coord = glm::vec2(0.0f, 0.0f);
            }
            
            if (mesh->HasVertexColors(0)) {
                aiColor4D color = mesh->mColors[0][i];
                vertex.color = our::Color(
                    static_cast<uint8_t>(color.r * 255),
                    static_cast<uint8_t>(color.g * 255),
                    static_cast<uint8_t>(color.b * 255),
                    static_cast<uint8_t>(color.a * 255)
                );
            } else {
                vertex.color = our::Color(255, 255, 255, 255);
            }
            
            // Initialize bone data to default
            vertex.boneIDs = glm::ivec4(-1);
            vertex.weights = glm::vec4(0.0f);
            
            vertices.push_back(vertex);
        }
        
        // Process bones
        for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
            int boneID = -1;
            std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();
            
            if (boneInfoMap.find(boneName) == boneInfoMap.end()) {
                our::BoneInfo newBoneInfo;
                newBoneInfo.id = boneCounter;
                newBoneInfo.offset = convertMatrix(mesh->mBones[boneIndex]->mOffsetMatrix);
                boneInfoMap[boneName] = newBoneInfo;
                boneID = boneCounter;
                boneCounter++;
            } else {
                boneID = boneInfoMap[boneName].id;
            }
            
            auto weights = mesh->mBones[boneIndex]->mWeights;
            int numWeights = mesh->mBones[boneIndex]->mNumWeights;
            
            for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex) {
                int vertexId = weights[weightIndex].mVertexId + baseVertex;
                float weight = weights[weightIndex].mWeight;
                if (vertexId < vertices.size()) {
                    setVertexBoneData(vertices[vertexId], boneID, weight);
                }
            }
        }
        
        // Process indices
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                elements.push_back(baseVertex + face.mIndices[j]);
            }
        }
    }
    
    std::cout << "Loaded skeletal mesh: " << filename << " with " << vertices.size() 
              << " vertices, " << elements.size() << " indices, and " 
              << boneCounter << " bones" << std::endl;
    
    return new our::Mesh(vertices, elements);
}

our::Mesh* our::mesh_utils::loadOBJ(const std::string& filename) {

    // The data that we will use to initialize our mesh
    std::vector<our::Vertex> vertices;
    std::vector<GLuint> elements;

    // Since the OBJ can have duplicated vertices, we make them unique using this map
    // The key is the vertex, the value is its index in the vector "vertices".
    // That index will be used to populate the "elements" vector.
    std::unordered_map<our::Vertex, GLuint> vertex_map;

    // The data loaded by Tiny OBJ Loader
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str())) {
        std::cerr << "Failed to load obj file \"" << filename << "\" due to error: " << err << std::endl;
        return nullptr;
    }
    if (!warn.empty()) {
        std::cout << "WARN while loading obj file \"" << filename << "\": " << warn << std::endl;
    }

    // An obj file can have multiple shapes where each shape can have its own material
    // Ideally, we would load each shape into a separate mesh or store the start and end of it in the element buffer to be able to draw each shape separately
    // But we ignored this fact since we don't plan to use multiple materials in the examples
    for (const auto &shape : shapes) {
        for (const auto &index : shape.mesh.indices) {
            Vertex vertex = {};

            // Read the data for a vertex from the "attrib" object
            vertex.position = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
            };

            vertex.tex_coord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    attrib.texcoords[2 * index.texcoord_index + 1]
            };


            vertex.color = {
                    attrib.colors[3 * index.vertex_index + 0] * 255,
                    attrib.colors[3 * index.vertex_index + 1] * 255,
                    attrib.colors[3 * index.vertex_index + 2] * 255,
                    255
            };

            // See if we already stored a similar vertex
            auto it = vertex_map.find(vertex);
            if (it == vertex_map.end()) {
                // if no, add it to the vertices and record its index
                auto new_vertex_index = static_cast<GLuint>(vertices.size());
                vertex_map[vertex] = new_vertex_index;
                elements.push_back(new_vertex_index);
                vertices.push_back(vertex);
            } else {
                // if yes, just add its index in the elements vector
                elements.push_back(it->second);
            }
        }
    }

    return new our::Mesh(vertices, elements);
}

// Create a sphere (the vertex order in the triangles are CCW from the outside)
// Segments define the number of divisions on the both the latitude and the longitude
our::Mesh* our::mesh_utils::sphere(const glm::ivec2& segments){
    std::vector<our::Vertex> vertices;
    std::vector<GLuint> elements;

    // We populate the sphere vertices by looping over its longitude and latitude
    for(int lat = 0; lat <= segments.y; lat++){
        float v = (float)lat / segments.y;
        float pitch = v * glm::pi<float>() - glm::half_pi<float>();
        float cos = glm::cos(pitch), sin = glm::sin(pitch);
        for(int lng = 0; lng <= segments.x; lng++){
            float u = (float)lng/segments.x;
            float yaw = u * glm::two_pi<float>();
            glm::vec3 normal = {cos * glm::cos(yaw), sin, cos * glm::sin(yaw)};
            glm::vec3 position = normal;
            glm::vec2 tex_coords = glm::vec2(u, v);
            our::Color color = our::Color(255, 255, 255, 255);
            vertices.push_back({position, color, tex_coords, normal});
        }
    }

    for(int lat = 1; lat <= segments.y; lat++){
        int start = lat*(segments.x+1);
        for(int lng = 1; lng <= segments.x; lng++){
            int prev_lng = lng-1;
            elements.push_back(lng + start);
            elements.push_back(lng + start - segments.x - 1);
            elements.push_back(prev_lng + start - segments.x - 1);
            elements.push_back(prev_lng + start - segments.x - 1);
            elements.push_back(prev_lng + start);
            elements.push_back(lng + start);
        }
    }

    return new our::Mesh(vertices, elements);
}
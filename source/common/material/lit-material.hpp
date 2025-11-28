#ifndef LIT_MATERIAL_HPP
#define LIT_MATERIAL_HPP

#include "material.hpp"
#include "../texture/texture2d.hpp"
#include "../texture/sampler.hpp"

namespace our {
public:
    // Physically Based Rendering (PBR) textures
    Texture2D* albedo = nullptr;              // Base color texture
    Texture2D* normal = nullptr;              // Normal map texture
    Texture2D* specular = nullptr;            // Specular map texture
    Texture2D* roughness = nullptr;           // Roughness map texture
    Texture2D* emission = nullptr;            // Emission map texture
    Texture2D* ambientOcculsion = nullptr;   // Ambient occlusion map texture


    // Samplers for each texture 
    Sampler* albedoSampler = nullptr;
    Sampler* normalSampler = nullptr;
    Sampler* specularSampler = nullptr;
    Sampler* roughnessSampler = nullptr;
    Sampler* emissionSampler = nullptr;
    Sampler* ambientOcculsionSampler = nullptr;


    // Material properties
    glm ::vec3 albedoColor = glm::vec3(1.0f);      // Default albedo color
    glm ::vec3 specularColor = glm::vec3(1.0f);
    float roughnessValue = 1.0f;                   // Default roughness value
    float metallicValue = 0.0f;                    // Default metallic value
    float emissionIntensity = 0.0f;               // Default emission intensity


    // Component ID
    static std::string getID() { return "LitMaterial"; }

    // setup the material (bind textures and samplers)
    void setup() override;

    // Deserialize from json
    void deserialize(const nlohmann::json& data) override;
    

}
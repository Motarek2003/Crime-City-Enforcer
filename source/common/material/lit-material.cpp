#include "lit-material.hpp"
#include "../asset-loader.hpp"
#include "../texture/default-texture.hpp"


namespace our {
    void LitMaterial::setup() {
        // setup pipeline state for this material
        pipeline.setup();

        // bind shader
        shader.use();


        // bind albedo texture and sampler
        if(albedo && albedoSampler) {
            glActiveTexture(GL_TEXTURE0);
            albedo->bind();
            albedoSampler->bind(0);
            shader -> set("material.albedoTexture", 0);
        }
        // bind normal texture and sampler
        if(normal && normalSampler) {
            glActiveTexture(GL_TEXTURE1);
            normal->bind();
            normalSampler->bind(1);
            shader -> set("material.normalTexture", 1);
        }
        // bind specular texture and sampler
        if(specular && specularSampler) {
            glActiveTexture(GL_TEXTURE2);
            specular->bind();
            specularSampler->bind(2);
            shader -> set("material.specularTexture", 2);
        }
        // bind roughness texture and sampler
        if(roughness && roughnessSampler) {
            glActiveTexture(GL_TEXTURE3);
            roughness->bind();
            roughnessSampler->bind(3);
            shader -> set("material.roughnessTexture", 3);
        }
        // bind emission texture and sampler
        if(emission && emissionSampler) {
            glActiveTexture(GL_TEXTURE4);
            emission->bind();
            emissionSampler->bind(4);
            shader -> set("material.emissionTexture", 4);
        }
        // bind ambient occlusion texture and sampler
        if(ambientOcculsion && ambientOcculsionSampler) {
            glActiveTexture(GL_TEXTURE5);
            ambientOcculsion->bind();
            ambientOcculsionSampler->bind(5);
            shader -> set("material.ambientOcculsionTexture", 5);
        }

        // set material properties
        shader -> set("material.albedoColor", albedoColor);
        shader -> set("material.specularColor", specularColor);
        shader -> set("material.roughnessValue", roughnessValue);
        shader -> set("material.metallicValue", metallicValue);
        shader -> set("material.emissionIntensity", emissionIntensity);

    }


    void LitMaterial::deserialize(const nlohmann::json& data) {

        Material::deserialize(data);


        // Load and assign textures
        if(data.contains("albedo")) {
            albedo = AssetLoader<Texture2D>::getTexture(data["albedo"]);
        }
        if(data.contains("normal")) {
            normal = AssetLoader<Texture2D>::getTexture(data["normal"]);
        }
        if(data.contains("specular")) {
            specular = AssetLoader<Texture2D>::getTexture(data["specular"]);
        }
        if(data.contains("roughness")) {
            roughness = AssetLoader<Texture2D>::getTexture(data["roughness"]);
        }
        if(data.contains("emission")) {
            emission = AssetLoader<Texture2D>::getTexture(data["emission"]);
        }
        if(data.contains("ambientOcculsion")) {
            ambientOcculsion = AssetLoader<Texture2D>::getTexture(data["ambientOcculsion"]);
        }

        // Load and assign samplers
        if(data.contains("albedoSampler")) {
            albedoSampler = AssetLoader<Sampler>::getSampler(data["albedoSampler"]);
        }

        if(data.contains("normalSampler")) {
            normalSampler = AssetLoader<Sampler>::getSampler(data["normalSampler"]);
        }

        if(data.contains("specularSampler")) {
            specularSampler = AssetLoader<Sampler>::getSampler(data["specularSampler"]);
        }

        if(data.contains("roughnessSampler")) {
            roughnessSampler = AssetLoader<Sampler>::getSampler(data["roughnessSampler"]);
        }
        
        if(data.contains("emissionSampler")) {
            emissionSampler = AssetLoader<Sampler>::getSampler(data["emissionSampler"]);
        }

        if(data.contains("ambientOcculsionSampler")) {
            ambientOcculsionSampler = AssetLoader<Sampler>::getSampler(data["ambientOcculsionSampler"]);
        }
        

        // Parse material properties
        albedoColor = data.value("albedoColor", glm::vec3(1.0f));
        specularColor = data.value("specularColor", glm::vec3(1.0f));
        roughnessValue = data.value("roughnessValue", 1.0f);
        metallicValue = data.value("metallicValue", 0.0f);
        emissionIntensity = data.value("emissionIntensity", 0.0f);
    }
}
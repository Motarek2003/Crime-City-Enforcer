#version 330 core

// Maximum number of lights
#define MAX_LIGHTS 16

// Light types
#define DIRECTIONAL_LIGHT 0
#define POINT_LIGHT 1
#define SPOT_LIGHT 2

// Light structure
struct Light {
    int type;               // Light type
    vec3 position;          // World position (for point/spot)
    vec3 direction;         // World direction (for directional/spot)
    vec3 color;             // Light color
    float intensity;        // Light intensity
    vec3 attenuation;       // Attenuation factors (constant, linear, quadratic)
    float innerCone;        // Inner cone angle (spot lights)
    float outerCone;        // Outer cone angle (spot lights)
};

// Material structure
struct Material {
    sampler2D albedo;
    sampler2D specular;
    sampler2D roughness;
    sampler2D ao;
    sampler2D emission;
    
    vec3 albedoTint;
    float roughnessFactor;
    float metallicFactor;
    float emissionFactor;
};

// Uniforms
uniform Material material;
uniform Light lights[MAX_LIGHTS];
uniform int lightCount;
uniform vec3 ambientLight;

// Input from vertex shader
in Vertex {
    vec4 color;
    vec2 tex_coord;
    vec3 normal;
    vec3 view;
    vec3 world_position;
} fs_in;

out vec4 frag_color;

// Calculate attenuation for point/spot lights
float calculateAttenuation(vec3 attenuation, float distance) {
    return 1.0 / (attenuation.x + attenuation.y * distance + attenuation.z * distance * distance);
}

// Calculate spotlight cone effect
float calculateSpotEffect(vec3 lightDir, vec3 spotDir, float innerCone, float outerCone) {
    float angle = dot(-lightDir, spotDir);
    float innerCos = cos(radians(innerCone));
    float outerCos = cos(radians(outerCone));
    
    return smoothstep(outerCos, innerCos, angle);
}

// Blinn-Phong lighting calculation
vec3 calculateLighting(Light light, vec3 normal, vec3 view, vec3 albedo, float roughness, float specular) {
    vec3 lightDir;
    float attenuation = 1.0;
    float spotEffect = 1.0;
    
    // Calculate light direction and attenuation based on light type
    if (light.type == DIRECTIONAL_LIGHT) {
        lightDir = normalize(-light.direction);
    } 
    else if (light.type == POINT_LIGHT) {
        lightDir = normalize(light.position - fs_in.world_position);
        float distance = length(light.position - fs_in.world_position);
        attenuation = calculateAttenuation(light.attenuation, distance);
    }
    else if (light.type == SPOT_LIGHT) {
        lightDir = normalize(light.position - fs_in.world_position);
        float distance = length(light.position - fs_in.world_position);
        attenuation = calculateAttenuation(light.attenuation, distance);
        spotEffect = calculateSpotEffect(lightDir, light.direction, light.innerCone, light.outerCone);
    }
    
    // Blinn-Phong lighting calculations
    vec3 halfVector = normalize(lightDir + view);
    
    float NdotL = max(dot(normal, lightDir), 0.0);
    float NdotH = max(dot(normal, halfVector), 0.0);
    
    // Diffuse component
    vec3 diffuse = albedo * NdotL;
    
    // Specular component (Blinn-Phong)
    float shininess = (1.0 - roughness) * 128.0 + 1.0;  // Convert roughness to shininess
    vec3 specularColor = vec3(specular) * pow(NdotH, shininess);
    
    // Combine components
    vec3 result = (diffuse + specularColor) * light.color * light.intensity;


    // return results
    return result * attenuation * spotEffect;
}

void main() {
    // Sample material properties
    vec3 albedo = texture(material.albedo, fs_in.tex_coord).rgb * material.albedoTint;
    float roughness = texture(material.roughness, fs_in.tex_coord).r * material.roughnessFactor;
    float specular = texture(material.specular, fs_in.tex_coord).r;
    float ao = texture(material.ao, fs_in.tex_coord).r;
    vec3 emission = texture(material.emission, fs_in.tex_coord).rgb * material.emissionFactor;
    
    // Normalize interpolated normal
    vec3 normal = normalize(fs_in.normal);
    vec3 view = normalize(fs_in.view);
    
    // Start with ambient lighting
    vec3 color = ambientLight * albedo * ao;
    
    // Add contribution from each light
    for (int i = 0; i < lightCount && i < MAX_LIGHTS; i++) {
        color += calculateLighting(lights[i], normal, view, albedo, roughness, specular);
    }
    
    // Add emission
    color += emission;
    
    frag_color = vec4(color, 1.0);
}
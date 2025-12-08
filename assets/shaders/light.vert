#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 tex_coord;
layout(location = 3) in vec3 normal;
layout(location = 4) in ivec4 boneIDs;
layout(location = 5) in vec4 weights;

out Varyings {
    vec4 color;
    vec2 tex_coord;
    vec3 normal;
    vec3 view;
    vec3 world;
} vs_out;

const int MAX_BONES = 100;

uniform mat4 M;
uniform mat4 M_IT;
uniform mat4 VP;
uniform vec3 camera_position;

// Animation uniforms
uniform bool useAnimation;
uniform mat4 boneMatrices[MAX_BONES];

void main(){
    vec4 totalPosition = vec4(0.0);
    vec3 totalNormal = vec3(0.0);
    
    if (useAnimation) {
        // Apply bone transformations
        for (int i = 0; i < 4; i++) {
            if (boneIDs[i] < 0 || boneIDs[i] >= MAX_BONES) 
                continue;
            
            vec4 localPosition = boneMatrices[boneIDs[i]] * vec4(position, 1.0);
            totalPosition += localPosition * weights[i];
            
            vec3 localNormal = mat3(boneMatrices[boneIDs[i]]) * normal;
            totalNormal += localNormal * weights[i];
        }
        
        // If no bones affected this vertex, use original position
        if (totalPosition == vec4(0.0)) {
            totalPosition = vec4(position, 1.0);
            totalNormal = normal;
        }
    } else {
        totalPosition = vec4(position, 1.0);
        totalNormal = normal;
    }
    
    vec4 world_position = M * totalPosition;
    gl_Position = VP * world_position;
    vs_out.color = color;
    vs_out.tex_coord = tex_coord;
    vs_out.normal = normalize((M_IT * vec4(totalNormal, 0.0)).xyz);
    vs_out.view = camera_position - world_position.xyz;
    vs_out.world = world_position.xyz;
}
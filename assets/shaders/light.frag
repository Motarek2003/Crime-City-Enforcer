#version 330 core

in Varyings {
    vec4 color;
    vec2 tex_coord;
    vec3 normal;
    vec3 view;
    vec3 world;
} fs_in;

out vec4 frag_color;

struct Material {
    sampler2D albedo;
    sampler2D specular;
    sampler2D roughness;
    sampler2D ambient_occlusion;
    sampler2D emissive;


    vec3 diffuse;
    vec3 specular_color;
    vec3 ambient;
};

uniform Material material;
uniform vec4 tint;

struct Light {
    int type; // 0: Directional, 1: Point, 2: Spot
    vec3 position;
    vec3 direction;
    vec3 color;
    vec3 attenuation; // x: constant, y: linear, z: quadratic
    float inner_cone;
    float outer_cone;
};

#define MAX_LIGHTS 16
uniform Light lights[MAX_LIGHTS];
uniform int light_count;

uniform vec3 ambient_light;

void main(){
    vec3 normal = normalize(fs_in.normal);
    vec3 view = normalize(fs_in.view);
    vec3 world_pos = fs_in.world;

    vec3 material_diffuse  = material.diffuse * texture(material.albedo, fs_in.tex_coord).rgb;
    vec3 material_specular = material.specular_color * texture(material.specular, fs_in.tex_coord).rgb;
    float material_roughness =  texture(material.roughness, fs_in.tex_coord).r; // Default roughness
    
    float material_shininess = 2.0 / pow(clamp(material_roughness, 0.001, 0.999), 4.0) - 2.0;
    vec3 material_ambient = material.ambient * material_diffuse * texture(material.ambient_occlusion, fs_in.tex_coord).r; // Ambient Occlusion Map we will use only 1 Channel
    vec3 material_emissive = texture(material.emissive, fs_in.tex_coord).rgb;

    vec3 color = vec3(0.0);

    vec3 ambient = ambient_light * material_ambient;
    color += ambient + material_emissive;

    for(int i = 0; i < light_count; i++){
        Light light = lights[i];

        vec3 light_direction;
        float attenuation = 1.0;

        if(light.type == 0){ // Directional
            light_direction = normalize(-light.direction);
        } else { // Point or Spot
            vec3 light_vector = light.position - world_pos;
            float distance = length(light_vector);
            light_direction = light_vector / distance;
            attenuation = 1.0 / dot(light.attenuation, vec3(1.0, distance, distance * distance));
            
            if(light.type == 2){ // Spot

                float angle = acos(dot(-light_direction, light.direction));
                attenuation*= smoothstep(light.outer_cone, light.inner_cone, angle);
            }
        }
        // Diffuse
        float lambert = max(dot(normal, light_direction), 0.0);
        vec3 diffuse = light.color * material_diffuse * lambert * attenuation;
        color += diffuse;
        // Specular
        vec3 reflect_dir = reflect(-light_direction, normal);
        float phong = pow(max(dot(view, reflect_dir), 0.0), material_shininess);

        vec3 specular = light.color * material_specular * phong * attenuation;
        color += specular;
    }

    vec4 tex_color = texture(material.albedo, fs_in.tex_coord);
    frag_color = vec4(color, tex_color.a);
}
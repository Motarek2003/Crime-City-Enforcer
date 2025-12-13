#version 330

// The texture holding the scene pixels
uniform sampler2D tex;

// Read "assets/shaders/fullscreen.vert" to know what "tex_coord" holds;
in vec2 tex_coord;
out vec4 frag_color;

void main(){
    vec4 scene_color = texture(tex, tex_coord);

    float lum = dot(scene_color.rgb, vec3(0.33, 0.34, 0.33));

    vec3 vision_color = vec3(0.0, lum * 1.5, 0.0);

    float scanline = sin(tex_coord.y * 800.0) * 0.1; 
    vision_color -= scanline;

    vec2 ndc_coord = (tex_coord * 2.0) - 1.0;
    float dist_squared = dot(ndc_coord, ndc_coord);
    
    vision_color *= (1.0 - dist_squared * 0.8);

    frag_color = vec4(vision_color, 1.0);
}
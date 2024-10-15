#version 450
layout( set = 0, binding = 0 ) uniform sampler2D density;
layout( set = 0, binding = 1 ) uniform ub{
    vec4 color;
    mat4 proj;
    mat4 camera;
    vec4 camPos;
    uvec2 resolution;
    uvec2 resDensity;
    vec4 reflection;
    int state;
};

layout( location = 1 ) in vec2 frag_uv;
layout( location = 0 ) out vec4 frag_color;

#define PI 3.14159265

vec2 toSpherical(vec3 dir){
    float phi = acos(dir.z) ;
    float theta = atan(dir.y,dir.x);

    return vec2(theta/(2*PI),phi/PI);
}

void main() {

    vec4 d = texture(density,vec2(frag_uv.x, frag_uv.y)).xyzw;

    
    frag_color = vec4(exp(-d.xyz),1.0);
    
    

}
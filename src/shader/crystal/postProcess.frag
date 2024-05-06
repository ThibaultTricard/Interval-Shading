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

layout( set = 0, binding = 2 ) uniform sampler2D env;

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

    vec4 pos;
    pos.xy = (2.0 * gl_FragCoord.xy)/vec2(resolution) - 1;
    pos.z = d.w;
    pos.w = 1.0;
    pos = inverse(proj*camera)*pos;
    pos = pos/pos.w;

    vec3 e = vec3(1.0/float(resDensity.x),-1.0/float(resDensity.y),0.0) * 3.0;
    vec4 d10 = texture(density,vec2(frag_uv.x + e.x, frag_uv.y)).xyzw;
    vec4 p10;
    p10.xy = (2.0 * (gl_FragCoord.xy + vec2(1,0)))/vec2(resolution) - 1;
    p10.z = d10.w;
    p10.w = 1.0;
    p10 = inverse(proj*camera)*p10;
    p10 = p10/p10.w;

    vec4 d01 = texture(density,vec2(frag_uv.x, frag_uv.y + e.y)).xyzw;
    vec4 p01;
    p01.xy = (2.0 * (gl_FragCoord.xy + vec2(0,1)))/vec2(resolution) - 1;
    p01.z = d01.w;
    p01.w = 1.0;
    p01 = inverse(proj*camera)*p01;
    p01 = p01/p01.w;    
    

    vec3 N = normalize(cross( normalize(p10.xyz - pos.xyz), normalize(p01.xyz - pos.xyz)));
    

    vec3 vCam = normalize(pos.xyz - camPos.xyz);

    vec3 R  = reflect(vCam,N);

    
    if(state == 0){
        frag_color = vec4(d.x) * 2.0;
        return;
    }
    if(state == 1){
        frag_color =  vec4(exp(-d.xyz*0.01 * color.w),1.0);
        return;
    }
    if(state == 2){
        vec3 reflected  = reflection.xyz ;
        vec3 transmise =  vec3(exp(-d.x * (1-color.xyz) * color.w));
        frag_color =  vec4(reflected * reflection.w +transmise* (1-reflection.w),1.0);
        return;
    }

    vec4 background = texture(env,toSpherical(vCam));
    background.xyz = pow(background.xyz,vec3(1.0/2.2));

    vec3 reflected  = reflection.xyz * texture(env,toSpherical(R)).xyz ;
    
    vec3 transmise =  background.xyz * background.w  * exp(-d.x * (1-color.xyz) * color.w);
    if(d.w !=1.0){
        frag_color =  vec4(reflected * reflection.w +transmise* (1-reflection.w),1.0);
    }
    else{
        frag_color = background;
    }
    

}
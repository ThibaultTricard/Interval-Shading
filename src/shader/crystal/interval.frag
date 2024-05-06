#version 450

layout(location = 0) out vec4 outColor;
layout(location = 0) in vec4 A;
layout(location = 3) flat in int I;

layout( set = 0, binding = 0 ) uniform UniformBuffer {
  mat4 model;
  mat4 view;
  mat4 persp;
  float nearplane;
  int mode;
};

float hash11(float p)
{
    p = fract(p * .1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

vec3 hue2rgb(float hue)
{
    const vec3 offset = vec3(0, 2.0944, 4.1888);  
    return clamp( sin(hue + offset) + 0.5, 0.0, 1.0);
}

void main()
{

    vec4 minpoint = vec4(A.xy,1/A.z,1.0);
    vec4 maxpoint = vec4(A.xy,1/A.w,1.0);
   

    vec4 a = inverse(persp * view)*minpoint;
    vec4 b = inverse(persp * view)*maxpoint;
    a = a/a.w;
    b = b/b.w;

    float absorbtion = length(b-a);
    vec3 color = 1 - hue2rgb(hash11(float(I))*2.0*3.14159265);

    if(mode == 1){
        outColor = vec4(vec3(color),1.0/A.z);
    }
    else{
        outColor = vec4(vec3(absorbtion),1.0/A.z);
    }
    

}
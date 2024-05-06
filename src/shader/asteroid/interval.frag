#version 450


layout(location = 0) out vec4 frag_color;
layout(location = 1) in vec4 A;
layout(location = 2) in vec4 B;
layout(location = 3) flat in int I;

layout( set = 0, binding = 0 ) uniform UniformBuffer {
  mat4 view;
  mat4 persp;
  float near;
  float time;
};

layout( set = 0, binding = 3 ) buffer pos{
  mat4 models[];
};

#define MPI 3.14159265


const float EPSILON = 0.001;


const int idx = 0;

// from https://www.shadertoy.com/view/4cVGRW
#define R       vec3(800,450,0)
#define rot(a)  mat2(cos(a+vec4(0,11,33,0)))                      // rotation 

// Perlin noise adapted from https://shadertoy.com/view/wtf3R2
#define P       ( v = smoothstep(0.,1.,fract(V)), 2.*abs(mix( Pz(0), Pz(1), v.z)) )
#define Pz(z)     mix( Py(0,z), Py(1,z), v.y)
#define Py(y,z)   mix( Px(0,y,z), Px(1,y,z), v.x)
#define Px(x,y,z) dot( H( C = floor(V) + vec3(x,y,z) ), V-C )
#define H(p)    ( 2.* fract(sin( (p) * mat3(R,73.-R,R.zxy))*3758.54) -1.)

#define S .3
#define seed I

float map(vec3 V)
{
    float l =  length(V) -.6 ;             // disc
    vec3 C, v; V.y += float(seed * 11235 % 3546) + time ;
    float i=0.,s=1., p=0.;
    for ( ; i++ < 4. ; V *= 2., s *= 2. )  // Perline turbulence
        p += P/s;
        p /= 2.; p -= .274;                    // mean: .274 std: .217
    if ( seed % 2 == 1 )
        l +=  1.5* p ;                     // make peaky
    else
         l -=  0.8* p ;                     // make floffy
    return l;
}

void main()
{

    vec4 minpoint = vec4(A.xy,1.0/A.z,1.0);
    vec4 maxpoint = vec4(A.xy,1.0/A.w,1.0);
   
    mat4 model = models[I];
    
    mat4 invViewProj = inverse( persp * view * model);

    
    vec4 a = invViewProj*minpoint;
    a = a/a.w;
    vec4 b = invViewProj*maxpoint;
    b = b/b.w;

    vec4 cam = vec4(0.0,0.0,0.0,1.0);
    cam = inverse(view * model)*cam;

    
    vec3 raystart = a.xyz;

    vec3 dir = normalize(b.xyz - raystart.xyz);
    float dist = length(a.xyz - raystart);

    float max_dist = length( b.xyz - raystart.xyz);
    

    bool hitted =false;
    int i = 0;
    vec3 hit= vec3(0,0,0);
    float t=9.;
    vec3 q;
    float stop = 0.001;
    for(i = 0; i < 1000 && t > stop &&  dist < max_dist; ){
        q = dir*dist + raystart;
        t = map(q/S)*S *0.5;
        dist += t;
        i++;
    }
    
   
    if(t< stop){
      q = dir*(dist) + raystart;
      vec4 p = persp * view * model *  vec4(q,1.0);
      p = p/p.w;
      
      
      frag_color = vec4(1.0, 1.0-float(i)/50.0,0,1);
      gl_FragDepth =  p.z;
    }else{
      discard;
    }
  
}
#version 450

layout( location = 0 ) in vec4 app_position;
layout( location = 1 ) in vec2 uv;

layout( location = 1 ) out vec2 frag_uv;

void main() {
  gl_Position = app_position;
  frag_uv = uv;
}
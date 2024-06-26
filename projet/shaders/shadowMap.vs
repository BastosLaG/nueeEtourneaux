#version 330 core

uniform mat4 modelMatrix;
uniform mat4 lightViewMatrix;
uniform mat4 lightProjectionMatrix;
layout (location = 0) in vec3 vsiPosition;
layout (location = 1) in vec3 vsiNormal;
layout (location = 2) in vec2 vsiTexCoord;

void main(void) {
  gl_Position = (lightProjectionMatrix/10) * lightViewMatrix * modelMatrix * vec4(vsiPosition, 1.0);
}

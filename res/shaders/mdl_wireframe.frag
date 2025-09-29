#version 330 core

uniform sampler2D uMeshTexture;
uniform sampler2D uMatCapTexture;
uniform float     uAlphaTestReference;

in vec3  fNormal;
in float fDepth;
in vec2  fUVMesh;
in vec2  fUVMatCap;

void main() {
    // Works in light and dark mode
    gl_FragColor = vec4(vec3(0.5), 1.0);
}

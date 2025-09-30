#version 330 core

uniform sampler2D uMeshTexture;
uniform sampler2D uMatCapTexture;
uniform float     uAlphaTestReference;

in vec3  fNormal;
in float fDepth;
in vec2  fUVMesh;
in vec2  fUVMatCap;

void main() {
    vec4 texColor = texture2D(uMeshTexture, fUVMesh);
    if (texColor.a < uAlphaTestReference) {
        texColor.a = 0.0;
    }
    gl_FragColor = texColor;
}

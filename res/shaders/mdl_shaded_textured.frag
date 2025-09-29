#version 330 core

uniform sampler2D uMeshTexture;
uniform sampler2D uMatCapTexture;
uniform float     uAlphaTestReference;

in vec3  fNormal;
in float fDepth;
in vec2  fUVMesh;
in vec2  fUVMatCap;

void main() {
    vec4 texColor = texture2D(uMeshTexture, fUVMesh) * clamp(texture2D(uMatCapTexture, fUVMatCap) * 1.25 + 0.25, 0.0, 1.0);
    if (texColor.a < uAlphaTestReference) {
        discard;
    }
    gl_FragColor = texColor;
}

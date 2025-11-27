#version 330 core

uniform sampler2D uMeshTexture;
uniform sampler2D uMatCapTexture;
uniform float     uAlphaTestReference;

in vec3  fNormal;
in float fDepth;
in vec2  fUVMesh;
in vec2  fUVMatCap;

layout (location = 0) out vec4 fragColor;

void main() {
    fragColor = texture(uMeshTexture, fUVMesh) * clamp(texture(uMatCapTexture, fUVMatCap) * 1.25 + 0.25, 0.0, 1.0);
    if (fragColor.a < uAlphaTestReference) {
        discard;
    }
}

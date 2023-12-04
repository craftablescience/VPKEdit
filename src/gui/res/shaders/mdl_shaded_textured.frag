#version 150

uniform sampler2D uMeshTexture;
uniform sampler2D uMatCapTexture;

in vec3 fNormal;
in vec2 fUVMesh;
in vec2 fUVMatCap;

void main() {
    gl_FragColor = texture2D(uMeshTexture, fUVMesh) * clamp(texture2D(uMatCapTexture, fUVMatCap) * 1.5 + 0.25, 0.0, 1.0);
}

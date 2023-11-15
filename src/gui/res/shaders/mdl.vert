layout(location = 0) in vec3 vPos;
layout(location = 1) in vec3 vNormal;
layout(location = 2) in vec2 vUV;

uniform mat4 uMVP;
uniform mat4 uMV;
uniform mat3 uNormalMatrix;
uniform vec3 uEyePosition;

out vec3 fNormal;
out vec2 fUVMesh;
out vec2 fUVMatCap;

void main() {
    gl_Position = uMVP * vec4(vPos, 1.0);
    fNormal = vNormal;
    fUVMesh = vUV;

    // https://www.clicktorelease.com/blog/creating-spherical-environment-mapping-shader
    vec4 eyePos = vec4(uEyePosition, 1.0);
    vec3 e = normalize(vec3(uMV * eyePos));
    vec3 n = normalize(uNormalMatrix * vNormal);
    vec3 r = reflect(e, n);
    r.z += 1.0;
    float m = 2.0 * length(r);
    fUVMatCap = r.xy / m + 0.5;
}

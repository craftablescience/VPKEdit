#ifdef GL_ES
// Set default precision to medium
precision mediump int;
precision mediump float;
#endif

uniform mat4 mvp;

attribute vec4 iPos;
attribute vec4 iNormal;
attribute vec2 iUV;

varying vec4 oNormal;
varying vec2 oUV;

void main() {
    gl_Position = mvp * iPos;
    oNormal = iNormal;
    oUV = iUV;
}

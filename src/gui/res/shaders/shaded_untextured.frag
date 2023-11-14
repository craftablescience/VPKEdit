#ifdef GL_ES
// Set default precision to medium
precision mediump int;
precision mediump float;
#endif

uniform sampler2D texture;

varying vec4 oNormal;
varying vec2 oUV;

void main() {
    gl_FragColor = texture2D(texture, oUV);
}

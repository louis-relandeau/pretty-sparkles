#version 330 core
in vec2 uv;
out vec4 fragColor;
uniform sampler2D uData;
uniform int uColormap;   // 0 = viridis-like, 1 = heat, 2 = grayscale

vec3 viridis(float t) {
    vec3 a = vec3(0.04, 0.03, 0.33);
    vec3 b = vec3(0.13, 0.69, 0.67);
    vec3 c = vec3(0.99, 0.91, 0.14);
    return t < 0.5 ? mix(a, b, t * 2.0) : mix(b, c, (t - 0.5) * 2.0);
}

vec3 heat(float t) {
    vec3 a = vec3(0.0, 0.0, 0.0);
    vec3 b = vec3(0.8, 0.0, 0.0);
    vec3 c = vec3(1.0, 1.0, 0.0);
    return t < 0.5 ? mix(a, b, t * 2.0) : mix(b, c, (t - 0.5) * 2.0);
}

void main() {
    float val = texture(uData, uv).r;
    vec3 col;
    if      (uColormap == 0) col = viridis(val);
    else if (uColormap == 1) col = heat(val);
    else                     col = vec3(val);
    fragColor = vec4(col, 1.0);
}
#version 330 core
in vec2 uv;
out vec4 fragColor;
uniform sampler2D layer0;
uniform sampler2D layer1;
uniform sampler2D layer2;

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
    float arc = texture(layer0, uv).r;
    float geom = texture(layer1, uv).r;
    float field = texture(layer2, uv).r;
    vec3 arc_field;
    vec3 col;
    if      (uColormap == 0) col = viridis(field);
    else if (uColormap == 1) col = heat(field);
    else                     col = vec3(field);

    arc_field = vec3(mix(col, vec3(0.99, 0, 0.14), arc));
    fragColor = vec4(mix(arc_field, vec3(0), geom), 1.0);
    //
}
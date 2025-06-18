#version 430 core

uniform sampler2D voronoiTex;

in vec2 texCoord;
out vec4 fragColor;

void main() {
    vec4 tex = texture(voronoiTex, texCoord);

    float distance = tex.b; // Distance is stored in blue channel
    distance = clamp(distance * 2.0, 0.0, 1.0); // boost contrast if needed

    fragColor = vec4(distance, distance, distance, 1.0);
}

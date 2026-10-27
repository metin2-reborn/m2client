#include "pixel_shader.h"

namespace metin2::gpu {

    auto pixel_shader_source(std::string_view body) -> std::string {
        std::string source = R"GLSL(#version 330 core
uniform sampler2D uImage;
uniform sampler2D uMask;
uniform float uTime;
uniform vec2 uSize;
uniform vec4 uUser[4];
in vec4 vColor;
in vec4 vTex0;
out vec4 fragColor;
#line 1
)GLSL";
        source += body;
        if (source.back() != '\n')
            source += '\n';
        return source;
    }
}

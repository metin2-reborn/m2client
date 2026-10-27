#pragma once

#include <array>
#include <memory>
#include <string>
#include <string_view>

// A custom pixel shader replaces the fixed-function fragment stage for the draws it is bound to; the
// fixed-function vertex stage still runs. The shader file holds only the body (main and helpers);
// pixel_shader_source puts it after a header that declares what the body may use:
//
//   uniform sampler2D uImage;  // texture stage 0
//   uniform sampler2D uMask;   // texture stage 1 (white when nothing is bound)
//   uniform float uTime;       // seconds, wraps every hour
//   uniform vec2 uSize;        // the drawn box in interface pixels
//   uniform vec4 uUser[4];     // free parameters
//   in vec4 vColor;            // the vertex diffuse colour (the image's alpha and tint)
//   in vec4 vTex0;             // texture coordinates of stage 0
//   out vec4 fragColor;
namespace metin2::gpu {

    class pixel_shader : public std::enable_shared_from_this<pixel_shader> {
      public:
        pixel_shader() = default;
        pixel_shader(const pixel_shader &) = delete;
        auto operator=(const pixel_shader &) -> pixel_shader & = delete;
        virtual ~pixel_shader() = default;
    };

    struct pixel_shader_params {
        float time = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        std::array<float, 16> user = {};
    };

    // The full GLSL 3.30 source for a body; the body's first line is line 1 in compiler messages.
    auto pixel_shader_source(std::string_view body) -> std::string;
}

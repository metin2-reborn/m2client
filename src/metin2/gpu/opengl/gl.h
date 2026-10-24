#pragma once

// OpenGL 3.3 core entry points from raylib's bundled glad, loaded when raylib created the window.
#include <external/glad.h>

#include <metin2/gpu/format.h>

namespace metin2::gpu::opengl {

    struct gl_format {
        format fmt;
        GLenum internal_format;
        GLenum pixel_format;
        GLenum pixel_type;
    };

    auto get_gl_format(format fmt) -> const gl_format *;

    struct extensions {
        bool s3tc = false;
        bool anisotropic = false;
    };

    auto get_extensions() -> const extensions &;

    // Bumped by anything that binds textures or deletes buffers behind the device's back (resource
    // uploads, buffer destruction, the blit): the device's binding cache is only trusted while it
    // has not changed.
    inline uint64_t binding_epoch = 0;
}

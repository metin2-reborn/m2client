#include "gl.h"

#include <cstring>

namespace metin2::gpu::opengl {

    namespace {
        // clang-format off
        constexpr gl_format formats[] = {
            {format::A8R8G8B8,     GL_RGBA8,                         GL_BGRA,            GL_UNSIGNED_INT_8_8_8_8_REV},
            {format::X8R8G8B8,     GL_RGBA8,                         GL_BGRA,            GL_UNSIGNED_INT_8_8_8_8_REV},
            {format::R8G8B8,       GL_RGB8,                          GL_BGR,             GL_UNSIGNED_BYTE},
            {format::R5G6B5,       GL_RGB565,                        GL_RGB,             GL_UNSIGNED_SHORT_5_6_5},
            {format::X1R5G5B5,     GL_RGB5_A1,                       GL_BGRA,            GL_UNSIGNED_SHORT_1_5_5_5_REV},
            {format::A1R5G5B5,     GL_RGB5_A1,                       GL_BGRA,            GL_UNSIGNED_SHORT_1_5_5_5_REV},
            {format::A4R4G4B4,     GL_RGBA4,                         GL_BGRA,            GL_UNSIGNED_SHORT_4_4_4_4_REV},
            {format::X4R4G4B4,     GL_RGBA4,                         GL_BGRA,            GL_UNSIGNED_SHORT_4_4_4_4_REV},
            {format::A8,           GL_R8,                            GL_RED,             GL_UNSIGNED_BYTE},
            {format::DXT1,         GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, 0,                  0},
            {format::DXT2,         GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, 0,                  0},
            {format::DXT3,         GL_COMPRESSED_RGBA_S3TC_DXT3_EXT, 0,                  0},
            {format::DXT4,         GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, 0,                  0},
            {format::DXT5,         GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, 0,                  0},
            {format::D16,          GL_DEPTH_COMPONENT16,             GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT},
            {format::D16_LOCKABLE, GL_DEPTH_COMPONENT16,             GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT},
            {format::D15S1,        GL_DEPTH24_STENCIL8,              GL_DEPTH_STENCIL,   GL_UNSIGNED_INT_24_8},
            {format::D24S8,        GL_DEPTH24_STENCIL8,              GL_DEPTH_STENCIL,   GL_UNSIGNED_INT_24_8},
            {format::D24X8,        GL_DEPTH_COMPONENT24,             GL_DEPTH_COMPONENT, GL_UNSIGNED_INT},
            {format::D24X4S4,      GL_DEPTH24_STENCIL8,              GL_DEPTH_STENCIL,   GL_UNSIGNED_INT_24_8},
            {format::D32,          GL_DEPTH_COMPONENT32,             GL_DEPTH_COMPONENT, GL_UNSIGNED_INT},
        };
        // clang-format on
    }

    auto get_gl_format(format fmt) -> const gl_format * {
        for (const gl_format &f : formats)
            if (f.fmt == fmt)
                return &f;
        return nullptr;
    }

    auto get_extensions() -> const extensions & {
        static const extensions found = [] {
            extensions e;
            GLint count = 0;
            glGetIntegerv(GL_NUM_EXTENSIONS, &count);
            for (GLint i = 0; i < count; ++i) {
                const auto *name = reinterpret_cast<const char *>(glGetStringi(GL_EXTENSIONS, static_cast<GLuint>(i)));
                if (!name)
                    continue;
                if (!strcmp(name, "GL_EXT_texture_compression_s3tc"))
                    e.s3tc = true;
                if (!strcmp(name, "GL_EXT_texture_filter_anisotropic") || !strcmp(name, "GL_ARB_texture_filter_anisotropic"))
                    e.anisotropic = true;
            }
            return e;
        }();
        return found;
    }
}

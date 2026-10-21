#include "device.h"
#include "opengl/device.h"

#include <metin2/log/log.h>

namespace metin2::gpu {

    namespace {
        constexpr std::string_view BACKENDS[] = {"opengl"};

        auto create_backend(std::string_view name, const device_settings &settings) -> std::unique_ptr<device> {
            if (name == "opengl") {
                auto dev = std::make_unique<opengl::gl_device>(settings);
                if (dev->initialize())
                    return dev;
            }
            return nullptr;
        }
    }

    auto available_backends() -> std::span<const std::string_view> {
        return BACKENDS;
    }

    auto create_device(std::string_view backend, const device_settings &settings) -> std::unique_ptr<device> {
        if (auto dev = create_backend(backend, settings))
            return dev;
        for (std::string_view name : BACKENDS) {
            if (name == backend)
                continue;
            log::warn("renderer '{}' is not available, using '{}'", backend, name);
            if (auto dev = create_backend(name, settings))
                return dev;
        }
        return nullptr;
    }

    auto primitive_vertex_count(primitive type, uint32_t primitive_count) -> uint32_t {
        switch (type) {
        case primitive::POINT_LIST:
            return primitive_count;
        case primitive::LINE_LIST:
            return primitive_count * 2;
        case primitive::LINE_STRIP:
            return primitive_count + 1;
        case primitive::TRIANGLE_LIST:
            return primitive_count * 3;
        case primitive::TRIANGLE_STRIP:
        case primitive::TRIANGLE_FAN:
            return primitive_count + 2;
        }
        return 0;
    }
}

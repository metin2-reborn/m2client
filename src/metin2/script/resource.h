#pragma once

// Registers the resource loaders of the client (images, fonts, models, effects, attribute
// maps) with a resource manager and tears every object pool down at exit.

#include <metin2/graphics/resource.h>
#include <metin2/graphics/resource_manager.h>

namespace metin2::inline script {

    class python_resource {
      public:
        python_resource();
        ~python_resource();

        void destroy();

      protected:
    };

    inline auto get_python_resource() -> python_resource & {
        static python_resource instance;
        return instance;
    }
}

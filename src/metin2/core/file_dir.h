#pragma once

namespace metin2::inline core {

    // Recursive-capable directory walker: create() visits every entry of a directory and calls
    // on_folder for sub-directories and on_file for files whose extension matches the filter.
    class dir {
      public:
        dir() = default;

        virtual ~dir() = default;

        auto create(const char *filter, const char *path = "", bool checked_extension = false) -> bool;

      protected:
        virtual auto on_folder(const char *filter, const char *path, const char *name) -> bool = 0;
        virtual auto on_file(const char *path, const char *name) -> bool = 0;
    };
}

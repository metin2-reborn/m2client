#include <metin2/core/parse.h>
#include <metin2/core/utils.h>
#include <metin2/log/log.h>
#include <string.h>

#include <metin2/core/file_loader.h>
#include "property.h"
#include "property_manager.h"

namespace metin2::inline world::inline maps {
    // Property file format: a fourcc, then \r\n, then the text file loader's format.
    property::property(const char *file_name) : mc_file_name_(nullptr), crc_(0) {
        file_name_ = file_name;
        string_path(file_name_);

        mc_file_name_ = strrchr(file_name_.c_str(), '/');

        if (!mc_file_name_)
            mc_file_name_ = file_name_.c_str();
        else
            ++mc_file_name_;
    }

    property::~property() = default;

    auto property::get_crc() -> uint32_t {
        return crc_;
    }

    auto property::file_name() -> const char * {
        return (file_name_.c_str());
    }

    auto property::string(const char *key, const char **string) -> bool {
        std::string temp_key = key;
        stl_lowers(temp_key);
        auto it = token_map_.find(temp_key.c_str());

        if (token_map_.end() == it)
            return false;

        *string = it->second[0].c_str();
        return true;
    }

    auto property::size() -> uint32_t {
        return token_map_.size();
    }

    auto property::vector(const char *key, token_vector &token_vector_value) -> bool {
        std::string temp_key = key;
        stl_lowers(temp_key);
        auto it = token_map_.find(temp_key.c_str());

        if (token_map_.end() == it)
            return false;

        // Copying with std::copy crashed here, so the tokens are copied one by one.
        token_vector &source_token_vector = it->second;
        auto itor = source_token_vector.begin();
        for (; itor != source_token_vector.end(); ++itor) {
            token_vector_value.push_back(*itor);
        }

        return true;
    }

    void property::put_vector(const char *key, const token_vector &token_vector) {
        std::string temp_key = key;
        stl_lowers(temp_key);

        token_map_.insert(token_vector_map::value_type(temp_key, token_vector));
    }

    auto property::read_from_memory(const void *c_pv_data, int len, const char *file_name) -> bool {
        const char *pc_data = (const char *)c_pv_data;

        if (*(uint32_t *)pc_data != MAKEFOURCC('Y', 'P', 'R', 'T'))
            return false;

        pc_data += sizeof(uint32_t);

        if (*pc_data != '\r' || *(pc_data + 1) != '\n') {
            metin2::log::error("Property {}: bad file format after the FourCC", file_name);
            return false;
        }

        pc_data += 2;

        token_vector token_vector;
        memory_text_file_loader text_file_loader;
        text_file_loader.bind(len - (sizeof(uint32_t) + 2), pc_data);

        crc_text_ = text_file_loader.line_string(0);
        crc_ = metin2::to_number<int>(crc_text_.c_str());

        for (uint32_t i = 1; i < text_file_loader.line_count(); ++i) {
            if (!text_file_loader.split_line(i, &token_vector))
                continue;

            stl_lowers(token_vector[0]);
            std::string key = token_vector[0];

            token_vector.erase(token_vector.begin());
            put_vector(key.c_str(), token_vector);
        }

        return true;
    }

    void property::clear() {
        token_map_.clear();
    }
}

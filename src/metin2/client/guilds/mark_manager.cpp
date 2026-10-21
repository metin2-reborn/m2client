#include "mark_manager.h"
#include <cstring>
#include <metin2/core/crc32.h>
#include <metin2/files/eter_mark_manager.h>
#include <metin2/log/log.h>

namespace metin2::inline client::inline guilds {

#define thecore_memcpy memcpy
#define itertype(cont) typeof((cont).begin())

    auto guild_mark_manager::new_image() -> guild_mark_image * {
        return new guild_mark_image;
    }

    void guild_mark_manager::delete_image(guild_mark_image *img_del) {
        delete img_del;
    }

    // Mark files are named "mark/..." and live in the user's profile.
    guild_mark_manager::guild_mark_manager() {
        // Build the set of free mark IDs (used by the server-side code).
        for (uint32_t i = 0; i < static_cast<uint32_t>(MAX_IMAGE_COUNT) * guild_mark_image::MARK_TOTAL_COUNT; ++i)
            free_mark_id_.insert(i);
    }

    guild_mark_manager::~guild_mark_manager() {
        for (auto it = idx_image_.begin(); it != idx_image_.end(); ++it)
            delete_image(it->second);

        idx_image_.clear();
    }

    auto guild_mark_manager::mark_image_filename(uint32_t img_idx, std::string &path) const -> bool {
        if (img_idx >= MAX_IMAGE_COUNT)
            return false;

        char buf[64];
        snprintf(buf, sizeof(buf), "mark/%s_%u.tga", path_prefix_.c_str(), img_idx);
        path = buf;
        return true;
    }

    void guild_mark_manager::set_mark_path_prefix(const char *prefix) {
        path_prefix_ = prefix;
    }

    auto guild_mark_manager::save_mark_index() -> bool {
        char buf[64];
        snprintf(buf, sizeof(buf), "mark/%s_index", path_prefix_.c_str());
        FILE *fp = fopen(get_mark_manager().path(buf).c_str(), "w");

        if (!fp) {
            metin2::log::error("Cannot open the guild mark index file");
            return false;
        }

        for (auto it = gid_mark_id_.begin(); it != gid_mark_id_.end(); ++it)
            fprintf(fp, "%d %d\n", it->first, it->second);

        fclose(fp);
        metin2::log::debug("Saved the guild mark index ({} entries)", gid_mark_id_.size());
        return true;
    }

    void guild_mark_manager::load_mark_images() {
        bool is_mark_exists[MAX_IMAGE_COUNT];
        memset(is_mark_exists, 0, sizeof(is_mark_exists));

        for (auto it = gid_mark_id_.begin(); it != gid_mark_id_.end(); ++it) {
            uint32_t mark_id = it->second;

            if (mark_id < static_cast<uint32_t>(MAX_IMAGE_COUNT) * guild_mark_image::MARK_TOTAL_COUNT)
                is_mark_exists[mark_id / guild_mark_image::MARK_TOTAL_COUNT] = true;
        }

        for (uint32_t i = 0; i < MAX_IMAGE_COUNT; ++i)
            if (is_mark_exists[i])
                image(i);
    }

    void guild_mark_manager::save_mark_image(uint32_t img_idx) {
        std::string path;

        if (mark_image_filename(img_idx, path))
            if (!image(img_idx)->save(get_mark_manager().path(path).c_str()))
                metin2::log::error("Cannot save {}", path.c_str());
    }

    auto guild_mark_manager::image(uint32_t img_idx) -> guild_mark_image * {
        auto it = idx_image_.find(img_idx);

        if (it == idx_image_.end()) {
            std::string image_path_value;

            if (mark_image_filename(img_idx, image_path_value)) {
                guild_mark_image *image = new_image();
                idx_image_.insert(std::map<uint32_t, guild_mark_image *>::value_type(img_idx, image));
                image->load(get_mark_manager().path(image_path_value).c_str());
                return image;
            } else
                return nullptr;
        } else
            return it->second;
    }

    auto guild_mark_manager::add_mark_id_by_guild_id(uint32_t guild_id, uint32_t mark_id) -> bool {
        if (mark_id >= static_cast<uint32_t>(MAX_IMAGE_COUNT) * guild_mark_image::MARK_TOTAL_COUNT)
            return false;

        gid_mark_id_.insert(std::map<uint32_t, uint32_t>::value_type(guild_id, mark_id));
        free_mark_id_.erase(mark_id);
        return true;
    }

    auto guild_mark_manager::mark_id(uint32_t guild_id) -> uint32_t {
        auto it = gid_mark_id_.find(guild_id);

        if (it == gid_mark_id_.end())
            return INVALID_MARK_ID;

        return it->second;
    }

    auto guild_mark_manager::alloc_mark_id(uint32_t guild_id_value) -> uint32_t {
        auto it = free_mark_id_.lower_bound(0);

        if (it == free_mark_id_.end())
            return INVALID_MARK_ID;

        uint32_t mark_id_value = *it;

        uint32_t img_idx = mark_id_value / guild_mark_image::MARK_TOTAL_COUNT;
        guild_mark_image *image_value = image(img_idx); // create the image if it does not exist

        if (image_value && add_mark_id_by_guild_id(guild_id_value, mark_id_value))
            return mark_id_value;

        return INVALID_MARK_ID;
    }

    auto guild_mark_manager::mark_image_count() const -> uint32_t {
        return idx_image_.size();
    }

    auto guild_mark_manager::save_mark(uint32_t guild_id_value, uint8_t *mark_image) -> uint32_t {
        uint32_t id_mark;

        if ((id_mark = mark_id(guild_id_value)) == INVALID_MARK_ID) {
            if ((id_mark = alloc_mark_id(guild_id_value)) == INVALID_MARK_ID) {
                metin2::log::error("Guild {}: cannot allocate a mark id", guild_id_value);
                return false;
            } else
                metin2::log::debug("Allocated guild mark id {}", id_mark);
        } else
            metin2::log::debug("Found guild mark id {}", id_mark);

        uint32_t img_idx = (id_mark / guild_mark_image::MARK_TOTAL_COUNT);
        guild_mark_image *image_value = image(img_idx);

        if (image_value) {
            printf("imgidx %u ", img_idx);

            std::string path_image;
            mark_image_filename(img_idx, path_image);
            image_value->save(get_mark_manager().path(path_image).c_str());

            save_mark_index();
        }

        return id_mark;
    }

    void guild_mark_manager::delete_mark(uint32_t guild_id_value) {
        auto it = gid_mark_id_.find(guild_id_value);

        if (it == gid_mark_id_.end())
            return;

        guild_mark_image *image_value;

        if ((image_value = image(it->second / guild_mark_image::MARK_TOTAL_COUNT)) != nullptr)
            image_value->delete_mark(it->second % guild_mark_image::MARK_TOTAL_COUNT);

        gid_mark_id_.erase(it);
        free_mark_id_.insert(it->second);

        save_mark_index();
    }

    void guild_mark_manager::diff_blocks(uint32_t img_idx, const uint32_t *crc_list, std::map<uint8_t, const guild_mark_block *> &diff_blocks) {
        diff_blocks.clear();

        // A client cannot ask for an image the server does not have.
        if (idx_image_.end() == idx_image_.find(img_idx)) {
            metin2::log::error("Invalid guild mark image index {}", img_idx);
            return;
        }

        guild_mark_image *p = image(img_idx);

        if (p)
            p->diff_blocks(crc_list, diff_blocks);
    }

    auto guild_mark_manager::save_block_from_compressed_data(uint32_t img_idx, uint32_t pos_block, const uint8_t *block, uint32_t size_value) -> bool {
        guild_mark_image *image_value = image(img_idx);

        if (image_value)
            image_value->save_block_from_compressed_data(pos_block, block, size_value);

        return false;
    }

    auto guild_mark_manager::block_crc_list(uint32_t img_idx, uint32_t *crc_list) -> bool {
        // A client cannot ask for an image the server does not have.
        if (idx_image_.end() == idx_image_.find(img_idx)) {
            metin2::log::error("Invalid guild mark image index {}", img_idx);
            return false;
        }

        guild_mark_image *p = image(img_idx);

        if (p)
            p->block_crc_list(crc_list);

        return true;
    }

    void guild_mark_manager::upload_symbol(uint32_t guild_id, int size, const uint8_t *data) {
        metin2::log::debug("Guild {}: uploading a {} byte symbol", guild_id, size);

        if (symbol_.find(guild_id) == symbol_.end())
            symbol_.insert(std::make_pair(guild_id, guild_symbol()));

        guild_symbol &symbol = symbol_[guild_id];
        symbol.raw.clear();

        if (size > 0) {
            symbol.raw.reserve(size);
            std::copy(data, (data + size), std::back_inserter(symbol.raw));
            symbol.crc = get_crc32(reinterpret_cast<const char *>(data), size);
        }
    }
}

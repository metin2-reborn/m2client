#include "effect_manager.h"
#include <metin2/core/crc32.h>
#include <metin2/core/random.h>
#include <metin2/core/utils.h>
#include <metin2/graphics/state_manager.h>
#include <metin2/log/log.h>

namespace metin2::inline effects {

    void effect_manager::get_info(std::string *pst_info) {
        char sz_info[256];

        snprintf(sz_info, sizeof(sz_info), "Effect: Inst - ED %zu, EI %zu Pool - PSI %d, MI %d, LI %d, PI %d, EI %d, ED %d, PSD %d, EM %d, LD %d",
            eft_data_map_.size(),
            eft_inst_map_.size(),
            particle_system_instance::pool_value.capacity(),
            effect_mesh_instance::pool_value.capacity(),
            light_instance::pool_value.capacity(),
            particle_instance::pool_value.capacity(),
            effect_instance::pool_value.capacity(),
            effect_data::pool_value.capacity(),
            particle_system_data::pool_value.capacity(),
            effect_mesh_script::pool_value.capacity(),
            light_data::pool_value.capacity());
        pst_info->append(sz_info);
    }

    void effect_manager::update_sound() {
        for (auto itor = eft_inst_map_.begin(); itor != eft_inst_map_.end(); ++itor) {
            effect_instance *effect_instance = itor->second;

            effect_instance->update_sound();
        }
    }

    auto effect_manager::is_alive_effect(uint32_t instance_index) -> bool {
        auto f = eft_inst_map_.find(instance_index);
        if (eft_inst_map_.end() == f)
            return false;

        return f->second->is_alive() ? true : false;
    }

    void effect_manager::update() {

        for (auto itor = eft_inst_map_.begin(); itor != eft_inst_map_.end();) {
            effect_instance *effect_instance = itor->second;

            effect_instance->update(/*fElapsedTime*/);

            if (!effect_instance->is_alive()) {
                itor = eft_inst_map_.erase(itor);

                effect_instance::delete_value(effect_instance);
            } else {
                ++itor;
            }
        }
    }

    struct effect_manager_less_effect_instance_ptr_render_order {
        auto operator()(effect_instance *left, effect_instance *right) -> bool {
            return left->less_render_order(right);
        }
    };

    struct effect_manager_effect_instance_render {
        inline void operator()(effect_instance *eft_inst) {
            eft_inst->render();
        }
    };

    void effect_manager::render() {
        STATEMANAGER.set_texture(0, nullptr);
        STATEMANAGER.set_texture(1, nullptr);

        // Render sorted by effect data so that instances of the same effect share state changes.
        static std::vector<effect_instance *> vct_eft_inst_sort;
        vct_eft_inst_sort.clear();

        for (auto i = eft_inst_map_.begin(); i != eft_inst_map_.end(); ++i)
            vct_eft_inst_sort.push_back(i->second);

        std::sort(vct_eft_inst_sort.begin(), vct_eft_inst_sort.end(), effect_manager_less_effect_instance_ptr_render_order());
        std::for_each(vct_eft_inst_sort.begin(), vct_eft_inst_sort.end(), effect_manager_effect_instance_render());
    }

    auto effect_manager::register_effect(const char *c_sz_file_name, bool is_exist_delete, bool is_need_cache) -> bool {
        std::string str_file_name;
        string_path(c_sz_file_name, str_file_name);
        uint32_t crc = get_case_crc32(str_file_name.c_str(), str_file_name.length());

        auto itor = eft_data_map_.find(crc);
        if (eft_data_map_.end() != itor) {
            if (is_exist_delete) {
                effect_data *eft_data = itor->second;
                effect_data::delete_value(eft_data);
                eft_data_map_.erase(itor);
            } else {
                return true;
            }
        }

        effect_data *eft_data = effect_data::new_value();

        if (!eft_data->load_script(c_sz_file_name)) {
            metin2::log::error("Cannot load effect script {}", c_sz_file_name);
            effect_data::delete_value(eft_data);
            return false;
        }

        eft_data_map_.insert(effect_data_map::value_type(crc, eft_data));

        if (is_need_cache) {
            if (eft_cache_map_.find(crc) == eft_cache_map_.end()) {
                effect_instance *new_eft_inst = effect_instance::new_value();
                new_eft_inst->set_effect_data_pointer(eft_data);
                eft_cache_map_.insert(effect_instance_map::value_type(crc, new_eft_inst));
            }
        }

        return true;
    }
    auto effect_manager::register_effect2(const char *c_sz_file_name, uint32_t *ret_crc, bool is_need_cache) -> bool {
        std::string str_file_name;
        string_path(c_sz_file_name, str_file_name);
        uint32_t crc = get_case_crc32(str_file_name.c_str(), str_file_name.length());
        *ret_crc = crc;

        return register_effect(c_sz_file_name, false, is_need_cache);
    }

    auto effect_manager::create_effect(const char *c_sz_file_name, const math::vector3 &position, const math::vector3 &rotation) -> int {
        std::string str_file_name;
        string_path(c_sz_file_name, str_file_name);
        uint32_t id = get_case_crc32(str_file_name.c_str(), str_file_name.length());
        return create_effect(id, position, rotation);
    }

    auto effect_manager::create_effect(uint32_t id, const math::vector3 &position, const math::vector3 &rotation) -> int {
        int instance_index = empty_index();

        create_effect_instance(instance_index, id);
        select_effect_instance(instance_index);
        math::matrix mat;
        mat = math::matrix_rotation_yaw_pitch_roll(math::to_radian(rotation.x), math::to_radian(rotation.y), math::to_radian(rotation.z));
        mat._41 = position.x;
        mat._42 = position.y;
        mat._43 = position.z;
        set_effect_instance_global_matrix(mat);

        return instance_index;
    }

    void effect_manager::create_effect_instance(uint32_t instance_index, uint32_t id) {
        if (!id)
            return;

        effect_data *effect;
        if (!get_effect_data(id, &effect)) {
            metin2::log::debug("Effect {} not found", id);
            return;
        }

        effect_instance *effect_instance = effect_instance::new_value();
        effect_instance->set_effect_data_pointer(effect);

        eft_inst_map_.insert(effect_instance_map::value_type(instance_index, effect_instance));
    }

    auto effect_manager::destroy_effect_instance(uint32_t instance_index) -> bool {
        auto itor = eft_inst_map_.find(instance_index);

        if (itor == eft_inst_map_.end())
            return false;

        effect_instance *effect_instance = itor->second;

        eft_inst_map_.erase(itor);

        effect_instance::delete_value(effect_instance);

        return true;
    }

    void effect_manager::deactive_effect_instance(uint32_t instance_index) {
        auto itor = eft_inst_map_.find(instance_index);

        if (itor == eft_inst_map_.end())
            return;

        effect_instance *effect_instance = itor->second;
        effect_instance->set_deactive();
    }

    void effect_manager::create_unsafe_effect_instance(uint32_t effect_data_id, effect_instance **effect_instance_value) {
        *effect_instance_value = nullptr;

        effect_data *effect_value;
        if (!get_effect_data(effect_data_id, &effect_value)) {
            metin2::log::debug("Effect {} not found", effect_data_id);
            return;
        }

        effect_instance *eft_inst_new = effect_instance::new_value();
        eft_inst_new->set_effect_data_pointer(effect_value);

        *effect_instance_value = eft_inst_new;
    }

    auto effect_manager::destroy_unsafe_effect_instance(effect_instance *effect_instance) -> bool {
        if (!effect_instance)
            return false;

        effect_instance::delete_value(effect_instance);

        return true;
    }

    auto effect_manager::select_effect_instance(uint32_t instance_index) -> bool {
        auto itor = eft_inst_map_.find(instance_index);

        selected_effect_instance_ = nullptr;

        if (eft_inst_map_.end() == itor)
            return false;

        selected_effect_instance_ = itor->second;

        return true;
    }

    void effect_manager::set_effect_textures(uint32_t id, std::vector<std::string> textures) {
        effect_data *effect_data;
        if (!get_effect_data(id, &effect_data)) {
            metin2::log::debug("Effect {} not found", id);
            return;
        }

        for (uint32_t i = 0; i < textures.size() && i < effect_data->particle_count(); i++) {
            particle_system_data *particle = effect_data->particle_pointer(i);
            particle->change_texture(textures.at(i).c_str());
        }
    }

    void effect_manager::set_effect_instance_global_matrix(const math::matrix &global) {
        if (!selected_effect_instance_)
            return;

        selected_effect_instance_->set_global_matrix(global);
    }

    void effect_manager::show_effect() {
        if (!selected_effect_instance_)
            return;

        selected_effect_instance_->show();
    }

    void effect_manager::hide_effect() {
        if (!selected_effect_instance_)
            return;

        selected_effect_instance_->hide();
    }

    auto effect_manager::get_effect_data(uint32_t id, effect_data **effect) -> bool {
        auto itor = eft_data_map_.find(id);

        if (itor == eft_data_map_.end())
            return false;

        *effect = itor->second;

        return true;
    }

    auto effect_manager::empty_index() -> int {
        static int max_index = 1;

        if (max_index > 2100000000)
            max_index = 1;

        int next_index = max_index++;
        while (eft_inst_map_.find(next_index) != eft_inst_map_.end())
            next_index++;

        return next_index;
    }

    void effect_manager::delete_all_instances() {
        destroy_effect_instance_map();
    }

    void effect_manager::destroy_effect_instance_map() {
        for (auto i = eft_inst_map_.begin(); i != eft_inst_map_.end(); ++i) {
            effect_instance *eft_inst = i->second;
            effect_instance::delete_value(eft_inst);
        }

        eft_inst_map_.clear();
    }

    void effect_manager::destroy_effect_cache_map() {
        for (auto i = eft_cache_map_.begin(); i != eft_cache_map_.end(); ++i) {
            effect_instance *eft_inst = i->second;
            effect_instance::delete_value(eft_inst);
        }

        eft_cache_map_.clear();
    }

    void effect_manager::destroy_effect_data_map() {
        for (auto i = eft_data_map_.begin(); i != eft_data_map_.end(); ++i) {
            effect_data *data = i->second;
            effect_data::delete_value(data);
        }

        eft_data_map_.clear();
    }

    void effect_manager::destroy() {
        destroy_effect_instance_map();
        destroy_effect_cache_map();
        destroy_effect_data_map();

        initialize_impl();
    }

    void effect_manager::initialize_impl() {
        selected_effect_instance_ = nullptr;
    }

    effect_manager::effect_manager() {
        initialize_impl();
    }

    effect_manager::~effect_manager() {
        destroy();
    }
}

#include "python_guild.h"
#include <metin2/client/app/abstract_player.h>
#include <metin2/script/python_header.h>
#include <metin2/script/python_utils.h>
#include "mark_manager.h"

namespace metin2::inline client::inline guilds {

    namespace {
        constexpr int GUILD_LEVEL_MAX = 20;

        // Experience a guild needs to finish each level.
        auto guild_level_experience(int level) -> unsigned {
            static const uint32_t experience[GUILD_LEVEL_MAX + 1] = {
                0,         // 0
                6000UL,    // 1
                18000UL,   // 2
                36000UL,   // 3
                64000UL,   // 4
                94000UL,   // 5
                130000UL,  // 6
                172000UL,  // 7
                220000UL,  // 8
                274000UL,  // 9
                334000UL,  // 10
                400000UL,  // 11
                600000UL,  // 12
                840000UL,  // 13
                1120000UL, // 14
                1440000UL, // 15
                1800000UL, // 16
                2600000UL, // 17
                3200000UL, // 18
                4000000UL, // 19
                16800000UL // 20
            };

            if (level < 0 || level > GUILD_LEVEL_MAX) {
                return 0;
            }
            return experience[level];
        }
    }

    std::map<uint32_t, uint32_t> guild_skill_slot_to_index_map;

    void python_guild::enable_guild() {
        guild_enable_ = true;
    }

    void python_guild::set_guild_money(uint32_t money) {
        guild_info_value_.guild_money = money;
    }

    void python_guild::set_guild_exp(uint8_t level, uint32_t exp) {
        guild_info_value_.guild_level = level;
        guild_info_value_.current_experience = exp;
    }

    void python_guild::set_grade_data(uint8_t grade_number, guild_grade_data &guild_grade_data_value) {
        grade_data_map_value_[grade_number] = guild_grade_data_value;
    }

    void python_guild::set_grade_name(uint8_t grade_number, const char *name) {
        if (!is_grade_data(grade_number))
            return;

        guild_grade_data &grade_data = grade_data_map_value_.find(grade_number)->second;
        grade_data.name_value = name;
    }

    void python_guild::set_grade_authority(uint8_t grade_number, uint8_t authority) {
        if (!is_grade_data(grade_number))
            return;

        guild_grade_data &grade_data = grade_data_map_value_.find(grade_number)->second;
        grade_data.authority_flag = authority;
    }

    void python_guild::clear_comment() {
        guild_board_comment_vector_.clear();
    }

    void python_guild::register_comment(uint32_t comment_id, const char *name, const char *comment) {
        if (0 == strlen(comment))
            return;

        guild_board_comment_data comment_data;
        comment_data.comment_id = comment_id;
        comment_data.name_value = name;
        comment_data.comment = comment;

        guild_board_comment_vector_.push_back(comment_data);
    }

    void python_guild::register_member(guild_member_data &r_guild_member_data) {
        guild_member_data *p_guild_member_data;
        if (member_data_ptr_by_pid(r_guild_member_data.pid, &p_guild_member_data)) {
            p_guild_member_data->general_flag = r_guild_member_data.general_flag;
            p_guild_member_data->grade = r_guild_member_data.grade;
            p_guild_member_data->level_value = r_guild_member_data.level_value;
            p_guild_member_data->offer = r_guild_member_data.offer;
        } else {
            guild_member_data_vector_value_.push_back(r_guild_member_data);
        }

        calculate_level_average();
        sort_member();
    }

    struct python_guild_find_guild_member_by_pid {
        python_guild_find_guild_member_by_pid(uint32_t searching_pid_value) : searching_pid(searching_pid_value) {}
        auto operator()(python_guild::guild_member_data &guild_member_data) -> int {
            return guild_member_data.pid == searching_pid;
        }

        uint32_t searching_pid;
    };

    struct python_guild_find_guild_member_by_name {
        python_guild_find_guild_member_by_name(const char *searching_name_value) : searching_name(searching_name_value) {}
        auto operator()(python_guild::guild_member_data &guild_member_data) -> int {
            return 0 == searching_name.compare(guild_member_data.name_value.c_str());
        }

        std::string searching_name;
    };

    void python_guild::change_guild_member_grade(uint32_t pid, uint8_t grade) {
        guild_member_data *guild_member_data_value;
        if (!member_data_ptr_by_pid(pid, &guild_member_data_value))
            return;

        guild_member_data_value->grade = grade;
    }

    void python_guild::change_guild_member_general_flag(uint32_t pid, uint8_t flag) {
        guild_member_data *guild_member_data_value;
        if (!member_data_ptr_by_pid(pid, &guild_member_data_value))
            return;

        guild_member_data_value->general_flag = flag;
    }

    void python_guild::remove_member(uint32_t pid) {
        guild_member_data_vector::iterator itor;
        itor = std::find_if(guild_member_data_vector_value_.begin(),
            guild_member_data_vector_value_.end(),
            python_guild_find_guild_member_by_pid(pid));

        if (guild_member_data_vector_value_.end() == itor)
            return;

        guild_member_data_vector_value_.erase(itor);
    }

    void python_guild::register_guild_name(uint32_t id, const char *name) {
        guild_name_map_value_.insert(std::make_pair(id, std::string(name)));
    }

    auto python_guild::is_main_player(uint32_t pid) -> bool {
        guild_member_data *guild_member_data_value;
        if (!member_data_ptr_by_pid(pid, &guild_member_data_value))
            return false;

        abstract_player &player = get_abstract_player();
        return 0 == guild_member_data_value->name_value.compare(player.get_name());
    }

    auto python_guild::is_guild_enable() -> bool {
        return guild_enable_;
    }

    auto python_guild::guild_info_ref() -> python_guild::guild_info & {
        return guild_info_value_;
    }

    auto python_guild::grade_data_ptr(uint32_t grade_number, guild_grade_data **data) -> bool {
        auto itor = grade_data_map_value_.find(grade_number);
        if (grade_data_map_value_.end() == itor)
            return false;

        *data = &(itor->second);

        return true;
    }

    auto python_guild::guild_board_comment_vector() -> const python_guild::guild_board_comment_data_vector & {
        return guild_board_comment_vector_;
    }

    auto python_guild::member_count() -> uint32_t {
        return guild_member_data_vector_value_.size();
    }

    auto python_guild::member_data_ptr(uint32_t index, guild_member_data **data) -> bool {
        if (index >= guild_member_data_vector_value_.size())
            return false;

        *data = &guild_member_data_vector_value_[index];

        return true;
    }

    auto python_guild::member_data_ptr_by_pid(uint32_t pid, guild_member_data **data) -> bool {
        guild_member_data_vector::iterator itor;
        itor = std::find_if(guild_member_data_vector_value_.begin(),
            guild_member_data_vector_value_.end(),
            python_guild_find_guild_member_by_pid(pid));

        if (guild_member_data_vector_value_.end() == itor)
            return false;

        *data = &(*itor);
        return true;
    }

    auto python_guild::member_data_ptr_by_name(const char *name, guild_member_data **data) -> bool {
        guild_member_data_vector::iterator itor;
        itor = std::find_if(guild_member_data_vector_value_.begin(),
            guild_member_data_vector_value_.end(),
            python_guild_find_guild_member_by_name(name));

        if (guild_member_data_vector_value_.end() == itor)
            return false;

        *data = &(*itor);
        return true;
    }

    auto python_guild::guild_member_level_average() -> uint32_t {
        return member_level_average_;
    }

    auto python_guild::guild_experience_summary() -> uint32_t {
        return member_experience_summary_;
    }

    auto python_guild::guild_skill_data_ref() -> python_guild::guild_skill_data & {
        return guild_skill_data_value_;
    }

    auto python_guild::guild_name(uint32_t id, std::string *pstr_guild_name) -> bool {
        if (guild_name_map_value_.end() == guild_name_map_value_.find(id))
            return false;

        *pstr_guild_name = guild_name_map_value_[id];

        return true;
    }

    auto python_guild::guild_id() -> uint32_t {
        return guild_info_value_.guild_id;
    }

    auto python_guild::has_guild_land() -> bool {
        return guild_info_value_.has_land;
    }

    void python_guild::start_guild_war(uint32_t enemy_guild_id) {
        int i;

        for (i = 0; i < ENEMY_GUILD_SLOT_MAX_COUNT; ++i)
            if (enemy_guild_id == enemy_guild_id_[i])
                return;

        for (i = 0; i < ENEMY_GUILD_SLOT_MAX_COUNT; ++i)
            if (0 == enemy_guild_id_[i]) {
                enemy_guild_id_[i] = enemy_guild_id;
                break;
            }
    }

    void python_guild::end_guild_war(uint32_t enemy_guild_id) {
        for (int i = 0; i < ENEMY_GUILD_SLOT_MAX_COUNT; ++i)
            if (enemy_guild_id == enemy_guild_id_[i])
                enemy_guild_id_[i] = 0;
    }

    auto python_guild::enemy_guild_id(uint32_t index) -> uint32_t {
        if (index >= ENEMY_GUILD_SLOT_MAX_COUNT)
            return 0;

        return enemy_guild_id_[index];
    }

    auto python_guild::is_doing_guild_war() -> bool {
        for (int i = 0; i < ENEMY_GUILD_SLOT_MAX_COUNT; ++i)
            if (0 != enemy_guild_id_[i]) {
                return true;
            }

        return false;
    }

    void python_guild::calculate_level_average() {
        member_level_summary_ = 0;
        member_level_average_ = 0;
        member_experience_summary_ = 0;

        if (guild_member_data_vector_value_.empty())
            return;

        guild_member_data_vector::iterator itor;

        itor = guild_member_data_vector_value_.begin();
        for (; itor != guild_member_data_vector_value_.end(); ++itor) {
            guild_member_data &guild_member_data_value = *itor;
            member_level_summary_ += guild_member_data_value.level_value;
            member_experience_summary_ += guild_member_data_value.offer;
        }

        assert(!guild_member_data_vector_value_.empty());
        member_level_average_ = member_level_summary_ / guild_member_data_vector_value_.size();
    }

    struct python_guild_less_member_grade {
        auto operator()(python_guild::guild_member_data &rleft, python_guild::guild_member_data &rright) -> bool {
            if (rleft.grade < rright.grade)
                return true;

            return false;
        }
    };

    void python_guild::sort_member() {
        std::sort(guild_member_data_vector_value_.begin(), guild_member_data_vector_value_.end(), python_guild_less_member_grade());
    }

    auto python_guild::is_grade_data(uint8_t grade_number) -> bool {
        return grade_data_map_value_.end() != grade_data_map_value_.find(grade_number);
    }

    void python_guild::initialize_impl() {
        memset(&guild_info_value_, 0, sizeof(guild_info_value_));
        memset(&guild_skill_data_value_, 0, sizeof(guild_skill_data_value_));
        memset(&enemy_guild_id_, 0, ENEMY_GUILD_SLOT_MAX_COUNT * sizeof(uint32_t));
        grade_data_map_value_.clear();
        guild_member_data_vector_value_.clear();
        member_level_summary_ = 0;
        member_level_average_ = 0;
        guild_enable_ = false;
        guild_name_map_value_.clear();
    }

    void python_guild::destroy() {
        initialize_impl();
    }

    python_guild::python_guild() {
        initialize_impl();
    }
    python_guild::~python_guild() = default;

    auto guild_is_guild_enable(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_guild().is_guild_enable());
    }

    auto guild_get_guild_id(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_guild().guild_id());
    }

    auto guild_has_guild_land(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_guild().has_guild_land());
    }

    auto guild_get_guild_name(PyObject *self, PyObject *args) -> PyObject * {
        int guild_id;
        if (!py_tuple_get_integer(args, 0, &guild_id)) {
            python_guild::guild_info &guild_info = get_python_guild().guild_info_ref();
            return Py_BuildValue("s", guild_info.guild_name);
        }

        std::string guild_name;
        if (!get_python_guild().guild_name(guild_id, &guild_name))
            return Py_BuildValue("s", "Noname");

        return Py_BuildValue("s", guild_name.c_str());
    }

    auto guild_get_guild_master_name(PyObject *self, PyObject *args) -> PyObject * {
        python_guild::guild_info &guild_info = get_python_guild().guild_info_ref();

        python_guild::guild_member_data *data;
        if (!get_python_guild().member_data_ptr_by_pid(guild_info.master_pid, &data))
            return Py_BuildValue("s", "Noname");

        return Py_BuildValue("s", data->name_value.c_str());
    }

    auto guild_get_enemy_guild_name(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return Py_BuildValue("s", "");

        uint32_t enemy_guild_id = get_python_guild().enemy_guild_id(index);

        std::string enemy_guild_name;
        if (!get_python_guild().guild_name(enemy_guild_id, &enemy_guild_name))
            return Py_BuildValue("s", "");

        return Py_BuildValue("s", enemy_guild_name.c_str());
    }

    auto guild_get_guild_money(PyObject *self, PyObject *args) -> PyObject * {
        python_guild::guild_info &guild_info = get_python_guild().guild_info_ref();
        return Py_BuildValue("i", guild_info.guild_money);
    }

    auto guild_get_guild_board_comment_count(PyObject *self, PyObject *args) -> PyObject * {
        const python_guild::guild_board_comment_data_vector &comment_vector = get_python_guild().guild_board_comment_vector();
        return Py_BuildValue("i", comment_vector.size());
    }

    auto guild_get_guild_board_comment_data(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_build_exception();

        const python_guild::guild_board_comment_data_vector &comment_vector = get_python_guild().guild_board_comment_vector();
        if (uint32_t(index) >= comment_vector.size())
            return Py_BuildValue("iss", 0, "Noname", "Noname");

        const python_guild::guild_board_comment_data &data = comment_vector[index];

        return Py_BuildValue("iss", data.comment_id, data.name_value.c_str(), data.comment.c_str());
    }

    auto guild_get_guild_level(PyObject *self, PyObject *args) -> PyObject * {
        python_guild::guild_info &guild_info = get_python_guild().guild_info_ref();
        return Py_BuildValue("i", guild_info.guild_level);
    }

    auto guild_get_guild_experience(PyObject *self, PyObject *args) -> PyObject * {
        python_guild::guild_info &guild_info = get_python_guild().guild_info_ref();

        if (guild_info.guild_level >= uint32_t(GUILD_LEVEL_MAX))
            return Py_BuildValue("ii", 0, 0);

        unsigned last_exp = guild_level_experience(guild_info.guild_level);

        return Py_BuildValue("ii", guild_info.current_experience, last_exp - guild_info.current_experience);
    }

    auto guild_get_guild_member_count(PyObject *self, PyObject *args) -> PyObject * {
        python_guild::guild_info &guild_info = get_python_guild().guild_info_ref();
        return Py_BuildValue("ii", guild_info.current_member_count, guild_info.max_member_count);
    }

    auto guild_get_guild_member_level_average(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_guild().guild_member_level_average());
    }

    auto guild_get_guild_experience_summary(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_guild().guild_experience_summary());
    }

    auto guild_get_guild_skill_point(PyObject *self, PyObject *args) -> PyObject * {
        const python_guild::guild_skill_data &skill_data = get_python_guild().guild_skill_data_ref();
        return Py_BuildValue("i", skill_data.skill_point);
    }

    auto guild_get_dragon_power_point(PyObject *self, PyObject *args) -> PyObject * {
        const python_guild::guild_skill_data &skill_data = get_python_guild().guild_skill_data_ref();
        return Py_BuildValue("ii", skill_data.guild_point, skill_data.max_guild_point);
    }

    auto guild_get_skill_level(PyObject *self, PyObject *args) -> PyObject * {
        int slot_index;
        if (!py_tuple_get_integer(args, 0, &slot_index))
            return py_build_exception();

        auto itor = guild_skill_slot_to_index_map.find(slot_index);

        if (guild_skill_slot_to_index_map.end() == itor)
            return Py_BuildValue("i", 0);

        uint32_t skill_index = itor->second;
        assert(skill_index < python_guild::GUILD_SKILL_MAX_NUM);

        const python_guild::guild_skill_data &skill_data = get_python_guild().guild_skill_data_ref();
        return Py_BuildValue("i", skill_data.skill_level[skill_index]);
    }

    auto guild_set_skill_index(PyObject *self, PyObject *args) -> PyObject * {
        int slot_index;
        if (!py_tuple_get_integer(args, 0, &slot_index))
            return py_build_exception();
        int skill_index;
        if (!py_tuple_get_integer(args, 1, &skill_index))
            return py_build_exception();

        guild_skill_slot_to_index_map.insert(std::make_pair(slot_index, skill_index));

        return py_build_none();
    }

    auto guild_get_grade_data(PyObject *self, PyObject *args) -> PyObject * {
        int grade_number;
        if (!py_tuple_get_integer(args, 0, &grade_number))
            return py_build_exception();

        python_guild::guild_grade_data *data;
        if (!get_python_guild().grade_data_ptr(grade_number, &data))
            return Py_BuildValue("si", "?", 0);

        return Py_BuildValue("si", data->name_value.c_str(), data->authority_flag);
    }

    auto guild_get_grade_name(PyObject *self, PyObject *args) -> PyObject * {
        int grade_number;
        if (!py_tuple_get_integer(args, 0, &grade_number))
            return py_build_exception();

        python_guild::guild_grade_data *data;
        if (!get_python_guild().grade_data_ptr(grade_number, &data))
            return Py_BuildValue("s", "?");

        return Py_BuildValue("s", data->name_value.c_str());
    }

    auto guild_get_member_count(PyObject *self, PyObject *args) -> PyObject * {
        return Py_BuildValue("i", get_python_guild().member_count());
    }

    auto guild_get_member_data(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_build_exception();

        python_guild::guild_member_data *data;
        if (!get_python_guild().member_data_ptr(index, &data))
            return Py_BuildValue("isiiiii", -1, "", 0, 0, 0, 0, 0);

        return Py_BuildValue("isiiiii", data->pid, data->name_value.c_str(), data->grade, data->job, data->level_value, data->offer, data->general_flag);
    }

    auto guild_member_index_to_pid(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_build_exception();

        python_guild::guild_member_data *data;
        if (!get_python_guild().member_data_ptr(index, &data))
            return Py_BuildValue("i", -1);

        return Py_BuildValue("i", data->pid);
    }

    auto guild_is_member(PyObject *self, PyObject *args) -> PyObject * {
        int index;
        if (!py_tuple_get_integer(args, 0, &index))
            return py_build_exception();

        python_guild::guild_member_data *data;
        if (get_python_guild().member_data_ptr(index, &data))
            return Py_BuildValue("i", true);

        return Py_BuildValue("i", false);
    }

    auto guild_is_member_by_name(PyObject *self, PyObject *args) -> PyObject * {
        char *name;
        if (!py_tuple_get_string(args, 0, &name))
            return py_build_exception();

        python_guild::guild_member_data *data;
        if (get_python_guild().member_data_ptr_by_name(name, &data))
            return Py_BuildValue("i", true);

        return Py_BuildValue("i", false);
    }

    auto guild_main_player_has_authority(PyObject *self, PyObject *args) -> PyObject * {
        int authority;
        if (!py_tuple_get_integer(args, 0, &authority))
            return py_build_exception();

        abstract_player &player = get_abstract_player();
        const char *main_player_name = player.get_name();

        python_guild::guild_member_data *member_data;
        if (!get_python_guild().member_data_ptr_by_name(main_player_name, &member_data))
            return Py_BuildValue("i", false);

        python_guild::guild_grade_data *grade_data;
        if (!get_python_guild().grade_data_ptr(member_data->grade, &grade_data))
            return Py_BuildValue("i", false);

        return Py_BuildValue("i", authority == (grade_data->authority_flag & authority));
    }

    auto guild_destroy(PyObject *self, PyObject *args) -> PyObject * {
        get_python_guild().destroy();
        guild_skill_slot_to_index_map.clear();
        return py_build_none();
    }

    auto guild_guild_id_to_mark_id(PyObject *self, PyObject *args) -> PyObject * {
        int guild_id;
        if (!py_tuple_get_integer(args, 0, &guild_id))
            return py_build_exception();

        return Py_BuildValue("i", get_guild_mark_manager().mark_id(guild_id));
    }

    auto guild_get_mark_image_filename_by_mark_id(PyObject *self, PyObject *args) -> PyObject * {
        int mark_id;

        if (!py_tuple_get_integer(args, 0, &mark_id))
            return py_build_exception();

        std::string image_path_value;
        get_guild_mark_manager().mark_image_filename(mark_id / guild_mark_image::MARK_TOTAL_COUNT, image_path_value);
        return Py_BuildValue("s", image_path_value.c_str());
    }

    auto guild_get_mark_index_by_mark_id(PyObject *self, PyObject *args) -> PyObject * {
        int mark_id;

        if (!py_tuple_get_integer(args, 0, &mark_id))
            return py_build_exception();

        return Py_BuildValue("i", mark_id % guild_mark_image::MARK_TOTAL_COUNT);
    }

    void initguild() {
        static PyMethodDef s_methods[] =
            {
                {"IsGuildEnable", guild_is_guild_enable, METH_VARARGS},
                {"GuildIDToMarkID", guild_guild_id_to_mark_id, METH_VARARGS},
                {"GetMarkImageFilenameByMarkID", guild_get_mark_image_filename_by_mark_id, METH_VARARGS},
                {"GetMarkIndexByMarkID", guild_get_mark_index_by_mark_id, METH_VARARGS},

                {"GetGuildID", guild_get_guild_id, METH_VARARGS},
                {"HasGuildLand", guild_has_guild_land, METH_VARARGS},
                {"GetGuildName", guild_get_guild_name, METH_VARARGS},
                {"GetGuildMasterName", guild_get_guild_master_name, METH_VARARGS},
                {"GetEnemyGuildName", guild_get_enemy_guild_name, METH_VARARGS},
                {"GetGuildMoney", guild_get_guild_money, METH_VARARGS},

                {"GetGuildBoardCommentCount", guild_get_guild_board_comment_count, METH_VARARGS},
                {"GetGuildBoardCommentData", guild_get_guild_board_comment_data, METH_VARARGS},

                {"GetGuildLevel", guild_get_guild_level, METH_VARARGS},
                {"GetGuildExperience", guild_get_guild_experience, METH_VARARGS},
                {"GetGuildMemberCount", guild_get_guild_member_count, METH_VARARGS},
                {"GetGuildMemberLevelAverage", guild_get_guild_member_level_average, METH_VARARGS},
                {"GetGuildExperienceSummary", guild_get_guild_experience_summary, METH_VARARGS},

                {"GetGuildSkillPoint", guild_get_guild_skill_point, METH_VARARGS},
                {"GetDragonPowerPoint", guild_get_dragon_power_point, METH_VARARGS},
                {"GetSkillLevel", guild_get_skill_level, METH_VARARGS},

                {"SetSkillIndex", guild_set_skill_index, METH_VARARGS},

                {"GetGradeData", guild_get_grade_data, METH_VARARGS},
                {"GetGradeName", guild_get_grade_name, METH_VARARGS},

                {"GetMemberCount", guild_get_member_count, METH_VARARGS},
                {"GetMemberData", guild_get_member_data, METH_VARARGS},
                {"MemberIndexToPID", guild_member_index_to_pid, METH_VARARGS},
                {"IsMember", guild_is_member, METH_VARARGS},
                {"IsMemberByName", guild_is_member_by_name, METH_VARARGS},
                {"MainPlayerHasAuthority", guild_main_player_has_authority, METH_VARARGS},

                {"Destroy", guild_destroy, METH_VARARGS},

                {nullptr, nullptr, 0, nullptr},
        };

        PyObject *module_value = Py_InitModule("guild", s_methods);
        PyModule_AddIntConstant(module_value, "AUTH_ADD_MEMBER", GUILD_AUTH_ADD_MEMBER);
        PyModule_AddIntConstant(module_value, "AUTH_REMOVE_MEMBER", GUILD_AUTH_REMOVE_MEMBER);
        PyModule_AddIntConstant(module_value, "AUTH_NOTICE", GUILD_AUTH_NOTICE);
        PyModule_AddIntConstant(module_value, "AUTH_SKILL", GUILD_AUTH_SKILL);
        PyModule_AddIntConstant(module_value, "ENEMY_GUILD_SLOT_MAX_COUNT", python_guild::ENEMY_GUILD_SLOT_MAX_COUNT);
    }
}

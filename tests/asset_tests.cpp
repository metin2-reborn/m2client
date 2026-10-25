#include "check.h"

#include <metin2/effects/effect_data.h>
#include <metin2/files/asset_toml.h>
#include <metin2/files/eter_file_manager.h>
#include <metin2/sound/type.h>
#include <metin2/world/actors/race_motion_data.h>
#include <metin2/world/maps/property.h>

#include <string>

using namespace metin2;

TEST(canonical_name_maps_the_folders_that_left_ymir_work) {
    CHECK_EQ(eter_file_manager::canonical_name("d:\\Ymir Work\\pc\\warrior\\Hair.DDS"), std::string("character:pc/warrior/hair.png"));
    CHECK_EQ(eter_file_manager::canonical_name("D:/ymir work/monster2/wolf/wolf.gr2"), std::string("character:monster2/wolf/wolf.gr2"));
    CHECK_EQ(eter_file_manager::canonical_name("d:/ymir work/effect/etc/dropitem/dropitem.mse"), std::string("effect:etc/dropitem/dropitem.toml"));
    CHECK_EQ(eter_file_manager::canonical_name("ymir work/zone/b/obj/house.gr2"), std::string("zone:b/obj/house.gr2"));
    CHECK_EQ(eter_file_manager::canonical_name("d:/ymir work/pc/warrior/wait.msa"), std::string("character:pc/warrior/wait.toml"));
    CHECK_EQ(eter_file_manager::canonical_name("d:/ymir work/special/fog.tga"), std::string("special:fog.tga"));
    // The new names map to themselves.
    CHECK_EQ(eter_file_manager::canonical_name("character:pc/warrior/hair.png"), std::string("character:pc/warrior/hair.png"));
    CHECK_EQ(eter_file_manager::canonical_name("Effect:etc/A.dds"), std::string("effect:etc/a.png"));
}

TEST(canonical_name_keeps_other_names) {
    // Folders that stayed in the system archive, and other archives, keep their name (lower case, '/').
    CHECK_EQ(eter_file_manager::canonical_name("d:\\ymir work\\ui\\Game\\a.dds"), std::string("d:/ymir work/ui/game/a.dds"));
    CHECK_EQ(eter_file_manager::canonical_name("d:/ymir work/pcx/a.dds"), std::string("d:/ymir work/pcx/a.dds"));
    CHECK_EQ(eter_file_manager::canonical_name("textures:terrainmaps/a.dds"), std::string("textures:terrainmaps/a.dds"));
    // Scripts are TOML everywhere.
    CHECK_EQ(eter_file_manager::canonical_name("season1/monster/x/wait.msa"), std::string("season1/monster/x/wait.toml"));
}

TEST(parse_path_resolves_old_names_to_the_new_archives) {
    const auto pc = eter_file_manager::parse_path("d:\\ymir work\\pc\\warrior\\a.dds");
    CHECK_EQ(pc.archive_value, std::string("character"));
    CHECK_EQ(pc.path, std::string("pc/warrior/a.png"));
    const auto zone = eter_file_manager::parse_path("d:/ymir work/zone/a.gr2");
    CHECK_EQ(zone.archive_value, std::string("zone"));
    CHECK_EQ(zone.path, std::string("a.gr2"));
    const auto ui = eter_file_manager::parse_path("d:/ymir work/ui/a.dds");
    CHECK_EQ(ui.archive_value, std::string("system"));
    CHECK_EQ(ui.path, std::string("ymir work/ui/a.dds"));
}

TEST(sound_script_name_follows_the_old_sound_folders) {
    CHECK_EQ(eter_file_manager::sound_script_name("character:pc/warrior/wait.toml"), std::string("sound:pc/warrior/wait.toml"));
    CHECK_EQ(eter_file_manager::sound_script_name("d:/ymir work/pc/warrior/wait.msa"), std::string("sound:pc/warrior/wait.toml"));
    CHECK_EQ(eter_file_manager::sound_script_name("effect:etc/levelup_1/levelup.toml"), std::string("sound:effect/etc/levelup_1/levelup.toml"));
    CHECK(eter_file_manager::sound_script_name("season1/monster/x/wait.toml").empty());
}

TEST(asset_toml_checks_the_type) {
    CHECK(parse_asset_toml("[asset]\ntype = \"Motion\"\n", "a.toml", "Motion").has_value());
    CHECK(!parse_asset_toml("[asset]\ntype = \"Effect\"\n", "a.toml", "Motion").has_value());
    CHECK(!parse_asset_toml("[motion]\nfile = \"a.gr2\"\n", "a.toml", "Motion").has_value());
    CHECK(!parse_asset_toml("MotionFileName \"a.gr2\"\n", "a.msa", "Motion").has_value());
}

TEST(asset_table_reads_required_and_optional_values) {
    const auto table = toml::parse("a = 1.5\nb = 2\nc = true\nd = [1, 2, 3]\ne = \"Y\"\nf = 300\n");
    asset_table t(table, "t.toml");
    float a = 0.0f;
    int b = 0;
    bool c = false;
    math::vector3 d;
    int e = 0;
    float missing = 7.0f;
    CHECK(t.required("a", a));
    CHECK(t.required("b", b));
    CHECK(t.required("c", c));
    CHECK(t.required("d", d));
    constexpr std::string_view names[] = {"X", "Y"};
    CHECK(t.required_enum("e", names, e));
    CHECK(t.optional("missing", missing));
    CHECK_EQ(a, 1.5f);
    CHECK_EQ(b, 2);
    CHECK(c);
    CHECK_EQ(d.z, 3.0f);
    CHECK_EQ(e, 1);
    CHECK_EQ(missing, 7.0f);
    CHECK(t.finish()); // f is unknown: a warning only

    asset_table bad(table, "t.toml");
    uint8_t f = 0;
    CHECK(!bad.required("f", f)); // out of range
    CHECK(!bad.required("nothing", a));
    CHECK(!bad.finish());
}

namespace {
    constexpr const char *motion_text = R"([asset]
type = "Motion"

[motion]
file = "character:pc/warrior/attack.gr2"
duration = 1.5
accumulation = [0.0, -100.0, 0.0]

[combo]
pre_input_time = 0.5
direct_input_time = 0.75
input_limit_time = 1.0

[attack]
type = "Snipe"
hitting_type = "Great"
stiffen_time = 0.1
invisible_time = 0.2
external_force = 30.0
motion_type = "Combo"

[[attack.hit]]
start_time = 0.25
end_time = 0.5
bone = "equip_right_hand"
weapon_length = 150.0
positions = [
    [0.3, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0],
    [0.25, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0],
]

[loop]
count = 2
start_time = 0.1
end_time = 0.9

[[event]]
type = "Effect"
time = 0.5
effect = "d:/ymir work/pc/warrior/effect/a.mse"
position = [0.0, 30.0, 0.0]
bone = "Bip01"
attach = true
follow = true

[[event]]
type = "SpecialAttack"
time = 0.6
duration = 0.2
attack_type = "Splash"
hitting_type = "Good"
stiffen_time = 0.0
invisible_time = 0.3
external_force = 0.0
collision_type = "Splash"
spheres = [{ radius = 130.0, position = [0.0, -90.0, 120.0] }]

[[event]]
type = "ScreenWaving"
time = 0.7
duration = 0.3
power = 5

[[event]]
type = "Warp"
time = 1.0
)";
}

TEST(motion_reads_every_part) {
    race_motion_data m;
    CHECK(m.load(motion_text, "attack.toml"));
    CHECK_EQ(std::string(m.motion_file_name()), std::string("character:pc/warrior/attack.gr2"));
    CHECK_EQ(m.motion_duration(), 1.5f);
    CHECK(m.is_combo_input_time_data());
    CHECK_EQ(m.next_combo_time(), 0.75f);
    CHECK(m.is_loop_motion());
    CHECK_EQ(m.get_loop_count(), 2);

    CHECK(m.is_attacking_motion());
    const auto *attack = m.motion_attack_data_pointer();
    CHECK_EQ(attack->attack_type_value, int{race_types::ATTACK_TYPE_SNIPE});
    CHECK_EQ(attack->hitting_type, int{race_types::HIT_TYPE_GREAT});
    CHECK_EQ(attack->motion_type_value, int{race_types::MOTION_TYPE_COMBO});
    CHECK_EQ(attack->hit_limit_count, 0);
    CHECK_EQ(attack->hit_data_container_value.size(), std::size_t{1});
    const auto &hit = attack->hit_data_container_value[0];
    CHECK_EQ(hit.bone_name, std::string("equip_right_hand"));
    CHECK_EQ(hit.hit_position.size(), std::size_t{2});
    CHECK_EQ(hit.hit_position.begin()->first, 0.25f); // ordered by time
    CHECK_EQ(hit.hit_position.rbegin()->second.position_value.z, 6.0f);

    CHECK_EQ(m.motion_event_data_count(), uint32_t{4});
    CHECK(m.has_splash_motion_event());
    const race_motion_data::motion_event_data *event = nullptr;
    CHECK(m.motion_event_data_pointer(0, &event));
    const auto *effect = dynamic_cast<const race_motion_data::motion_effect_event_data *>(event);
    CHECK(effect != nullptr);
    // Old effect names are read as their new name.
    CHECK_EQ(effect->effect_file_name, std::string("character:pc/warrior/effect/a.toml"));
    CHECK(effect->is_attaching && effect->is_following && !effect->is_independent);
    CHECK(m.motion_event_data_pointer(1, &event));
    const auto *special = dynamic_cast<const race_motion_data::motion_attacking_event_data *>(event);
    CHECK(special != nullptr);
    CHECK(special->is_enable_hit_process);
    CHECK_EQ(special->collision_data_value.sphere_data_vector.size(), std::size_t{1});
    CHECK_EQ(special->collision_data_value.sphere_data_vector[0].attribute().radius, 130.0f);
    CHECK_EQ(m.get_event_type(2), int{race_motion_data::MOTION_EVENT_TYPE_SCREEN_WAVING});
    CHECK_EQ(m.get_event_type(3), int{race_motion_data::MOTION_EVENT_TYPE_WARP});
}

TEST(motion_rejects_bad_scripts) {
    race_motion_data no_motion;
    CHECK(!no_motion.load("[asset]\ntype = \"Motion\"\n", "a.toml"));
    race_motion_data no_duration;
    CHECK(!no_duration.load("[asset]\ntype = \"Motion\"\n[motion]\nfile = \"a.gr2\"\n", "a.toml"));
    race_motion_data bad_event;
    CHECK(!bad_event.load("[asset]\ntype = \"Motion\"\n[motion]\nfile = \"a.gr2\"\nduration = 1.0\n[[event]]\ntype = \"Dance\"\ntime = 0.0\n", "a.toml"));
    race_motion_data bad_enum;
    CHECK(!bad_enum.load("[asset]\ntype = \"Motion\"\n[motion]\nfile = \"a.gr2\"\nduration = 1.0\n[attack]\nhitting_type = \"Huge\"\nstiffen_time = 0.0\n"
                         "invisible_time = 0.0\nexternal_force = 0.0\nmotion_type = \"Normal\"\n",
        "a.toml"));
    race_motion_data effect;
    CHECK(!effect.load("[asset]\ntype = \"Effect\"\n", "a.toml"));
}

namespace {
    constexpr const char *effect_text = R"([asset]
type = "Effect"

[bounding_sphere]
radius = 50.0
center = [0.0, 0.0, 10.0]

[[particle]]
start_time = 0.1
path = [
    { time = 0.0, mode = "Bezier", position = [0.0, 0.0, 10.0], control = [1.0, 2.0, 3.0] },
    { time = 1.0, mode = "Direct", position = [0.0, 0.0, 20.0] },
]

[particle.emitter]
max_emission_count = 2
cycle_length = 1.5
cycle_loop = true
shape = "Ellipse"
radius = 20.0
velocity_events = []
emission_rate_events = [[0.0, 5.0], [0.3, 5.0]]
life_time_events = [[0.0, 0.5]]
particle_size_x_events = [[0.0, 12.0]]
particle_size_y_events = [[0.0, 12.0]]

[particle.sprite]
src_blend = 5
dest_blend = 2
billboard = "All"
rotation = "RandomDirection"
rotation_speed = 100.0
rotation_random_start = [0, 360]
stretch = false
texture_animation = "None"
texture_animation_delay = 0.03
gravity_events = [[0.0, 400.0]]
scale_x_events = [[0.0, 0.0], [1.0, 1.0]]
scale_y_events = [[0.0, 0.0], [1.0, 1.0]]
red_events = [[0.0, 1.0]]
green_events = [[0.0, 0.0]]
blue_events = [[0.0, 0.0]]
alpha_events = [[0.0, 1.0], [1.0, 0.0]]
rotation_events = [[0.0, 0.0]]
textures = []

[[mesh]]
mesh = "impact1.mde"
loop = false
frame_delay = 0.02

[[mesh.element]]
billboard = "Y"
blending = true
src_blend = 5
dest_blend = 2
texture_loop = true
texture_frame_delay = 0.02
color_factor = [0.5, 0.25, 1.0, 1.0]

[[light]]
ambient = [0.1, 0.1, 0.1, 1.0]
diffuse = [1.0, 0.5, 0.0, 1.0]
max_range = 300.0
attenuation = [0.0, 0.01, 0.0]
range_events = [[0.0, 100.0], [1.0, 300.0]]
)";
}

TEST(effect_reads_particles_meshes_and_lights) {
    effect_data e;
    CHECK(e.load(effect_text, "effect:etc/test/test.toml"));
    CHECK_EQ(e.bounding_sphere_radius(), 50.0f);
    CHECK_EQ(e.bounding_sphere_position().z, 10.0f);
    CHECK_EQ(e.particle_count(), uint32_t{1});
    CHECK_EQ(e.mesh_count(), uint32_t{1});
    CHECK_EQ(e.light_count(), uint32_t{1});

    particle_system_data *p = e.particle_pointer(0);
    CHECK_EQ(p->start_time(), 0.1f);
    math::vector3 position;
    p->get_position(0.5f, position);
    CHECK_EQ(position.x, 0.5f); // halfway along the curve that starts at the first entry
    const emitter_property *emitter = p->emitter_property_pointer();
    CHECK_EQ(emitter->max_emission_count, uint32_t{2});
    CHECK_EQ(emitter->emitter_shape, uint8_t{emitter_property::EMITTER_SHAPE_ELLIPSE});
    CHECK(emitter->time_event_emitting_velocity.empty());               // given empty
    CHECK_EQ(emitter->time_event_emitting_size.size(), std::size_t{1}); // missing: 0
    CHECK_EQ(emitter->time_event_emission_count_per_second.size(), std::size_t{2});
    const particle_property *sprite = p->particle_property_pointer();
    CHECK_EQ(sprite->rotation_type, uint8_t{particle_property::ROTATION_TYPE_RANDOM_DIRECTION});
    CHECK_EQ(sprite->rotation_random_starting_end, uint16_t{360});
    CHECK_EQ(sprite->time_event_gravity.size(), std::size_t{1});
    CHECK(sprite->time_event_air_resistance.empty());
    CHECK_EQ(sprite->time_event_color.size(), std::size_t{2}); // the times of all four channels

    effect_mesh_script *mesh = e.mesh_pointer(0);
    // A name without an archive is next to the script.
    CHECK_EQ(std::string(mesh->mesh_file_name()), std::string("effect:etc/test/impact1.mde"));
    CHECK_EQ(mesh->billboard_type(0), int{MESH_BILLBOARD_TYPE_Y});
    math::color color;
    CHECK(mesh->color_factor(0, &color));
    CHECK_EQ(color.g, 0.25f);
}

TEST(effect_rejects_bad_scripts) {
    effect_data no_sprite;
    CHECK(!no_sprite.load("[asset]\ntype = \"Effect\"\n[[particle]]\n[particle.emitter]\nmax_emission_count = 1\n", "a.toml"));
    effect_data bad_mode;
    CHECK(!bad_mode.load("[asset]\ntype = \"Effect\"\n[[mesh]]\nmesh = \"a.mde\"\nloop = false\nframe_delay = 0.1\n"
                         "path = [{ time = 0.0, mode = \"Spiral\", position = [0.0, 0.0, 0.0] }]\n",
        "a.toml"));
    effect_data motion;
    CHECK(!motion.load("[asset]\ntype = \"Motion\"\n", "a.toml"));
}

TEST(sound_script_reads_the_sounds_in_order) {
    sound_types::sound_data_vector sounds;
    CHECK(sound_types::load_sound_script("[asset]\ntype = \"SoundScript\"\n"
                                         "[[sound]]\ntime = 0.5\nfile = \"sound:pc/a.wav\"\n"
                                         "[[sound]]\ntime = 0.25\nfile = \"sound:common/b.wav\"\n",
        "a.toml", sounds));
    CHECK_EQ(sounds.size(), std::size_t{2});
    CHECK_EQ(sounds[0].time, 0.5f);
    CHECK_EQ(sounds[0].sound_file_name, std::string("sound:pc/a.wav"));
    CHECK_EQ(sounds[1].sound_file_name, std::string("sound:common/b.wav"));

    CHECK(sound_types::load_sound_script("[asset]\ntype = \"SoundScript\"\n", "a.toml", sounds));
    CHECK(sounds.empty());
}

TEST(sound_script_rejects_bad_scripts) {
    sound_types::sound_data_vector sounds;
    CHECK(!sound_types::load_sound_script("[asset]\ntype = \"Motion\"\n", "a.toml", sounds));
    CHECK(!sound_types::load_sound_script("ScriptType CharacterSoundInformation\n", "a.mss", sounds));
    CHECK(!sound_types::load_sound_script("[asset]\ntype = \"SoundScript\"\n[[sound]]\ntime = 0.5\n", "a.toml", sounds));
    CHECK(!sound_types::load_sound_script("[asset]\ntype = \"SoundScript\"\n[[sound]]\ntime = \"x\"\nfile = \"a.wav\"\n", "a.toml", sounds));
}

namespace {
    constexpr std::string_view kPropertyHeader = "[asset]\ntype = \"Property\"\n[property]\nid = 3000000000\nname = \"House\"\n";

    auto load_property(std::string_view kind) -> std::optional<property> {
        property p("property:a.toml");
        if (!p.load(std::string(kPropertyHeader) + std::string(kind))) {
            return std::nullopt;
        }
        return p;
    }
}

TEST(property_reads_every_kind) {
    const auto building = load_property("[building]\nfile = \"zone:b/house.gr2\"\nshadow = true\n");
    CHECK(building.has_value());
    CHECK_EQ(building->crc(), uint32_t{3000000000});
    CHECK_EQ(building->name(), std::string("House"));
    CHECK_EQ(building->type(), property_types::PROPERTY_TYPE_BUILDING);
    CHECK_EQ(building->building().file_name, std::string("zone:b/house.gr2"));
    CHECK_EQ(building->building().attribute_data_file_name, std::string("zone:b/house.mdatr"));
    CHECK(building->building().is_shadow_flag);

    const auto tree = load_property("[tree]\nfile = \"d:/ymir work/tree/a.spt\"\n");
    CHECK(tree.has_value());
    CHECK_EQ(tree->type(), property_types::PROPERTY_TYPE_TREE);
    CHECK_EQ(tree->tree().file_name, std::string("d:/ymir work/tree/a.spt"));

    const auto effect = load_property("[effect]\nfile = \"effect:background/a.toml\"\n");
    CHECK(effect.has_value());
    CHECK_EQ(effect->type(), property_types::PROPERTY_TYPE_EFFECT);
    CHECK_EQ(effect->effect().file_name, std::string("effect:background/a.toml"));

    const auto block = load_property("[dungeon_block]\nfile = \"zone:dungeon/a.gr2\"\n");
    CHECK(block.has_value());
    CHECK_EQ(block->type(), property_types::PROPERTY_TYPE_DUNGEON_BLOCK);
    CHECK_EQ(block->dungeon_block().attribute_data_file_name, std::string("zone:dungeon/a.mdatr"));

    const auto ambience = load_property("[ambience]\nplay_type = \"Step\"\ninterval = 2.0\ninterval_variation = 0.5\n"
                                        "sounds = [\"sound:ambience/a.wav\", \"sound:ambience/b.wav\"]\n");
    CHECK(ambience.has_value());
    CHECK_EQ(ambience->type(), property_types::PROPERTY_TYPE_AMBIENCE);
    CHECK_EQ(ambience->ambience().play_type, property_types::AMBIENCE_PLAY_TYPE_STEP);
    CHECK_EQ(ambience->ambience().play_interval, 2.0f);
    CHECK_EQ(ambience->ambience().max_volume_area_percentage, 0.0f); // missing: 0
    CHECK_EQ(ambience->ambience().ambience_sound_vector.size(), std::size_t{2});
}

TEST(property_rejects_bad_scripts) {
    CHECK(!load_property("").has_value());
    CHECK(!load_property("[tree]\nfile = \"a.spt\"\n[effect]\nfile = \"a.toml\"\n").has_value());
    CHECK(!load_property("[building]\n").has_value());
    CHECK(!load_property("[ambience]\nplay_type = \"LOOP\"\ninterval = 0.0\ninterval_variation = 0.0\nsounds = []\n").has_value());

    property no_id("property:a.toml");
    CHECK(!no_id.load("[asset]\ntype = \"Property\"\n[property]\nname = \"a\"\n[tree]\nfile = \"a.spt\"\n"));
    property old("property:a.prb");
    CHECK(!old.load("YPRT\r\n1430790742\r\npropertytype\t\"Tree\"\r\n"));
}

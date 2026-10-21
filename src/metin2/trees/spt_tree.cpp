// Reads SpeedTree 1.x parameter files (.spt) and generates the trees' branch, frond and leaf
// geometry.
//
// A .spt file ("__IdvSpt_02_") is a stream of 32-bit tokens, each followed by a payload whose type
// depends on the token. It holds generation parameters, not meshes. The generation model follows
// the SpeedTree 4.1 SDK sources (used as a reference at the project owner's request; this file is
// an independent implementation, no SDK code is copied):
//
// - Each branch grows along its local +X axis in segments. Its length, radius, start angle and
//   gravity come from Bezier splines evaluated at its position on the parent (normalized to the
//   parent's first..last child range) and are scaled by the tree size.
// - The start angle tilts a child away from its parent's axis after a random azimuth around it;
//   every segment is then bent toward the ground by gravity (signed and shaped by the angle
//   profile) and jittered by the disturbance spline.
// - A branch has int(frequency / size * length) children; the last level's children are buds,
//   which place leaves, thinned so that no two leaves are closer than the spacing tolerance.
// - Branches at or above the frond level become fronds (crossed blades or an extruded profile)
//   instead of bark tubes.
//
// The client uses static lighting, no wind and a single level of detail; see PORTING.md, section 7.
#include "spt_tree.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <utility>

namespace metin2::inline trees {

    namespace {

        // ------------------------------------------------------------------ math

        struct vec3 {
            float x = 0, y = 0, z = 0;
        };

        inline auto make(float x, float y, float z) -> vec3 { return {x, y, z}; }
        inline auto operator+(vec3 a, vec3 b) -> vec3 { return make(a.x + b.x, a.y + b.y, a.z + b.z); }
        inline auto operator-(vec3 a, vec3 b) -> vec3 { return make(a.x - b.x, a.y - b.y, a.z - b.z); }
        inline auto operator*(vec3 a, float s) -> vec3 { return make(a.x * s, a.y * s, a.z * s); }
        inline auto dot(vec3 a, vec3 b) -> float { return a.x * b.x + a.y * b.y + a.z * b.z; }
        inline auto cross(vec3 a, vec3 b) -> vec3 { return make(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }
        inline auto length(vec3 a) -> float { return std::sqrt(dot(a, a)); }
        inline auto normalize(vec3 a, vec3 fallback = make(0, 0, 1)) -> vec3 {
            float l = length(a);
            return l > 1e-6f ? a * (1.0f / l) : fallback;
        }
        inline auto lerp(vec3 a, vec3 b, float t) -> vec3 { return a + (b - a) * t; }
        inline auto lerpf(float a, float b, float t) -> float { return a + (b - a) * t; }

        constexpr float pi = 3.14159265358979f;
        inline auto rad(float deg) -> float { return deg * pi / 180.0f; }
        inline auto deg(float r) -> float { return r * 180.0f / pi; }
        inline auto angle_between_deg(vec3 a, vec3 b) -> float {
            float c = dot(normalize(a), normalize(b));
            return deg(std::acos(std::clamp(c, -1.0f, 1.0f)));
        }

        const vec3 out_axis = make(1, 0, 0); // a branch grows along its local +X axis
        const vec3 down = make(0, 0, -1);

        // 3x3 rotation with row vectors (v' = v * M). The rotate_* functions apply a rotation in the
        // current local frame (M = R * M); multiplying by a matrix on the right applies it in world space.
        struct mat3 {
            float m[3][3] = {
                {1, 0, 0},
                {0, 1, 0},
                {0, 0, 1}
            };

            static auto mul(const mat3 &a, const mat3 &b) -> mat3 {
                mat3 r;
                for (int i = 0; i < 3; ++i)
                    for (int j = 0; j < 3; ++j)
                        r.m[i][j] = a.m[i][0] * b.m[0][j] + a.m[i][1] * b.m[1][j] + a.m[i][2] * b.m[2][j];
                return r;
            }
            auto row(int i) const -> vec3 { return make(m[i][0], m[i][1], m[i][2]); }
            // column-vector product (M * v)
            auto apply(vec3 v) const -> vec3 {
                return make(m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z, m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
                    m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z);
            }
            static auto axis(float degrees, vec3 a) -> mat3 {
                float s = std::sin(rad(degrees)), c = std::cos(rad(degrees)), t = 1 - c;
                mat3 r;
                r.m[0][0] = t * a.x * a.x + c;
                r.m[0][1] = t * a.x * a.y + s * a.z;
                r.m[0][2] = t * a.x * a.z - s * a.y;
                r.m[1][0] = t * a.x * a.y - s * a.z;
                r.m[1][1] = t * a.y * a.y + c;
                r.m[1][2] = t * a.y * a.z + s * a.x;
                r.m[2][0] = t * a.x * a.z + s * a.y;
                r.m[2][1] = t * a.y * a.z - s * a.x;
                r.m[2][2] = t * a.z * a.z + c;
                return r;
            }
            void rotate_axis(float degrees, vec3 a) { *this = mul(axis(degrees, a), *this); }
            void rotate_x(float degrees) {
                float s = std::sin(rad(degrees)), c = std::cos(rad(degrees));
                mat3 r;
                r.m[1][1] = c;
                r.m[1][2] = s;
                r.m[2][1] = -s;
                r.m[2][2] = c;
                *this = mul(r, *this);
            }
            void rotate_y(float degrees) {
                float s = std::sin(rad(degrees)), c = std::cos(rad(degrees));
                mat3 r;
                r.m[0][0] = c;
                r.m[0][2] = -s;
                r.m[2][0] = s;
                r.m[2][2] = c;
                *this = mul(r, *this);
            }
            void rotate_yz(float y, float z) {
                float sy = std::sin(rad(y)), cy = std::cos(rad(y)), sz = std::sin(rad(z)), cz = std::cos(rad(z));
                mat3 r;
                r.m[0][0] = cy * cz;
                r.m[0][1] = cy * sz;
                r.m[0][2] = -sy;
                r.m[1][0] = -sz;
                r.m[1][1] = cz;
                r.m[1][2] = 0;
                r.m[2][0] = sy * cz;
                r.m[2][1] = sy * sz;
                r.m[2][2] = cy;
                *this = mul(r, *this);
            }
        };

        inline auto operator*(vec3 v, const mat3 &t) -> vec3 {
            return make(v.x * t.m[0][0] + v.y * t.m[1][0] + v.z * t.m[2][0], v.x * t.m[0][1] + v.y * t.m[1][1] + v.z * t.m[2][1],
                v.x * t.m[0][2] + v.y * t.m[1][2] + v.z * t.m[2][2]);
        }

        // xorshift32; the generated shapes depend on its exact sequence.
        struct xorshift {
            uint32_t s = 1;
            explicit xorshift(uint32_t seed) { reseed(seed); }
            void reseed(uint32_t seed) {
                s = seed * 2654435761u + 0x9e3779b9u;
                if (!s) s = 1;
                next();
                next();
            }
            auto next() -> uint32_t {
                s ^= s << 13;
                s ^= s >> 17;
                s ^= s << 5;
                return s;
            }
            auto unit() -> float { return float(next() >> 8) * (1.0f / 16777216.0f); } // [0, 1)
            auto uniform(float a, float b) -> float { return a + (b - a) * unit(); }
        };

        // ------------------------------------------------------------------ splines

        // "BezierSpline <min> <max> <variance> { <n> <x> <y> <dx> <dy> <tangent length> ... }": cubic Bezier
        // segments between control points, with handles along each point's tangent. Evaluating at t returns
        // curve(t) * (max - min) + min plus a uniform random offset of up to +-variance (or +-variance *
        // curve(t) for "scaled" variance).
        struct spline {
            float min = 0, max = 0, var = 0;
            std::vector<std::pair<float, float>> table; // (x, y) sampled densely, x ascending

            auto curve(float t) const -> float {
                if (table.empty()) return 1.0f;
                t = std::clamp(t, 0.0f, 1.0f);
                if (t <= table.front().first) return table.front().second;
                if (t >= table.back().first) return table.back().second;
                auto it = std::lower_bound(table.begin(), table.end(), t, [](const std::pair<float, float> &p, float v) { return p.first < v; });
                const auto &b = *it;
                const auto &a = *(it - 1);
                float span = b.first - a.first;
                return span > 1e-7f ? lerpf(a.second, b.second, (t - a.first) / span) : b.second;
            }
            auto mean(float t) const -> float { return curve(t) * (max - min) + min; }
            auto eval(float t, xorshift &rng, bool scaled_variance = false) const -> float {
                float c = curve(t);
                float v = c * (max - min) + min;
                return v + (scaled_variance ? c : 1.0f) * rng.uniform(-var, var);
            }
            auto scaled_variance(float t, xorshift &rng) const -> float {
                float c = curve(t);
                return rng.uniform(-var * c, var * c);
            }
        };

        auto parse_spline(const std::string &text, spline &out) -> bool {
            const char *p = text.c_str();
            if (strncmp(p, "BezierSpline", 12) != 0) return false;
            p += 12;
            char *end;
            out.min = strtof(p, &end);
            p = end;
            out.max = strtof(p, &end);
            p = end;
            out.var = strtof(p, &end);
            p = end;
            while (*p && *p != '{')
                ++p;
            if (!*p) return false;
            ++p;
            long n = strtol(p, &end, 10);
            p = end;
            struct point {
                float x, y, tx, ty, len;
            };
            std::vector<point> pts;
            for (long i = 0; i < n; ++i) {
                float v[5];
                for (float &f : v) {
                    f = strtof(p, &end);
                    if (end == p) return false;
                    p = end;
                }
                float l = std::sqrt(v[2] * v[2] + v[3] * v[3]);
                if (l > 1e-6f) {
                    v[2] /= l;
                    v[3] /= l;
                }
                pts.push_back({v[0], v[1], v[2], v[3], v[4]});
            }
            if (pts.empty()) return false;
            out.table.clear();
            if (pts.size() == 1) {
                out.table.emplace_back(0.0f, pts[0].y);
                out.table.emplace_back(1.0f, pts[0].y);
                return true;
            }
            const int steps = 48;
            for (size_t i = 0; i + 1 < pts.size(); ++i) {
                const point &a = pts[i];
                const point &b = pts[i + 1];
                float p0[2] = {a.x, a.y}, p1[2] = {a.x + a.tx * a.len, a.y + a.ty * a.len};
                float p2[2] = {b.x - b.tx * b.len, b.y - b.ty * b.len}, p3[2] = {b.x, b.y};
                for (int k = (i == 0 ? 0 : 1); k <= steps; ++k) {
                    float u = float(k) / steps, w = 1 - u;
                    float x = w * w * w * p0[0] + 3 * w * w * u * p1[0] + 3 * w * u * u * p2[0] + u * u * u * p3[0];
                    float y = w * w * w * p0[1] + 3 * w * w * u * p1[1] + 3 * w * u * u * p2[1] + u * u * u * p3[1];
                    if (!out.table.empty() && x <= out.table.back().first) continue; // keep x strictly ascending
                    out.table.emplace_back(x, y);
                }
            }
            return true;
        }

        // ------------------------------------------------------------------ the .spt description

        struct level_desc {
            spline disturbance, gravity, flexibility, flexibility_scale, length, radius, radius_scale, start_angle, angle_profile;
            int cross_segments = 6, segments = 3;
            float first = 0.3f, last = 1.0f, frequency = 0.3f, s_tile = 1, t_tile = 1;
            bool s_tile_absolute = false, t_tile_absolute = false;
        };

        struct leaf_texture {
            bool blossom = false;
            float color[3] = {1, 1, 1};
            float color_variance = 0;
            float origin[2] = {0.5f, 0.5f};
            float size[2] = {0.1f, 0.1f}; // fraction of the tree size
            float uv[8] = {1, 1, 0, 1, 0, 0, 1, 0};
        };

        struct material {
            float diffuse[3] = {1, 1, 1}, ambient[3] = {0.5f, 0.5f, 0.5f}, emissive[3] = {0, 0, 0};
        };

        struct frond_texture {
            float aspect = 1, size_scale = 1, min_angle = 0, max_angle = 0;
            float uv[8] = {1, 1, 0, 1, 0, 0, 1, 0};
        };

        struct tree_desc {
            std::string bark_texture, composite_texture, shadow_texture;
            float size = 1000, size_variance = 0;
            unsigned int seed = 1;
            std::vector<level_desc> levels;
            // leaves
            std::vector<leaf_texture> leaves;
            float blossom_distance = 0.75f, blossom_weighting = 0.8f, spacing_tolerance = 0.5f, dimming_scalar = 1.0f;
            int collision = 2, dimming_depth = 1;
            bool dimming = true;
            float leaf_lighting_adjustment = 0.5f;
            int static_lighting_style = 0;
            material branch_material, leaf_material, frond_material;
            // fronds
            bool frond_enabled = false;
            int frond_level = 1, frond_type = 1, frond_blades = 2, frond_profile_segments = 4;
            spline frond_profile;
            std::vector<frond_texture> fronds;
            // collision objects
            std::vector<std::vector<float>> capsules; // x y z radius height
        };

        auto base_name(const std::string &path) -> std::string {
            size_t i = path.find_last_of("\\/");
            return i == std::string::npos ? path : path.substr(i + 1);
        }

        class tag_reader {
          public:
            explicit tag_reader(std::span<const uint8_t> data) : data_(data) {}
            auto at_end() const -> bool { return pos_ >= data_.size(); }
            auto u32() -> uint32_t {
                uint32_t v = 0;
                if (pos_ + 4 <= data_.size()) memcpy(&v, data_.data() + pos_, 4);
                pos_ += 4;
                return v;
            }
            auto f32() -> float {
                float v = 0;
                if (pos_ + 4 <= data_.size()) memcpy(&v, data_.data() + pos_, 4);
                pos_ += 4;
                return v;
            }
            auto boolean() -> bool {
                bool v = pos_ < data_.size() && data_[pos_];
                pos_ += 1;
                return v;
            }
            auto str() -> std::string {
                uint32_t n = u32();
                if (pos_ + n > data_.size()) {
                    pos_ = data_.size() + 1;
                    return {};
                }
                std::string s(reinterpret_cast<const char *>(data_.data() + pos_), n);
                pos_ += n;
                return s;
            }
            void skip(size_t k) { pos_ += k; }
            auto overrun() const -> bool { return pos_ > data_.size(); }

          private:
            std::span<const uint8_t> data_;
            size_t pos_ = 0;
        };

        auto parse_spt(std::span<const uint8_t> data, tree_desc &t, std::string &error) -> bool {
            tag_reader r(data);
            if (r.u32() != 1000 || r.str() != "__IdvSpt_02_") {
                error = "not a SpeedTree 1.x (.spt) file";
                return false;
            }
            std::vector<std::vector<float>> leaf_uv, frond_uv;
            auto level = [&]() -> level_desc & { if (t.levels.empty()) t.levels.emplace_back(); return t.levels.back(); };
            auto leaf = [&]() -> leaf_texture & { if (t.leaves.empty()) t.leaves.emplace_back(); return t.leaves.back(); };
            auto frond = [&]() -> frond_texture & { if (t.fronds.empty()) t.fronds.emplace_back(); return t.fronds.back(); };
            auto read_spline = [&](spline &s) { if (!parse_spline(r.str(), s)) error = "bad spline"; };
            auto read_material = [&](material &m) {
                float v[13];
                for (float &f : v)
                    f = r.f32();
                std::copy(v, v + 3, m.diffuse);
                std::copy(v + 3, v + 6, m.ambient);
                std::copy(v + 9, v + 12, m.emissive);
            };
            while (!r.at_end()) {
                uint32_t tag = r.u32();
                // clang-format off
                switch (tag) {
                // block markers without payload
                case 1001: case 1002: case 1003: case 1004: case 1005: case 1008: case 1009: case 1010: case 1011: case 1012:
                case 1013: case 1015: case 1017: case 8000: case 8001: case 9000: case 9001: case 9005: case 9006: case 10000:
                case 10001: case 11000: case 11001: case 12000: case 12001: case 13000: case 13001: case 14001: case 15000:
                case 15001: case 16000: case 16001: case 18000: case 18001: case 19000: case 19001: case 20000: case 20001:
                    break;
                case 1007: t.leaves.emplace_back(); break;  // a leaf texture starts
                case 1016: t.levels.emplace_back(); break;  // a branch level starts
                case 14000: t.fronds.emplace_back(); break; // a frond texture starts
                // tree
                case 2000: t.bark_texture = base_name(r.str()); break;
                case 2001: case 2003: r.f32(); break; // far / near LOD distances
                case 2002: r.boolean(); break;        // LOD enabled
                case 2004: r.u32(); break;            // render style
                case 2005: t.seed = r.u32(); break;
                case 2006: t.size = r.f32(); break;
                case 2007: t.size_variance = r.f32(); break;
                // general leaf information
                case 3000: t.blossom_distance = r.f32(); break;
                case 3001: t.dimming_depth = int(r.u32()); break; // blossom level, also the dimming depth
                case 3002: t.blossom_weighting = r.f32(); break;
                case 3003: case 3006: r.boolean(); break;
                case 3004: case 3005: r.f32(); break;
                case 3007: t.spacing_tolerance = r.f32(); break;
                case 3008: t.collision = int(r.u32()); break;
                case 3009: t.dimming = r.boolean(); break;
                case 3010: t.dimming_scalar = r.f32(); break;
                case 1006: r.u32(); break; // number of leaf textures
                // single leaf texture
                case 4000: leaf().blossom = r.boolean(); break;
                case 4001: for (float &f : leaf().color) f = r.f32(); break;
                case 4002: leaf().color_variance = r.f32(); break;
                case 4003: r.str(); break; // file name (the leaves come from the composite map)
                case 4004: { float v[3]; for (float &f : v) f = r.f32(); leaf().origin[0] = v[0]; leaf().origin[1] = v[1]; break; }
                case 4005: { float v[3]; for (float &f : v) f = r.f32(); leaf().size[0] = v[0]; leaf().size[1] = v[1]; break; }
                case 4006: r.skip(12); break; // size used (recomputed from the tree size)
                case 4007: r.f32(); break;
                // old wind information
                case 5000: case 5001: case 5002: case 5003: case 5004: r.skip(12); break;
                case 5005: r.f32(); break;
                case 5006: r.boolean(); break;
                // branch level
                case 1014: r.u32(); break; // number of branch levels
                case 6000: read_spline(level().disturbance); break;
                case 6001: read_spline(level().gravity); break;
                case 6002: read_spline(level().flexibility); break;
                case 6003: read_spline(level().flexibility_scale); break;
                case 6004: read_spline(level().length); break;
                case 6005: read_spline(level().radius); break;
                case 6006: read_spline(level().radius_scale); break;
                case 6007: read_spline(level().start_angle); break;
                case 6008: level().cross_segments = int(r.u32()); break;
                case 6009: level().segments = int(r.u32()); break;
                case 6010: level().first = r.f32(); break;
                case 6011: level().last = r.f32(); break;
                case 6012: level().frequency = r.f32(); break;
                case 6013: level().s_tile = r.f32(); break;
                case 6014: level().t_tile = r.f32(); break;
                case 6015: level().s_tile_absolute = r.boolean(); break;
                case 6016: level().t_tile_absolute = r.boolean(); break;
                case 6017: read_spline(level().angle_profile); break;
                // lighting
                case 8002: case 8004: case 8008: r.u32(); break; // lighting methods
                case 8003: read_material(t.branch_material); break;
                case 8005: read_material(t.leaf_material); break;
                case 8006: t.leaf_lighting_adjustment = r.f32(); break;
                case 8007: t.static_lighting_style = int(r.u32()); break;
                case 8009: read_material(t.frond_material); break;
                // texture coordinates in the composite map
                case 10002: case 10003: case 10004: {
                    uint32_t k = r.u32();
                    if (k > 256) {
                        error = "bad texture coordinate table";
                        return false;
                    }
                    for (uint32_t i = 0; i < k; ++i) {
                        std::vector<float> v(8);
                        for (float &f : v) f = r.f32();
                        if (tag == 10002) leaf_uv.push_back(v);
                        else if (tag == 10004) frond_uv.push_back(v);
                    }
                    break;
                }
                case 10005: t.composite_texture = base_name(r.str()); break;
                case 10006: case 10007: r.boolean(); break;
                // collision objects
                case 12002: r.skip(16); break; // spheres
                case 12003: { std::vector<float> v(5); for (float &f : v) f = r.f32(); t.capsules.push_back(v); break; }
                case 12004: r.skip(24); break; // boxes
                // fronds
                case 13002: t.frond_level = int(r.u32()); break;
                case 13003: t.frond_type = int(r.u32()); break;
                case 13004: t.frond_blades = int(r.u32()); break;
                case 13005: read_spline(t.frond_profile); break;
                case 13006: t.frond_profile_segments = int(r.u32()); break;
                case 13007: t.frond_enabled = r.boolean(); break;
                case 13008: case 13009: r.u32(); break;
                case 13010: case 13011: case 13012: case 13013: r.f32(); break;
                case 14002: r.str(); break; // file name (the fronds come from the composite map)
                case 14003: frond().aspect = r.f32(); break;
                case 14004: frond().size_scale = r.f32(); break;
                case 14005: frond().min_angle = r.f32(); break;
                case 14006: frond().max_angle = r.f32(); break;
                case 14007: case 14008: r.u32(); break;
                // texture controls, flares, shadow projection, user data
                case 15002: r.boolean(); break;
                case 15003: r.f32(); break;
                case 18002: case 18003: case 18004: r.skip(12); break;
                case 18005: t.shadow_texture = base_name(r.str()); break;
                case 19002: r.str(); break;
                case 20002: t.composite_texture = base_name(r.str()); break;
                case 20003: case 20004: r.boolean(); break;
                case 20005: r.skip(32); break;
                default: // remaining tokens of the old format carry one 32-bit value (wind, LOD, flare settings)
                    r.skip(4);
                    break;
                }
                // clang-format on
                if (r.overrun()) {
                    error = "truncated .spt file";
                    return false;
                }
            }
            for (size_t i = 0; i < t.leaves.size() && i < leaf_uv.size(); ++i)
                std::copy(leaf_uv[i].begin(), leaf_uv[i].end(), t.leaves[i].uv);
            for (size_t i = 0; i < t.fronds.size() && i < frond_uv.size(); ++i)
                std::copy(frond_uv[i].begin(), frond_uv[i].end(), t.fronds[i].uv);
            if (t.levels.empty() || t.size <= 0) {
                error = "the tree has no branch levels";
                return false;
            }
            if (t.leaves.size() > 64) {
                error = "more than 64 leaf textures";
                return false;
            }
            return true;
        }

        // ------------------------------------------------------------------ generation

        // The forest's light: static lighting is baked into the vertex colours with it.
        const vec3 light_direction = normalize(make(-0.707f, -0.3f, 0.707f));
        constexpr float light_diffuse[3] = {1, 1, 1};
        constexpr float light_ambient[3] = {0.5f, 0.5f, 0.5f};

        auto argb(vec3 c) -> uint32_t {
            auto q = [](float v) { return uint32_t(std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f); };
            return 0xff000000u | (q(c.x) << 16) | (q(c.y) << 8) | q(c.z);
        }

        // Static lighting of branches and fronds: emissive + ambient + diffuse * N.L.
        auto standard_lighting(const material &m, vec3 n) -> vec3 {
            float d = std::max(0.0f, dot(normalize(n), light_direction));
            return make(m.emissive[0] + light_ambient[0] * m.ambient[0] + d * light_diffuse[0] * m.diffuse[0],
                m.emissive[1] + light_ambient[1] * m.ambient[1] + d * light_diffuse[1] * m.diffuse[1],
                m.emissive[2] + light_ambient[2] * m.ambient[2] + d * light_diffuse[2] * m.diffuse[2]);
        }

        // The renderer's texture coordinates run top-down.
        auto flip_v(float v) -> float { return 1.0f - v; }

        struct node {
            vec3 pos, dir;
            mat3 trans;
            float radius = 0, running = 0;
        };

        struct branch {
            int level = 0;
            float length = 0, percent_of_parent = 0;
            const branch *parent = nullptr;
            std::vector<node> v;

            // the node segment holding the point at distance d along the branch
            void locate(float d, size_t &i, float &f) const {
                i = 0;
                while (i + 2 < v.size() && v[i + 1].running < d)
                    ++i;
                float span = v[i + 1].running - v[i].running;
                f = span > 1e-6f ? std::clamp((d - v[i].running) / span, 0.0f, 1.0f) : 0.0f;
            }
        };

        struct frond_guide {
            std::vector<node> v;
            float length = 0, radius = 0, offset_angle = 0;
            int texture = 0;
        };

        struct grown_leaf {
            vec3 pos;
            vec3 color;
            int map = 0;
        };

        constexpr size_t max_vertices = 65000;
        constexpr size_t max_indices = 65000;
        constexpr size_t max_leaves = 8000;
    }

    class spt_generator {
      public:
        spt_generator(spt_tree &tree, const tree_desc &desc, float size, unsigned int seed) : tree_(tree), t_(desc), size_(size), rng_(seed), seed_(seed) {
            for (size_t i = 0; i < t_.leaves.size(); ++i)
                (t_.leaves[i].blossom ? blossoms_ : regular_).push_back(int(i));
            for (const leaf_texture &l : t_.leaves) {
                tree_.leaf_sizes_.insert(tree_.leaf_sizes_.end(), {l.size[0] * t_.size, l.size[1] * t_.size});
                tree_.leaf_origins_.insert(tree_.leaf_origins_.end(), {l.origin[0], l.origin[1]});
            }
        }

        void run() {
            mat3 initial;
            grow(nullptr, 0, make(0, 0, 0), 0.0f, initial, make(0, 0, 1), -1.0f);
            for (const frond_guide &g : guides_) {
                if (t_.frond_type == 0)
                    add_blades(g);
                else
                    add_extrusion(g);
            }
        }

        std::vector<grown_leaf> leaves;

      private:
        auto leaf_side(int texture) const -> float {
            return std::max(tree_.leaf_sizes_[size_t(texture) * 2], tree_.leaf_sizes_[size_t(texture) * 2 + 1]) * t_.spacing_tolerance;
        }

        // Grow one branch (or frond guide) and, recursively, its children.
        void grow(const branch *parent, int lv, vec3 base, float pct, const mat3 &base_trans, vec3 z_axis, float parent_radius) {
            const level_desc &L = t_.levels[size_t(lv)];
            bool frond = t_.frond_enabled && lv >= t_.frond_level;

            branch b;
            b.level = lv;
            b.parent = parent;
            b.percent_of_parent = pct;
            float len = L.length.eval(pct, rng_) * size_;
            float azimuth = rng_.uniform(-180.0f, 180.0f);
            float start_angle = L.start_angle.eval(pct, rng_);
            float gravity = L.gravity.eval(pct, rng_);
            float radius = std::fabs(L.radius.eval(pct, rng_) * size_);
            float max_radius = L.radius.max * size_;
            if (lv > 0 && parent_radius > 0.0f && parent_radius * 0.85f < max_radius) max_radius = parent_radius * 0.85f;
            radius = std::min(radius, max_radius);
            int count = std::max(L.segments, 1) + 1;
            if (len <= 0.0f) return;

            b.length = len;
            b.v.resize(size_t(count));
            node &v0 = b.v[0];
            v0.pos = base;
            v0.trans = base_trans;
            v0.trans.rotate_axis(azimuth, base_trans.apply(z_axis));
            float az_dist = L.disturbance.scaled_variance(0.0f, rng_);
            float pitch_dist = L.disturbance.scaled_variance(0.0f, rng_);
            v0.trans.rotate_yz(start_angle + pitch_dist, az_dist);
            v0.radius = radius * L.radius_scale.eval(0.0f, rng_, true);
            apply_gravity(v0, L, gravity, 0.0f);

            float running = 0;
            for (int i = 1; i < count; ++i) {
                float progress = float(i) / float(count - 1);
                node &v = b.v[size_t(i)];
                const node &prev = b.v[size_t(i - 1)];
                float seg = progress * len - running;
                v.radius = radius * L.radius_scale.eval(progress, rng_, true);
                v.trans = prev.trans;
                v.dir = out_axis * v.trans;
                apply_gravity(v, L, gravity, progress);
                float pd = L.disturbance.scaled_variance(progress, rng_);
                float ad = L.disturbance.scaled_variance(progress, rng_);
                v.trans.rotate_yz(pd, ad);
                v.dir = out_axis * v.trans;
                v.pos = prev.pos + prev.dir * seg;
                running += seg;
                v.running = running;
            }

            if (frond) {
                frond_guide g;
                g.v = b.v;
                for (size_t i = 0; i + 1 < g.v.size(); ++i)
                    g.length += length(g.v[i + 1].pos - g.v[i].pos);
                if (!t_.fronds.empty()) {
                    g.texture = int(rng_.next() % t_.fronds.size());
                    const frond_texture &ft = t_.fronds[size_t(g.texture)];
                    g.radius = g.length * ft.aspect * 0.5f * ft.size_scale;
                    g.offset_angle = rng_.uniform(ft.min_angle, ft.max_angle);
                    if (guides_.size() % 2 == 1) g.offset_angle = -g.offset_angle;
                } else {
                    g.radius = g.length * 0.5f;
                }
                guides_.push_back(std::move(g));
            } else {
                add_tube(b, L);
            }

            // children: branches of the next level, or buds (leaves) below the last level
            int children = int(L.frequency / size_ * len);
            int next = lv + 1;
            if (next >= int(t_.levels.size())) return;
            bool buds = next >= int(t_.levels.size()) - 1;
            for (int i = 0; i < children; ++i) {
                if (!buds) {
                    seed_ += 3;
                    rng_.reseed(seed_);
                }
                float p;
                if (!buds && i == 0) // one child always grows near the end of the parent
                    p = rng_.uniform(lerpf(L.first, L.last, 0.85f), lerpf(L.first, L.last, 0.95f));
                else
                    p = rng_.uniform(L.first, L.last);
                size_t k;
                float f;
                b.locate(p * len, k, f);
                vec3 pos = lerp(b.v[k].pos, b.v[k + 1].pos, f);
                float profile = L.last != L.first ? (p - L.first) / (L.last - L.first) : 1.0f;
                if (buds) {
                    bud(b, next, pos, profile, b.v[k].trans, b.v[k].dir);
                } else {
                    rng_.reseed(seed_);
                    rng_.uniform(0.0f, 100.0f);
                    float parent_r = lerpf(b.v[k].radius, b.v[k + 1].radius, f);
                    grow(&b, next, pos, profile, b.v[k].trans, b.v[k].dir, parent_r);
                }
            }
        }

        void apply_gravity(node &v, const level_desc &L, float gravity, float progress) {
            v.dir = out_axis * v.trans;
            float angle = angle_between_deg(v.dir, down);
            float facing = 1.0f - std::fabs(90.0f - angle) / 90.0f;
            vec3 axis = cross(v.dir, down);
            if (length(axis) > 1e-5f) {
                axis = normalize(axis);
                float profile = -((L.angle_profile.eval(progress, rng_) - 0.5f) * 2.0f);
                v.trans = mat3::mul(v.trans, mat3::axis(profile * gravity * angle * facing, axis));
            }
            v.dir = out_axis * v.trans;
        }

        // A bud: a short stalk at 60 degrees from the parent; a leaf sits at its end.
        void bud(const branch &parent, int lv, vec3 base, float pct, const mat3 &base_trans, vec3 z_axis) {
            const level_desc &L = t_.levels[size_t(lv)];
            float len = L.length.eval(pct, rng_) * size_;
            float seg = len / float(std::max(L.segments, 1));
            float azimuth = rng_.uniform(-180.0f, 180.0f);
            mat3 trans = base_trans;
            trans.rotate_axis(azimuth, base_trans.apply(z_axis));
            trans.rotate_y(60.0f);
            vec3 dir = out_axis * trans;
            vec3 pos = base + dir * seg;
            vec3 normal = normalize(pos - base, dir);
            // general direction of the parent: from its base (or the base of the branch dimming_depth up)
            const branch *anchor = &parent;
            for (int n = 0; anchor && n < t_.dimming_depth - 1; ++n)
                anchor = anchor->parent ? anchor->parent : anchor;
            vec3 parent_dir = anchor && !anchor->v.empty() ? normalize(pos - anchor->v[0].pos, normal) : normal;
            make_leaf(pos, pct, &parent, normal, parent_dir);
        }

        void make_leaf(vec3 pos, float pct, const branch *parent, vec3 normal, vec3 parent_dir) {
            if (t_.leaves.empty() || leaves.size() >= max_leaves) return;
            int texture;
            bool blossom = !blossoms_.empty() && is_blossom(parent, pct);
            if (blossom)
                texture = blossoms_[rng_.next() % blossoms_.size()];
            else if (!regular_.empty())
                texture = regular_[rng_.next() % regular_.size()];
            else
                return;
            int map = texture * 2 + int(rng_.next() & 1); // every texture also has a mirrored variant
            const leaf_texture &lt = t_.leaves[size_t(texture)];

            if (t_.collision != 0 && !room_for_leaf(pos, texture, parent)) return;

            float scale = 1.0f;
            if (t_.dimming) {
                float dim = pct;
                const branch *b = parent;
                for (int n = 0; b && n < t_.dimming_depth; ++n) {
                    dim *= b->percent_of_parent;
                    b = b->parent;
                }
                scale = lerpf(dim, 1.0f, 1.0f - t_.dimming_scalar);
            }
            vec3 n = normalize(lerp(parent_dir, length(normal) > 1e-4f ? normal : parent_dir, lt.color_variance), parent_dir);
            float offset = rng_.uniform(-lt.color_variance, lt.color_variance);
            vec3 color = make(lt.color[0] + offset, lt.color[1] + offset, lt.color[2] + offset);
            if (t_.static_lighting_style != 0) { // SLS_USE_LIGHT_SOURCES (and shadows, approximated by the same)
                float d = lerpf(t_.leaf_lighting_adjustment, 1.0f, std::max(0.0f, dot(n, light_direction)));
                const material &m = t_.leaf_material;
                color = make(m.emissive[0] + light_ambient[0] * m.ambient[0] + d * light_diffuse[0] * color.x,
                    m.emissive[1] + light_ambient[1] * m.ambient[1] + d * light_diffuse[1] * color.y,
                    m.emissive[2] + light_ambient[2] * m.ambient[2] + d * light_diffuse[2] * color.z);
            }
            grown_leaf l;
            l.pos = pos;
            l.color = color * scale;
            l.map = map;
            leaves.push_back(l);
            cell_ = std::max(cell_, leaf_side(texture));
            grid_[grid_key(pos)].push_back(leaves.size() - 1);
            if (t_.collision == 1) local_leaves_[parent].push_back(leaves.size() - 1);
        }

        auto is_blossom(const branch *parent, float pct) -> bool {
            float test = pct;
            if (t_.dimming_depth != 0) {
                const branch *b = parent;
                for (int n = 1; b && b->parent && n < t_.dimming_depth; ++n)
                    b = b->parent;
                if (b) test = b->percent_of_parent;
            }
            bool in_range = t_.blossom_distance < 0.0f ? test < -t_.blossom_distance : test > t_.blossom_distance;
            return in_range && rng_.unit() <= t_.blossom_weighting;
        }

        // Leaves are rejected when another leaf lies within the spacing tolerance of their larger side
        // (against the whole tree, or only the leaves of the same branch).
        auto room_for_leaf(vec3 pos, int texture, const branch *parent) -> bool {
            float side = leaf_side(texture);
            auto near = [&](size_t idx) {
                vec3 o = leaves[idx].pos;
                return std::fabs(pos.x - o.x) < side && std::fabs(pos.y - o.y) < side && std::fabs(pos.z - o.z) < side;
            };
            if (t_.collision == 1) {
                auto it = local_leaves_.find(parent);
                if (it == local_leaves_.end()) return true;
                for (size_t idx : it->second)
                    if (near(idx)) return false;
                return true;
            }
            if (leaves.empty()) return true;
            float cell = std::max(cell_, side);
            if (cell != cell_) { // rebuild the grid for a larger cell
                cell_ = cell;
                grid_.clear();
                for (size_t i = 0; i < leaves.size(); ++i)
                    grid_[grid_key(leaves[i].pos)].push_back(i);
            }
            int cx = int(std::floor(pos.x / cell_)), cy = int(std::floor(pos.y / cell_)), cz = int(std::floor(pos.z / cell_));
            for (int dx = -1; dx <= 1; ++dx)
                for (int dy = -1; dy <= 1; ++dy)
                    for (int dz = -1; dz <= 1; ++dz) {
                        auto it = grid_.find(key(cx + dx, cy + dy, cz + dz));
                        if (it == grid_.end()) continue;
                        for (size_t idx : it->second)
                            if (near(idx)) return false;
                    }
            return true;
        }

        static auto key(int x, int y, int z) -> uint64_t {
            return (uint64_t(uint32_t(x) & 0x1fffff) << 42) | (uint64_t(uint32_t(y) & 0x1fffff) << 21) | uint64_t(uint32_t(z) & 0x1fffff);
        }

        auto grid_key(vec3 p) const -> uint64_t {
            float c = cell_ > 0 ? cell_ : 1.0f;
            return key(int(std::floor(p.x / c)), int(std::floor(p.y / c)), int(std::floor(p.z / c)));
        }

        static void push_vertex(spt_tree::mesh &out, vec3 p, uint32_t color, float u, float v) {
            spt_tree::vertex &x = out.vertices.emplace_back();
            x.position[0] = p.x;
            x.position[1] = p.y;
            x.position[2] = p.z;
            x.color = color;
            x.tex_coords[0] = u;
            x.tex_coords[1] = v;
            x.shadow_coords[0] = 0.02f; // a white spot of the self-shadow map
            x.shadow_coords[1] = 0.5f;
        }

        void add_tube(const branch &b, const level_desc &L) {
            spt_tree::mesh &out = tree_.branches_;
            int c = L.cross_segments;
            if (c < 3) return;
            size_t base = out.vertices.size();
            size_t rows = b.v.size();
            if (base + rows * size_t(c + 1) > max_vertices || out.strip.size() + rows * size_t(2 * (c + 1) + 2) > max_indices) return;
            float base_radius = b.v[0].radius;
            float s_tile = L.s_tile_absolute ? L.s_tile : L.s_tile * base_radius * 2.0f * pi;
            float t_tile = L.t_tile_absolute ? L.t_tile : L.t_tile * (b.length / size_);
            for (size_t i = 0; i < rows; ++i) {
                const node &v = b.v[i];
                float progress = rows > 1 ? float(i) / float(rows - 1) : 0.0f;
                vec3 r1 = v.trans.row(1), r2 = v.trans.row(2);
                for (int j = 0; j <= c; ++j) {
                    float around = float(j) / float(c);
                    float a = 2.0f * pi * around;
                    vec3 n = r1 * std::cos(a) + r2 * std::sin(a);
                    vec3 p = j == c ? v.pos + r1 * v.radius : v.pos + n * v.radius;
                    push_vertex(out, p, argb(standard_lighting(t_.branch_material, n)), around * s_tile, flip_v(progress * t_tile));
                }
            }
            for (size_t i = 0; i + 1 < rows; ++i) {
                if (!out.strip.empty()) { // degenerate join keeps the winding parity
                    out.strip.push_back(out.strip.back());
                    out.strip.push_back(uint16_t(base + (i + 1) * size_t(c + 1)));
                }
                for (int j = 0; j <= c; ++j) {
                    out.strip.push_back(uint16_t(base + (i + 1) * size_t(c + 1) + size_t(j)));
                    out.strip.push_back(uint16_t(base + i * size_t(c + 1) + size_t(j)));
                }
            }
        }

        // Map a frond-local (s, t) into the frond texture's rectangle of the composite map.
        void push_frond_vertex(vec3 p, vec3 n, float s, float t, const frond_guide &g) {
            static const float default_uv[8] = {1, 1, 0, 1, 0, 0, 1, 0};
            const float *c = t_.fronds.empty() ? default_uv : t_.fronds[size_t(g.texture)].uv;
            push_vertex(tree_.fronds_, p, argb(standard_lighting(t_.frond_material, n)), lerpf(c[2], c[0], s), flip_v(lerpf(c[5], c[1], t)));
        }

        void add_blades(const frond_guide &g) {
            spt_tree::mesh &out = tree_.fronds_;
            int blades = std::max(1, t_.frond_blades);
            size_t n = g.v.size();
            if (n < 2) return;
            float between = 180.0f / float(blades);
            for (int k = 0; k < blades; ++k) {
                size_t base = out.vertices.size();
                if (base + 2 * n > max_vertices || out.strip.size() + 2 * n + 2 > max_indices) return;
                std::vector<vec3> right(n), side0(n), side1(n);
                for (size_t i = 0; i < n; ++i) {
                    mat3 t = g.v[i + 1 < n ? i : i - 1].trans;
                    t.rotate_x(between * float(k) + g.offset_angle);
                    right[i] = make(0, 1, 0) * t;
                    vec3 r = right[i];
                    if (i > 0 && i + 1 < n)
                        r = normalize((right[i] + right[i - 1]) * 0.5f, right[i]);
                    else if (i + 1 == n)
                        r = right[i - 1];
                    side0[i] = g.v[i].pos + r * g.radius;
                    side1[i] = g.v[i].pos - r * g.radius;
                }
                // texture t along each edge by running length ("unpinched")
                float len_r = 0, len_l = 0;
                std::vector<float> run_r(n, 0), run_l(n, 0);
                for (size_t i = 1; i < n; ++i) {
                    len_r += length(side0[i] - side0[i - 1]);
                    run_r[i] = len_r;
                    len_l += length(side1[i] - side1[i - 1]);
                    run_l[i] = len_l;
                }
                for (size_t i = 0; i < n; ++i) {
                    vec3 normal = normalize(i == 0 ? g.v[1].pos - g.v[0].pos : g.v[i].pos - g.v[0].pos);
                    float t_r = len_r > 0 ? 0.999f * run_r[i] / len_r : 0, t_l = len_l > 0 ? 0.999f * run_l[i] / len_l : 0;
                    push_frond_vertex(side0[i], normal, 1.0f, t_r, g);
                    push_frond_vertex(side1[i], normal, 0.0f, t_l, g);
                }
                if (!out.strip.empty()) {
                    out.strip.push_back(out.strip.back());
                    out.strip.push_back(uint16_t(base));
                }
                for (size_t i = 0; i < 2 * n; ++i)
                    out.strip.push_back(uint16_t(base + i));
            }
        }

        // An extruded frond: a cross-section profile (from the profile spline) swept along the guide.
        void add_extrusion(const frond_guide &g) {
            spt_tree::mesh &out = tree_.fronds_;
            size_t n = g.v.size();
            int segs = std::max(2, t_.frond_profile_segments);
            size_t per = size_t(segs * 2 - 1);
            size_t base = out.vertices.size();
            if (n < 2 || base + n * per > max_vertices || out.strip.size() + (n - 1) * (2 * per + 2) > max_indices) return;
            std::vector<vec3> profile(per), normals(per);
            float adjust = t_.frond_profile.mean(0.0f) * g.radius;
            size_t i = 0;
            for (; i < size_t(segs); ++i) {
                float pct = 1.0f - float(i) / float(segs - 1);
                profile[i] = make(0, -pct * g.radius, t_.frond_profile.mean(pct) * g.radius - adjust);
            }
            for (int j = segs - 2; j >= 0; --j) {
                vec3 p = profile[size_t(j)];
                p.y = -p.y;
                profile[i++] = p;
            }
            for (i = 0; i < per; ++i) {
                float a;
                auto slope = [&](size_t x, size_t y) { return std::atan2(profile[y].z - profile[x].z, profile[y].y - profile[x].y); };
                if (i == size_t(segs - 1))
                    a = pi * 0.5f;
                else if (i == 0)
                    a = slope(0, 1) + pi * 0.5f;
                else if (i == per - 1)
                    a = slope(i - 1, i) + pi * 0.5f;
                else
                    a = 0.5f * (slope(i - 1, i) + slope(i, i + 1)) + pi * 0.5f;
                normals[i] = normalize(make(0, std::cos(a), std::sin(a)));
            }
            std::vector<vec3> pos(n * per);
            for (size_t k = 0; k < n; ++k) {
                mat3 t = g.v[k].trans, tp = k ? g.v[k - 1].trans : g.v[k].trans;
                t.rotate_x(g.offset_angle);
                tp.rotate_x(g.offset_angle);
                for (size_t j = 0; j < per; ++j)
                    pos[k * per + j] = g.v[k].pos + (profile[j] * t + profile[j] * tp) * 0.5f;
            }
            std::vector<float> lengths(per, 0), running(n * per, 0);
            for (size_t j = 0; j < per; ++j)
                for (size_t k = 1; k < n; ++k) {
                    lengths[j] += length(pos[k * per + j] - pos[(k - 1) * per + j]);
                    running[k * per + j] = lengths[j];
                }
            for (size_t k = 0; k < n; ++k) {
                mat3 t = g.v[k].trans;
                t.rotate_x(g.offset_angle);
                for (size_t j = 0; j < per; ++j) {
                    float s = float(j) / float(per - 1);
                    float tc = lengths[j] > 0 ? 0.999f * running[k * per + j] / lengths[j] : 0.0f;
                    push_frond_vertex(pos[k * per + j], normalize(normals[j] * t), s, tc, g);
                }
            }
            for (size_t k = 0; k + 1 < n; ++k) {
                if (!out.strip.empty()) {
                    out.strip.push_back(out.strip.back());
                    out.strip.push_back(uint16_t(base + (k + 1) * per));
                }
                for (size_t j = 0; j < per; ++j) {
                    out.strip.push_back(uint16_t(base + (k + 1) * per + j));
                    out.strip.push_back(uint16_t(base + k * per + j));
                }
            }
        }

        spt_tree &tree_;
        const tree_desc &t_;
        float size_;
        xorshift rng_;
        unsigned int seed_;
        std::vector<int> blossoms_, regular_;
        std::vector<frond_guide> guides_;
        std::unordered_map<uint64_t, std::vector<size_t>> grid_;
        std::unordered_map<const branch *, std::vector<size_t>> local_leaves_;
        float cell_ = 0;
    };

    auto spt_tree::load(std::span<const uint8_t> data) -> std::expected<spt_tree, std::string> {
        if (data.empty()) return std::unexpected("empty tree data");
        tree_desc desc;
        std::string error;
        if (!parse_spt(data, desc, error)) return std::unexpected(error);

        // the tree's own seed decides its shape
        unsigned int seed = desc.seed ? desc.seed : 1;
        xorshift rng(seed);
        float size = std::max(1.0f, rng.uniform(desc.size - desc.size_variance, desc.size + desc.size_variance));

        spt_tree tree;
        tree.bark_texture_ = desc.bark_texture;
        tree.composite_texture_ = desc.composite_texture;
        tree.shadow_texture_ = desc.shadow_texture;

        spt_generator gen(tree, desc, size, seed);
        gen.run();

        // leaf maps: per texture a plain and a mirrored entry
        tree.leaf_maps_.resize(desc.leaves.size() * 2);
        for (size_t i = 0; i < desc.leaves.size(); ++i) {
            const float *c = desc.leaves[i].uv;
            const float plain[8] = {c[0], c[1], c[2], c[3], c[4], c[5], c[6], c[7]};
            const float mirrored[8] = {c[2], c[1], c[0], c[3], c[6], c[5], c[4], c[7]};
            for (int k = 0; k < 4; ++k) {
                tree.leaf_maps_[i * 2].tex_coords[k][0] = plain[k * 2];
                tree.leaf_maps_[i * 2].tex_coords[k][1] = flip_v(plain[k * 2 + 1]);
                tree.leaf_maps_[i * 2 + 1].tex_coords[k][0] = mirrored[k * 2];
                tree.leaf_maps_[i * 2 + 1].tex_coords[k][1] = flip_v(mirrored[k * 2 + 1]);
            }
        }

        // bounds
        vec3 lo = make(1e9f, 1e9f, 1e9f), hi = make(-1e9f, -1e9f, -1e9f);
        auto grow = [&](vec3 p) {
            lo = make(std::min(lo.x, p.x), std::min(lo.y, p.y), std::min(lo.z, p.z));
            hi = make(std::max(hi.x, p.x), std::max(hi.y, p.y), std::max(hi.z, p.z));
        };
        for (const mesh *part : {&tree.branches_, &tree.fronds_})
            for (const vertex &v : part->vertices)
                grow(make(v.position[0], v.position[1], v.position[2]));

        for (const grown_leaf &l : gen.leaves) {
            tree.leaves_.push_back({
                {l.pos.x, l.pos.y, l.pos.z},
                argb(l.color), uint8_t(l.map)
            });
            size_t tex = size_t(l.map / 2);
            float r = std::max(tree.leaf_sizes_[tex * 2], tree.leaf_sizes_[tex * 2 + 1]);
            grow(l.pos + make(r, r, r));
            grow(l.pos - make(r, r, r));
        }

        if (lo.x > hi.x) lo = hi = make(0, 0, 0);
        tree.bounds_[0] = lo.x;
        tree.bounds_[1] = lo.y;
        tree.bounds_[2] = std::min(lo.z, 0.0f);
        tree.bounds_[3] = hi.x;
        tree.bounds_[4] = hi.y;
        tree.bounds_[5] = hi.z;

        float scale = desc.size > 0 ? size / desc.size : 1.0f;
        for (const auto &c : desc.capsules)
            tree.collision_.push_back({
                {c[0] * scale, c[1] * scale, c[2] * scale},
                c[3] * scale, c[4] * scale
            });

        return tree;
    }

    void spt_tree::face_leaves(const float (&direction)[3]) {
        vec3 camera = make(direction[0], direction[1], direction[2]);
        if (faced_ && length(make(facing_[0], facing_[1], facing_[2]) - camera) < 1e-3f) return;
        std::copy(direction, direction + 3, facing_);
        faced_ = true;
        vec3 forward = normalize(camera);
        vec3 right = cross(forward, make(0, 0, 1));
        right = length(right) < 1e-3f ? make(1, 0, 0) : normalize(right);
        vec3 up = normalize(cross(right, forward));
        for (size_t m = 0; m < leaf_maps_.size(); ++m) {
            size_t tex = m / 2;
            float w = leaf_sizes_[tex * 2], h = leaf_sizes_[tex * 2 + 1];
            float ox = leaf_origins_[tex * 2], oy = leaf_origins_[tex * 2 + 1];
            if (m % 2) ox = 1.0f - ox; // the mirrored entry mirrors the origin too
            const float cx[4] = {(1 - ox) * w, -ox * w, -ox * w, (1 - ox) * w};
            const float cy[4] = {(1 - oy) * h, (1 - oy) * h, -oy * h, -oy * h};
            for (int k = 0; k < 4; ++k) {
                vec3 o = right * cx[k] + up * cy[k];
                leaf_maps_[m].corners[k][0] = o.x;
                leaf_maps_[m].corners[k][1] = o.y;
                leaf_maps_[m].corners[k][2] = o.z;
            }
        }
    }
}

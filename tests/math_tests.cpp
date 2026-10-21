#include "check.h"

#include <metin2/math/math.h>

#include <cmath>
#include <random>

using namespace metin2;

namespace {
    auto near(float a, float b) -> bool {
        return std::fabs(a - b) <= 1e-5f * std::max(1.0f, std::max(std::fabs(a), std::fabs(b)));
    }

    auto near(const math::vector3 &a, const math::vector3 &b) -> bool {
        return near(a.x, b.x) && near(a.y, b.y) && near(a.z, b.z);
    }

    auto near(const math::matrix &a, const math::matrix &b) -> bool {
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                if (!near(a.m[i][j], b.m[i][j]))
                    return false;
        return true;
    }
}

TEST(math_types_keep_the_direct3d_layout) {
    CHECK_EQ(sizeof(math::vector2), size_t{8});
    CHECK_EQ(sizeof(math::vector3), size_t{12});
    CHECK_EQ(sizeof(math::vector4), size_t{16});
    CHECK_EQ(sizeof(math::matrix), size_t{64});
    CHECK_EQ(sizeof(math::quaternion), size_t{16});
    CHECK_EQ(sizeof(math::plane), size_t{16});
    CHECK_EQ(sizeof(math::color), size_t{16});
    math::matrix m;
    m._43 = 5.0f;
    CHECK_EQ(m.m[3][2], 5.0f);
    CHECK_EQ(math::quaternion().w, 1.0f);
}

TEST(math_rotations_follow_row_vectors) {
    // A quarter turn about z takes x to y.
    const math::vector3 v = math::transform_coord({1.0f, 0.0f, 0.0f}, math::matrix_rotation_z(math::PI / 2));
    CHECK(near(v, {0.0f, 1.0f, 0.0f}));
    // Translation sits in the fourth row.
    CHECK(near(math::transform_coord({1.0f, 2.0f, 3.0f}, math::matrix_translation(10.0f, 20.0f, 30.0f)), {11.0f, 22.0f, 33.0f}));
    // Rotating by a then b equals multiplying a * b, for matrices and quaternions alike.
    const math::quaternion a = math::quaternion_rotation_axis({0.0f, 0.0f, 1.0f}, 0.3f);
    const math::quaternion b = math::quaternion_rotation_axis({1.0f, 0.0f, 0.0f}, 1.1f);
    CHECK(near(math::matrix_rotation_quaternion(a * b), math::matrix_rotation_quaternion(a) * math::matrix_rotation_quaternion(b)));
}

TEST(math_inverse_undoes_a_matrix) {
    const math::matrix m = math::matrix_rotation_yaw_pitch_roll(0.4f, -0.7f, 1.3f) * math::matrix_translation(3.0f, -2.0f, 8.0f) * math::matrix_scaling(2.0f, 3.0f, 0.5f);
    math::matrix inv;
    CHECK(math::inverse(m, inv));
    CHECK(near(m * inv, math::matrix_identity()));
    math::matrix untouched = math::matrix_identity();
    CHECK(!math::inverse(math::matrix(), untouched));
    CHECK(untouched == math::matrix_identity());
}

TEST(math_colors_round_trip_argb) {
    const math::color c(0x80ff4020u);
    CHECK_EQ(static_cast<uint32_t>(c), 0x80ff4020u);
    CHECK_EQ(static_cast<uint32_t>(math::color(2.0f, -1.0f, 0.5f, 1.0f)), 0xffff0080u);
}

#if __has_include(<d3dx8math.h>)
#include <d3dx8math.h>

// The functions replace the D3DX ones the game used; they must agree.
TEST(math_matches_d3dx) {
    std::mt19937 rng(11);
    std::uniform_real_distribution<float> value(-10.0f, 10.0f);
    auto vec = [&] { return math::vector3(value(rng), value(rng), value(rng)); };
    auto dx = [](const math::vector3 &v) { return D3DXVECTOR3(v.x, v.y, v.z); };
    auto mine = [](const D3DXVECTOR3 &v) { return math::vector3(v.x, v.y, v.z); };
    auto mat = [](const D3DXMATRIX &m) { return math::matrix(&m._11); };

    for (int round = 0; round < 200; ++round) {
        const math::vector3 a = vec();
        const math::vector3 b = vec();
        const D3DXVECTOR3 da = dx(a);
        const D3DXVECTOR3 db = dx(b);
        const float angle = value(rng);

        D3DXVECTOR3 r;
        CHECK(near(math::cross(a, b), mine(*D3DXVec3Cross(&r, &da, &db))));
        CHECK(near(math::normalize(a), mine(*D3DXVec3Normalize(&r, &da))));
        CHECK(near(math::dot(a, b), D3DXVec3Dot(&da, &db)));
        CHECK(near(math::length(a), D3DXVec3Length(&da)));

        D3DXMATRIX dm;
        D3DXMatrixRotationYawPitchRoll(&dm, angle, value(rng), value(rng));
        D3DXMATRIX translation;
        D3DXMatrixTranslation(&translation, a.x, a.y, a.z);
        dm *= translation;
        const math::matrix m = mat(dm);
        CHECK(near(math::transform_coord(b, m), mine(*D3DXVec3TransformCoord(&r, &db, &dm))));
        CHECK(near(math::transform_normal(b, m), mine(*D3DXVec3TransformNormal(&r, &db, &dm))));

        D3DXMATRIX dinv;
        math::matrix inv;
        CHECK(math::inverse(m, inv) == (D3DXMatrixInverse(&dinv, nullptr, &dm) != nullptr));
        CHECK(near(inv, mat(dinv)));
        CHECK(near(math::determinant(m), D3DXMatrixfDeterminant(&dm)));
        D3DXMATRIX dt;
        CHECK(near(math::transpose(m), mat(*D3DXMatrixTranspose(&dt, &dm))));

        D3DXMATRIX d2;
        CHECK(near(math::matrix_rotation_axis(a, angle), mat(*D3DXMatrixRotationAxis(&d2, &da, angle))));
        CHECK(near(math::matrix_rotation_x(angle), mat(*D3DXMatrixRotationX(&d2, angle))));
        CHECK(near(math::matrix_rotation_z(angle), mat(*D3DXMatrixRotationZ(&d2, angle))));
        const D3DXVECTOR3 up(0.0f, 0.0f, 1.0f);
        CHECK(near(math::matrix_look_at_rh(a, b, {0.0f, 0.0f, 1.0f}), mat(*D3DXMatrixLookAtRH(&d2, &da, &db, &up))));
        CHECK(near(math::matrix_perspective_fov_rh(0.8f, 1.3f, 10.0f, 5000.0f), mat(*D3DXMatrixPerspectiveFovRH(&d2, 0.8f, 1.3f, 10.0f, 5000.0f))));
        CHECK(near(math::matrix_ortho_off_center_rh(-3.0f, 5.0f, -2.0f, 7.0f, 1.0f, 9.0f), mat(*D3DXMatrixOrthoOffCenterRH(&d2, -3.0f, 5.0f, -2.0f, 7.0f, 1.0f, 9.0f))));

        D3DXQUATERNION dq1;
        D3DXQUATERNION dq2;
        D3DXQuaternionRotationAxis(&dq1, &da, angle);
        D3DXQuaternionRotationYawPitchRoll(&dq2, b.x, b.y, b.z);
        const math::quaternion q1 = math::quaternion_rotation_axis(a, angle);
        const math::quaternion q2 = math::quaternion_rotation_yaw_pitch_roll(b.x, b.y, b.z);
        D3DXQUATERNION dq;
        D3DXQuaternionMultiply(&dq, &dq1, &dq2);
        const math::quaternion q = q1 * q2;
        CHECK(near(q.x, dq.x) && near(q.y, dq.y) && near(q.z, dq.z) && near(q.w, dq.w));
        CHECK(near(math::matrix_rotation_quaternion(q), mat(*D3DXMatrixRotationQuaternion(&d2, &dq))));

        D3DXPLANE dp(a.x, a.y, a.z, angle);
        D3DXPLANE dpn;
        D3DXPlaneNormalize(&dpn, &dp);
        const math::plane pn = math::plane_normalize({a.x, a.y, a.z, angle});
        CHECK(near(pn.a, dpn.a) && near(pn.d, dpn.d));
        CHECK(near(math::plane_dot_coord(pn, b), D3DXPlaneDotCoord(&dpn, &db)));
    }
}
#endif

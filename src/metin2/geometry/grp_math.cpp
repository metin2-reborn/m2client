#include "grp_math.h"

namespace metin2::inline geometry {

    auto cross_product2d(float x1, float y1, float x2, float y2) -> float {
        return x1 * y2 - y1 * x2;
    }

    auto is_in_triangle2d(float ax, float ay, float bx, float by, float cx, float cy, float tx, float ty) -> bool {
        float c1 = cross_product2d(bx - ax, by - ay, tx - ax, ty - ay);
        float c2 = cross_product2d(cx - bx, cy - by, tx - bx, ty - by);
        float c3 = cross_product2d(ax - cx, ay - cy, tx - cx, ty - cy);

        if (c1 * c2 > 0.0f && c1 * c3 > 0.0f)
            return true;

        if (c1 * c2 * c3 == 0.0f) {
            if (tx < ax)
                if (tx < bx)
                    if (tx < cx)
                        return false;

            if (tx > ax)
                if (tx > bx)
                    if (tx > cx)
                        return false;

            if (ty < ay)
                if (ty < by)
                    if (ty < cy)
                        return false;

            if (ty > ay)
                if (ty > by)
                    if (ty > cy)
                        return false;

            return true;
        }

        return false;
    }
}

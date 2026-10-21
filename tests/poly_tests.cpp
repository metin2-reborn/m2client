#include "check.h"

#include <metin2/poly/poly.h>

#include <cmath>
#include <string>

using namespace metin2;

namespace {
    auto evaluate(const std::string &text, double k = 0) -> double {
        expression poly;
        poly.set_str(text);
        if (!poly.analyze())
            return NAN;
        poly.set_var("k", k);
        return poly.eval();
    }

    auto near(double a, double b) -> bool {
        return std::fabs(a - b) <= 1e-5 * std::fmax(1.0, std::fabs(b));
    }
}

TEST(poly_parses_numbers) {
    CHECK(near(evaluate("12"), 12));
    CHECK(near(evaluate("12.50 + 0.25"), 12.75));
}

TEST(poly_evaluates_operators_and_functions) {
    CHECK(near(evaluate("-2 + 3 * 4 ^ 2"), 46));
    CHECK(near(evaluate("(1 + 2) / 4"), 0.75));
    CHECK(near(evaluate("min(3, 7) + max(3, 7)"), 10));
    CHECK(near(evaluate("abs(-2) + floor(2.7)"), 4));
    CHECK(near(evaluate("40 + 2 * k", 5), 50));
}

TEST(poly_rejects_syntax_errors) {
    for (const char *text : {"(1 + 2", "1 2", "min(1)", "1 + * 2", ")"}) {
        expression poly;
        poly.set_str(text);
        CHECK(!poly.analyze());
    }
}

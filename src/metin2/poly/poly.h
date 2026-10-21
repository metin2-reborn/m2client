#pragma once

#include "sym_table.h"

#include <string>
#include <vector>

namespace metin2::inline poly {

#define POLY_MAXVALUE 0
#define POLY_NONE     POLY_MAXVALUE
#define POLY_ROOT     POLY_MAXVALUE + 1
#define POLY_MUL      POLY_MAXVALUE + 2
#define POLY_PLU      POLY_MAXVALUE + 3
#define POLY_POW      POLY_MAXVALUE + 4
#define POLY_MIN      POLY_MAXVALUE + 5
#define POLY_DIV      POLY_MAXVALUE + 6
#define POLY_OPEN     POLY_MAXVALUE + 7
#define POLY_CLOSE    POLY_MAXVALUE + 8
#define POLY_NUM      POLY_MAXVALUE + 9
#define POLY_ID       POLY_MAXVALUE + 10
#define POLY_EOS      POLY_MAXVALUE + 11
#define POLY_COS      POLY_MAXVALUE + 12
#define POLY_SIN      POLY_MAXVALUE + 13
#define POLY_TAN      POLY_MAXVALUE + 14
#define POLY_COSEC    POLY_MAXVALUE + 15
#define POLY_CSC      POLY_COSEC
#define POLY_SEC      POLY_MAXVALUE + 16
#define POLY_COT      POLY_MAXVALUE + 17
#define POLY_PI       POLY_ID
#define POLY_EXP      POLY_ID
#define POLY_LOG      POLY_MAXVALUE + 18
#define POLY_LN       POLY_MAXVALUE + 19
#define POLY_LOG10    POLY_MAXVALUE + 20

#define POLY_ABS   POLY_MAXVALUE + 21
#define POLY_MINF  POLY_MAXVALUE + 22
#define POLY_MAXF  POLY_MAXVALUE + 23
#define POLY_IRAND POLY_MAXVALUE + 24
#define POLY_FRAND POLY_MAXVALUE + 25
#define POLY_MOD   POLY_MAXVALUE + 26
#define POLY_FLOOR POLY_MAXVALUE + 27

#define POLY_MAXSTACK 100

    class expression {
      public:
        enum random_type {
            RANDOM_TYPE_FREELY,
            RANDOM_TYPE_FORCE_MIN,
            RANDOM_TYPE_FORCE_MAX,
        };

      public:
        expression();
        virtual ~expression();

        auto analyze(const char *str = nullptr) -> int;
        auto eval() -> float;
        void set_random(int random_type_value);
        void set_str(const std::string &str);
        auto set_var(const std::string &name, double var) -> int;
        auto var_count() -> int;
        auto var_name(unsigned int index) -> const char *;
        void clear();

      protected:
        auto my_irandom(double start, double end) -> int;
        auto my_frandom(double start, double end) -> double;

        void init();
        auto insert(const std::string &s, int tok) -> int;
        auto find(const std::string &s) -> int;
        void emit(int t, int tval);
        void match(int t);
        void expo();
        void factor();
        void term();
        int token_;
        double num_token_;
        int look_ahead_;
        auto lexan() -> int;
        int error_pos_;
        void error();
        void expr();
        bool error_occur_;
        unsigned int look_pos_;

        std::vector<int> token_base_;
        std::vector<double> num_base_;
        std::vector<sym_table *> symbol_;
        std::vector<int> symbol_index_;
        int st_size_;
        int math_symbol_count_;
        std::string data_value_;
        int random_type_value_;
    };
}

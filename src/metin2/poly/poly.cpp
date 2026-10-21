#include "poly.h"
#include <metin2/core/random.h>

#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <string>

namespace metin2::inline poly {

    namespace {
        // uniform on [0, 1)
        auto unit_random() -> double {
            return metin2::random::fnumber(0.0, 1.0);
        }
    } // namespace

    void expression::set_random(int random_type_value) {
        random_type_value_ = random_type_value;
    }

    auto expression::my_irandom(double start, double end) -> int {
        switch (random_type_value_) {
        case RANDOM_TYPE_FORCE_MIN:
            return int(start);
            break;
        case RANDOM_TYPE_FORCE_MAX:
            return int(end);
            break;
        }

        // Make range as inclusive-exclusive
        int is = int(start + 0.5);
        int ie = int(end - start + 0.5) + 1;

        return int(unit_random() * ie + is);
    }

    auto expression::my_frandom(double start, double end) -> double {
        return unit_random() * (end - start) + start;
    }

    expression::expression() {
        random_type_value_ = RANDOM_TYPE_FREELY;
        look_pos_ = 0;
        error_occur_ = true;
        symbol_.clear();
        st_size_ = 0;
        math_symbol_count_ = 0;
        symbol_.reserve(50);
        init();
    }

    expression::~expression() {
        clear();
    }

    void expression::set_str(const std::string &str) {
        data_value_ = str;
    }

    auto expression::eval() -> float {
        int now;
        double save[POLY_MAXSTACK], t;
        int sp = 0;
        if (error_occur_) {
            return 0;
        }

        auto pos = token_base_.begin();
        auto posn = num_base_.begin();
        while (pos != token_base_.end()) {

            now = *pos;
            ++pos;
            switch (now) {
            case POLY_NUM:
                save[sp++] = *posn++;
                break;
            case POLY_ID:
                save[sp++] =
                    symbol_[*pos]->val;
                pos++;
                break;
            case POLY_PLU:
                sp--;
                save[sp - 1] += save[sp];
                break;
            case POLY_MIN:
                sp--;
                save[sp - 1] -= save[sp];
                break;
            case POLY_MUL:
                sp--;
                save[sp - 1] *= save[sp];
                break;
            case POLY_MOD:
                sp--;
                if (save[sp] == 0) {
                    return 0;
                }
                save[sp - 1] = fmod(save[sp - 1], save[sp]);
                break;
            case POLY_DIV:
                sp--;
                if (save[sp] == 0) {
                    return 0;
                }
                save[sp - 1] /= save[sp];
                break;
            case POLY_POW:
                sp--;
                save[sp - 1] = pow(save[sp - 1], save[sp]);
                break;
            case POLY_ROOT:
                if (save[sp - 1] < 0) {
                    return 0;
                }
                save[sp - 1] = sqrt(save[sp - 1]);
                break;
            case POLY_COS:
                save[sp - 1] = cos(save[sp - 1]);
                break;
            case POLY_SIN:
                save[sp - 1] = sin(save[sp - 1]);
                break;
            case POLY_TAN:
                if (!(t = cos(save[sp - 1]))) {
                    return 0;
                }
                save[sp - 1] = tan(save[sp - 1]);
                break;
            case POLY_CSC:
                if (!(t = sin(save[sp - 1]))) {
                    return 0;
                }
                save[sp - 1] = 1 / t;
                break;
            case POLY_SEC:
                if (!(t = cos(save[sp - 1]))) {
                    return 0;
                }
                save[sp - 1] = 1 / t;
                break;
            case POLY_COT:
                if (!(t = sin(save[sp - 1]))) {
                    return 0;
                }
                save[sp - 1] = cos(save[sp - 1]) / t;
                break;
            case POLY_LN:
                if (save[sp - 1] <= 0) {
                    return 0;
                }
                save[sp - 1] = std::log(save[sp - 1]);
                break;
            case POLY_LOG10:
                if (save[sp - 1] <= 0) {
                    return 0;
                }
                save[sp - 1] = log10(save[sp - 1]);
                break;
            case POLY_LOG:
                if (save[sp - 1] <= 0) {
                    return 0;
                }
                if (save[sp - 2] <= 0 || save[sp - 2] == 1) {
                    return 0;
                }

                save[sp - 2] = std::log(save[sp - 1]) / std::log(save[sp - 2]);
                sp--;
                break;
            case POLY_ABS:
                save[sp - 1] = fabs(save[sp - 1]);
                break;
            case POLY_FLOOR:
                save[sp - 1] = floor(save[sp - 1]);
                break;
            case POLY_IRAND:
                save[sp - 2] = my_irandom(save[sp - 2], save[sp - 1]);
                sp--;
                break;
            case POLY_FRAND:
                save[sp - 2] = my_frandom(save[sp - 2], save[sp - 1]);
                sp--;
                break;
            case POLY_MINF:
                save[sp - 2] = (save[sp - 2] < save[sp - 1]) ? save[sp - 2] : save[sp - 1];
                sp--;
                break;
            case POLY_MAXF:
                save[sp - 2] = (save[sp - 2] > save[sp - 1]) ? save[sp - 2] : save[sp - 1];
                sp--;
                break;
            default:
                return 0;
            }
        }
        return float(save[sp - 1]);
    }

    auto expression::analyze(const char *str) -> int {
        if (str)
            set_str(str);

        if (0 == data_value_.length())
            return true;

        error_occur_ = false;
        look_pos_ = 0;
        look_ahead_ = lexan();

        expr();

        if (token_base_.empty()) {
            return false;
        }

        return !error_occur_;
    }

    void expression::clear() {
        int i;
        token_base_.clear();
        num_base_.clear();

        for (i = 0; i < st_size_; ++i) {
            if (symbol_[i]) delete symbol_[i];
            symbol_[i] = nullptr;
        }
        symbol_.clear();
        symbol_index_.clear();
        st_size_ = 0;
        math_symbol_count_ = 0;
    }

    void expression::expr() {
        int t;

        switch (look_ahead_) {
        case '+':
        case '-':
            look_pos_--;
            look_ahead_ = POLY_NUM;
            num_token_ = token_ = 0;
        }

        term();

        while (!error_occur_) {
            switch (look_ahead_) {
            case '+':
            case '-':
                t = look_ahead_;
                match(t);
                term();
                emit(t, POLY_NONE);
                continue;
            case POLY_EOS:
            case ')':
            case ',':      return;
            default:
                error();
                return;
            }
        }
    }

    void expression::error() {
        error_pos_ = look_pos_;
        error_occur_ = true;
    }

    auto expression::lexan() -> int {
        int t;
        double tt;

        while (look_pos_ < data_value_.size()) {
            if (data_value_[look_pos_] == ' ' || data_value_[look_pos_] == '\t')
                ;
            else if (isdigit(data_value_[look_pos_])) {
                t = 0;
                for (; look_pos_ < data_value_.size(); look_pos_++) {
                    if (isdigit(data_value_[look_pos_]))
                        t = t * 10 + data_value_[look_pos_] - '0';
                    else
                        break;
                }
                token_ = t;
                tt = 0.1;
                num_token_ = 0;
                if (look_pos_ < data_value_.size() && data_value_[look_pos_] == '.') {
                    look_pos_++;
                    for (; look_pos_ < data_value_.size(); look_pos_++, tt *= 0.1) {
                        if (isdigit(data_value_[look_pos_]))
                            num_token_ += tt * (data_value_[look_pos_] - '0');
                        else
                            break;
                    }
                }
                num_token_ += token_;
                return POLY_NUM;
            } else if (isalpha(data_value_[look_pos_])) {
                std::string local_symbol("");
                while (look_pos_ < data_value_.size() && isalpha(data_value_[look_pos_])) {
                    local_symbol += data_value_[look_pos_];
                    look_pos_++;
                }
                token_ = find(local_symbol);
                if (token_ == -1) {
                    token_ = insert(local_symbol, POLY_ID);
                }
                return symbol_[((token_))]->token;
            } else {
                token_ = 0;
                return data_value_[look_pos_++];
            }
            look_pos_++;
        }
        return POLY_EOS;
    }

    void expression::term() {
        int t;
        factor();
        while (!error_occur_) {
            switch (look_ahead_) {
            case '*':
            case '/':
            case '%':
                t = look_ahead_;
                match(t);
                factor();
                emit(t, POLY_NONE);
                continue;
            default:
                return;
            }
        }
    }

    void expression::factor() {
        int t;
        expo();
        while (!error_occur_) {
            switch (look_ahead_) {
            case '^':
                t = look_ahead_;
                match(t);
                expo();
                emit(t, POLY_NONE);
                continue;
            default:
                return;
            }
        }
    }

    void expression::expo() {
        int t;
        switch (look_ahead_) {
        case '(':
            match('(');
            expr();
            match(')');
            break;
        case POLY_NUM:
            emit(POLY_NUM, token_);
            match(POLY_NUM);
            break;
        case POLY_ID:
            emit(POLY_ID, (int)(token_));
            match(POLY_ID);
            break;
        case POLY_ROOT:
        case POLY_SIN:
        case POLY_COT:
        case POLY_TAN:
        case POLY_CSC:
        case POLY_SEC:
        case POLY_LN:
        case POLY_LOG10:
        case POLY_COS:
        case POLY_ABS:
        case POLY_FLOOR:
            t = look_ahead_;
            match(look_ahead_);
            match('(');
            expr();
            match(')');
            emit(t, token_);
            break;
        case POLY_LOG:
        case POLY_MINF:
        case POLY_MAXF:
        case POLY_IRAND:
        case POLY_FRAND:
        case POLY_MOD:
            t = look_ahead_;
            match(look_ahead_);
            match('(');
            expr();
            match(',');
            expr();
            match(')');
            emit(t, token_);
            break;
        case POLY_EOS:
            break;
        default:
            error();
        }
    }

    void expression::match(int t) {
        if (look_ahead_ == t)
            look_ahead_ = lexan();
        else
            error();
    }

    void expression::emit(int t, int tval) {
        switch (t) {
        case '+':
            token_base_.push_back(POLY_PLU);
            break;
        case '-':
            token_base_.push_back(POLY_MIN);
            break;
        case '*':
            token_base_.push_back(POLY_MUL);
            break;
        case '/':
            token_base_.push_back(POLY_DIV);
            break;
        case '%':
            token_base_.push_back(POLY_MOD);
            break;
        case '^':
            token_base_.push_back(POLY_POW);
            break;
        case POLY_ROOT:
        case POLY_SIN:
        case POLY_TAN:
        case POLY_COT:
        case POLY_COS:
        case POLY_CSC:
        case POLY_SEC:
        case POLY_LOG:
        case POLY_LN:
        case POLY_LOG10:
        case POLY_ABS:
        case POLY_MINF:
        case POLY_MAXF:
        case POLY_IRAND:
        case POLY_FRAND:
        case POLY_MOD:
        case POLY_FLOOR:
            token_base_.push_back(t);
            break;
        case POLY_NUM:
            token_base_.push_back(t);
            num_base_.push_back(num_token_);
            break;
        case POLY_ID:
            token_base_.push_back(t);
            token_base_.push_back(tval);
            break;
        default:
            error();
            clear();
            return;
        }
    }

    auto expression::find(const std::string &s) -> int {
        int l, m, r;

        l = 0;
        r = st_size_ - 1;

        while (l <= r) {
            m = (l + r) >> 1;

            if (symbol_[symbol_index_[m]]->strlex == s)
                return symbol_index_[m];
            else if (symbol_[symbol_index_[m]]->strlex < s)
                l = m + 1;
            else
                r = m - 1;
        }
        return -1;
    }

    auto expression::insert(const std::string &s, int tok) -> int {
        int i;
        bool added = false;

        symbol_.push_back(new sym_table(tok, s));
        for (i = 0; i < st_size_; i++) {
            if (s < symbol_[symbol_index_[i]]->strlex) {
                symbol_index_.insert(symbol_index_.begin() + i, st_size_);
                added = true;
                break;
            }
        }
        if (!added) {
            symbol_index_.push_back(st_size_);
        }
        st_size_++;
        return st_size_ - 1;
    }

    auto expression::set_var(const std::string &name, double d_var) -> int {

        if (error_occur_) return false;
        int index = find(name);
        if (index == -1) return false;
        sym_table *st_var = symbol_[((index))];
        st_var->val = d_var;
        return true;
    }

    auto expression::var_count() -> int {
        return symbol_.size() - math_symbol_count_;
    }

    auto expression::var_name(unsigned int index) -> const char * {
        assert(index + math_symbol_count_ < symbol_.size());
        return symbol_[index + math_symbol_count_]->strlex.c_str();
    }

    void expression::init() {
        insert("min", POLY_MINF);
        insert("max", POLY_MAXF);
        insert("number", POLY_IRAND);
        insert("irandom", POLY_IRAND);
        insert("irand", POLY_IRAND);
        insert("frandom", POLY_FRAND);
        insert("frand", POLY_FRAND);
        insert("rt", POLY_ROOT);
        insert("sqrt", POLY_ROOT);
        insert("cos", POLY_COS);
        insert("sin", POLY_SIN);
        insert("tan", POLY_TAN);
        insert("cot", POLY_COT);
        insert("csc", POLY_CSC);
        insert("cosec", POLY_COSEC);
        insert("sec", POLY_SEC);
        insert("pi", POLY_PI);
        set_var("pi", 3.141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117068);
        insert("e", POLY_EXP);
        set_var("e", 2.718281828459045235360287471352662497757247093699959574966967627724076630353547594571382178525166427);
        insert("log", POLY_LOG);
        insert("ln", POLY_LN);
        insert("log10", POLY_LOG10);
        insert("abs", POLY_ABS);
        insert("mod", POLY_MOD);
        insert("floor", POLY_FLOOR);
        math_symbol_count_ = st_size_;
    }
}

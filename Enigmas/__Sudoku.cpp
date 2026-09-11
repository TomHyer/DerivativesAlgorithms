
#include "Base/__Platform.h"
#include "KillerSudoku.h"
#include "Base/Strict.h"


extern "C" __declspec(dllexport) int xlAutoOpen(void)
{
    return Excel::AutoOpen("Enigmas");
}

namespace
{
/*IF--------------------------------------------------------------------------
public Killer_Sudoku
    Create a matrix describing a puzzle
&inputs
entropy is number
    A measure of difficulty; 150 is high
seed is integer
    &$ > 0
    Seed for random generation
minimal is boolean
    Show fewer outputs
&outputs
puzzle is cell[][]
    The puzzle
-IF-------------------------------------------------------------------------*/

    void Killer_Sudoku
        (double entropy,
         int seed,
         bool minimal,
         Matrix_<Cell_>* puzzle)
    {
        *puzzle = KS(entropy, seed, minimal);
    }
}  // leave local

#include "MG_Killer_Sudoku_public.inc"




// generic parser
#include <memory>
#include <map>
#include <set>
#include "Base/Optionals.h"

// GOAL: parse string -> AST in a user-specified grammar, with a one-pass state machine
//          WITHOUT metaprogramming or non-native code
//          
// CAVEATS:  semantics are not handled at all.  Bracketing pairs must be a single character.  "Maximal munch" for operator lexing.

namespace
{
    using namespace std;

    struct Token_
    {
        char kind_;  // for single-character tokens, kind_==val_; otherwise non-printable
        String_ val_;

        Token_(char c) : kind_(c), val_(1, c) {}
        Token_(const String_& s, char kind) : kind_(kind), val_(s) {}
    };
    const Token_ NONE_TOKEN = { String_(), 0};
    constexpr char ALPHA_KIND = 16, NUMERIC_KIND = 17;

    inline bool operator==(const Token_& lhs, const Token_& rhs) { return lhs.val_ == rhs.val_; }
    inline bool operator!=(const Token_& lhs, const Token_& rhs) { return lhs.val_ != rhs.val_; }
    inline bool operator<(const Token_ & lhs, const Token_ & rhs) { return lhs.val_ < rhs.val_; }

    struct Bracket_
    {
        char bra_;
        char ket_;
    };

    struct BracketList_
    {
        map<char, std::optional<Token_>> braToKet_;
        map<char, std::optional<Token_>> ketToBra_;

        BracketList_(const Vector_<Bracket_>& pairs)
        {
            for (Bracket_ b : pairs)
            {
                braToKet_[b.bra_] = Token_(b.ket_);
                ketToBra_[b.ket_] = Token_(b.bra_);
            }
        }

        bool IsBracket(char c) const { return braToKet_.count(c) || ketToBra_.count(c); }
    };

    struct OperatorList_
    {
        enum class Assoc_ { LEFT, RIGHT, UNARY };  // kinds of associativity
        Vector_<pair<Assoc_, Vector_<Token_>>> vals_;

        map<String_, char> multi_;
        set<Token_> unary_, binary_;

        Token_ Register(char c, bool binary)
        {
            set<Token_>& existing = binary ? binary_ : unary_;
            Token_ retval(c);
            REQUIRE(!existing.count(retval), "Multiple declarations of " + String_(1, c));
            existing.emplace(retval);
            return retval;
        }
        Token_ Register(const String_& s, bool binary)
        {
            if (s.size() == 1)
                return Register(s[0], binary);
            if (!multi_.count(s))
                multi_.emplace(s, -static_cast<char>(1 + multi_.size()));
            Token_ retval(s, multi_[s]);
            set<Token_>& existing = binary ? binary_ : unary_;
            REQUIRE(!existing.count(retval), "Multiple declarations of " + s);
            existing.emplace(retval);
            return retval;
        }

        struct Append_
        {
            OperatorList_& parent_;
            template<class T_> Append_& operator()(T_ t) { parent_.vals_.back().second.push_back(parent_.Register(t, parent_.vals_.back().first != Assoc_::UNARY)); return *this; }
            Append_& Left() { parent_.vals_.emplace_back(Assoc_::LEFT, Vector_<Token_>()); return *this; }
            Append_& Right() { parent_.vals_.emplace_back(Assoc_::RIGHT, Vector_<Token_>()); return *this; }
            Append_& Unary() { parent_.vals_.emplace_back(Assoc_::UNARY, Vector_<Token_>()); return *this; }
        };

        Append_ Left() { Append_ retval{ *this }; return retval.Left(); }
        Append_ Right() { Append_ retval{ *this };  return retval.Right(); }
        Append_ Unary() { Append_ retval{ *this };  return retval.Unary(); }

        bool ExistsUnary(Token_ t) const { return !!unary_.count(t); }
        bool ExistsBinary(Token_ t) const { return !!binary_.count(t); }
    };

    struct Precedence_
    {
        map<char, int> u1_, l1_, r1_;
        map<String_, int> u2_, l2_, r2_;

        Precedence_(const OperatorList_& ops)
        {
            int score = 0;
            for (const auto& [a, v] : ops.vals_)
            {
                for (const Token_& t : v)
                {
                    if (a == OperatorList_::Assoc_::UNARY)
                        u1_[t.kind_] = u2_[t.val_] = score;
                    else
                    {
                        l1_[t.kind_] = l2_[t.val_] = score + (a == OperatorList_::Assoc_::LEFT ? -1 : 1);
                        r1_[t.kind_] = r2_[t.val_] = score - (a == OperatorList_::Assoc_::LEFT ? -1 : 1);
                    }
                }
                score += 10;
            }
        }

        bool SplitLeft(const Token_& left, const Token_& right) const
        {
            if (auto pl = l1_.find(left.kind_), pr = r1_.find(right.kind_); pl != l1_.end() && pr != r1_.end())
                return pl->second > pr->second;
            REQUIRE(false, "Precedence only supports binary-binary comparison");
        }
    };

    bool IsAlphanumeric(char c)
    {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
    }
    bool IsNumeric(char c)
    {
        return c >= '0' && c <= '9';
    }
    bool IsWhite(char c)
    {
        return c == ' ' || c == '\t' || c == '\n';
    }

    struct LexerBase_
    {
        virtual ~LexerBase_() {}
    };
    struct OpLexer_ : LexerBase_
    {
        struct RHS_
        {
            Token_ halt_ = NONE_TOKEN;
            shared_ptr<LexerBase_> continue_;
        };
        map<char, RHS_> vals_;

        void Emplace(const Token_& t, int offset = 0)
        {
            RHS_& child = vals_[t.val_[offset]];
            if (t.val_.size() == offset + 1)
                child.halt_ = t;
            else
            {
                if (!child.continue_)
                    child.continue_ = std::make_shared<OpLexer_>();
                static_cast<OpLexer_*>(child.continue_.get())->Emplace(t, offset + 1);
            }
        }

        OpLexer_() {}
        OpLexer_(const OperatorList_& ops)
        {
            for (const auto& [a, v] : ops.vals_)
                for (const auto& t : v)
                    Emplace(t);
        }

        Token_ operator()(const BracketList_& brackets, const char*& p, const char* end) const
        {   // only call for non-alphanumeric
            const auto* layer = &vals_;
            const Token_* candidate = nullptr;
            const char* candidateEnd = nullptr;
            for ( ; ; )
            {
                auto pr = layer->find(*p);
                if (pr == layer->end())
                {
                    REQUIRE(candidate, "No such operator from " + String_(1, *p));
                    p = candidateEnd;
                    return *candidate;
                }
                ++p;
                if (pr->second.halt_ != NONE_TOKEN)
                {
                    candidate = &pr->second.halt_;
                    candidateEnd = p;
                }
                if (p == end || IsAlphanumeric(*p) || IsWhite(*p) || brackets.IsBracket(*p) || !pr->second.continue_.get())
                {   // token is over
                    REQUIRE(candidate, "No such operator at " + String_(1, *p));
                    p = candidateEnd;
                    return *candidate;
                }
                layer = &static_cast<const OpLexer_*>(pr->second.continue_.get())->vals_;
                // ... and go around again
            }
        }
    };


    struct Lexer_
    {
        OperatorList_ ops_;
        BracketList_ brackets_;
        OpLexer_ opLex_;
        const char* loc_;
        const char* end_;

        Lexer_(const OperatorList_& ops, const BracketList_& brackets)
            : ops_(ops)
            , brackets_(brackets)
            , opLex_(ops)
        {
            // learn to recognize all the tokens
            for (const auto& [_, tt] : ops.vals_)
                for (const Token_& t : tt)
                    opLex_.Emplace(t);
        }

        void Restart(const char* begin, const char* end)
        {
            loc_ = begin;
            end_ = end;
        }

        Token_ Number()
        {
            const char* start = loc_;
            auto ffwd = [&]() { ++loc_;  while (loc_ != end_ && IsNumeric(*loc_)) ++loc_; };
            // consume digits
            ffwd();
            // allowed one decimal point
            if (loc_ != end_ && *loc_ == '.')
                ffwd();
            // allowed one exponent, which could have a sign attached
            if (loc_ != end_ && (*loc_ == 'e' || *loc_ == 'E'))
            {
                if (loc_ + 1 != end_ && (*(loc_ + 1) == '+' || *(loc_ + 1) == '-'))
                    ++loc_;
                ffwd();
            }
            return Token_{ String_(start, loc_), NUMERIC_KIND };
        }

        Token_ Next()
        {
            while (loc_ != end_ && IsWhite(*loc_))
                ++loc_;
            if (loc_ == end_ || !*loc_)
                return NONE_TOKEN;
            if (IsAlphanumeric(*loc_))
            {
                if (IsNumeric(*loc_))
                    return Number();
                // some kind of name -- read alphanumerics until we run out
                const char* start = loc_;
                while (loc_ != end_ && IsAlphanumeric(*loc_))
                    ++loc_;
                return Token_(String_(start, loc_), ALPHA_KIND);
            }
            if (brackets_.IsBracket(*loc_))
                return Token_(*loc_++);

            // must be an operator
            return opLex_(brackets_, loc_, end_);
        }
    };

    struct AST_
    {
        enum class State_
        {
            EMPTY,
            ATOM,
            UNARY,
            BINARY,
            FUNCTION
        } state_;
        char bra_;  // 0 if not parenthesized
        Token_ head_;
        AST_* parent_;
        Vector_<shared_ptr<AST_>> children_;

        AST_() : head_(NONE_TOKEN), parent_(nullptr), bra_(0), state_(State_::EMPTY) {}
    };

    struct Parser_
    {
        using State_ = AST_::State_;
        AST_ top_;
        AST_* current_;

        OperatorList_ ops_;
        Precedence_ precedence_;
        OpLexer_ opLexer_;
        BracketList_ brackets_;

        Parser_(const OperatorList_& ops, const BracketList_& brackets) : ops_(ops), precedence_(ops), opLexer_(ops), brackets_(brackets) {}

        AST_* Append(Token_ next)
        { 
            if (current_->state_ == State_::EMPTY && next != NONE_TOKEN)
            {
                current_->head_ = next;
                return current_;
            }
            current_->children_.emplace_back(new AST_);
            AST_* retval = current_->children_.back().get();
            retval->parent_ = current_;
            retval->head_ = next;
            return retval;
        }

        void Insert(Token_ next)
        {
            // close-brace
            if (brackets_.ketToBra_.count(next.kind_))
            {
                char bra = brackets_.ketToBra_[next.kind_].value().kind_;
                while (current_ && (!current_->bra_ || brackets_.ketToBra_.count(current_->bra_))) // walk past close-braces, look for open-brace
                {
                    if (current_->state_ == State_::UNARY)
                    {
                        REQUIRE(!current_->children_.empty(), "unary without argument, like '( + )'");
                    }
                    else if (current_->state_ == State_::BINARY)
                    {
                        REQUIRE(current_->children_.size() == 2, "binary without second argument, like 'A + )'");
                    }
                    current_->state_ = State_::ATOM;
                    current_ = current_->parent_;
                }
                REQUIRE(current_, "No open-brace found for " + next.val_);
                REQUIRE(current_->bra_ == bra, "Wrong kind of open-brace found: " + String_(1, current_->bra_));
                current_->bra_ = next.kind_;  // mark as closed
                current_->state_ = State_::ATOM;
                return;
            }
            // open-brace
            if (brackets_.braToKet_.count(next.kind_))
            {
                if (current_->state_ == State_::ATOM || current_->state_ == State_::FUNCTION)
                    current_->state_ = State_::FUNCTION;  // this allows a numeric atom to be a function name
                if (current_->state_ != State_::EMPTY || current_->bra_)
                    current_ = Append(NONE_TOKEN);
                current_->state_ = State_::EMPTY;
                current_->bra_ = next.kind_;
                return;
            }
            // name or value
            if (IsAlphanumeric(next.val_[0]))
            {
                REQUIRE(current_->state_ != State_::ATOM, "consecutive atoms, like 'A B'");
                REQUIRE(current_->state_ != State_::FUNCTION, "function then atom, like 'A(3) B'");
                current_ = Append(next);
                current_->state_ = State_::ATOM;
                return;
            }
            // some kind of operator
            if (current_->state_ == State_::UNARY || current_->state_ == State_::BINARY)  // stacked operators, like "3 + - - + +" which is still OK awaiting closure
            {
                current_ = Append(next);
                REQUIRE(ops_.ExistsUnary(next), "can't stack binary operators (postfix not yet supported)");
                current_->state_ = State_::UNARY;
                return;
            }
            // operator applied to a non-operator, at last
            REQUIRE(ops_.ExistsBinary(next), "postfix not yet supported");
            for ( ; ; )
            {
                AST_* lhsOp = current_;
                if (lhsOp && lhsOp->bra_ && lhsOp->state_ == State_::ATOM)
                {   // see if it's the arguments of a function call; remember call can look like f(x)(y)(z)
                    for (AST_* args = lhsOp; args && args->state_ == State_::ATOM && brackets_.ketToBra_.count(args->bra_); args = args->parent_)
                    {
                        if (args->parent_ && args->parent_->state_ == State_::FUNCTION)
                        {
                            lhsOp = args->parent_;
                            break;
                        }
                    }
                }
                while (lhsOp && lhsOp->state_ != State_::UNARY && lhsOp->state_ != State_::BINARY && !brackets_.IsBracket(lhsOp->bra_))
                    lhsOp = lhsOp->parent_;
                if (!lhsOp || (lhsOp->bra_ && (lhsOp->state_ == State_::ATOM || brackets_.ketToBra_.count(lhsOp->bra_))))
                    break;
                if (precedence_.SplitLeft(lhsOp->head_, next))
                    break;  // lhs operator has lower precedence, like "2 + 3 *"
                // next operator has lower precedence, like "2 * 3 +"; roll up the lhs 
                while (current_ != lhsOp)
                {
                    current_ = current_->parent_;
                }
                current_->state_ = State_::ATOM;    // no longer splittable
            }
            // change the existing AST to add this operator
            if (current_->state_ == State_::EMPTY)
            {
                current_->state_ = State_::UNARY;
                current_->head_ = next;
            }
            else
            {
                while (current_->parent_ && current_->state_ == State_::ATOM && brackets_.ketToBra_.count(current_->bra_))
                    current_ = current_->parent_;
                AST_* parent = current_->parent_;
                shared_ptr<AST_> temp(new AST_(*current_));
                temp->parent_ = current_;
                // and take this node for ourselves
                current_->state_ = State_::BINARY;
                temp->bra_ = 0;  // grouping stays up there
                current_->head_ = next;
                current_->parent_ = parent;
                current_->children_ = { temp };
            }
        }

        AST_ operator()(const char* begin, const char* end)
        {
            Lexer_ lex(ops_, brackets_);
            lex.Restart(begin, end);
            top_ = AST_();
            current_ = &top_;
            for ( ; ; )
            {
                Token_ t = lex.Next();
                if (t == NONE_TOKEN)
                    break;
                Insert(t);
            }
            // if the whole expression was parenthesized, cast those off
            while (top_.state_ == State_::EMPTY && top_.children_.size() == 1 && top_.children_[0]->bra_)
            {
                top_.state_ = top_.children_[0]->state_;
                top_.head_ = top_.children_[0]->head_;
                auto newChildren = top_.children_[0]->children_;
                top_.children_.clear();
                top_.children_ = newChildren;
            }
            return top_;
        }
    };

    int example()
    {
        // example usage:
        OperatorList_ ops;
        ops.Left()("::").Left()('.')("->").Unary()("++")("--")('+')('-')('*')('&').Left()(".*")("->*")
            .Left()('*')('/')('%').Left()('+')('-').Left()("<<")(">>").Left()("<=>")
            .Left()('<')("<=")('>')(">=").Left()("==")("!=").Left()('&').Left()('^').Left()('|')
            .Left()("&&").Left()("||").Right()('?')(':')
            .Right()('=')("+=")("-=")("*=")("/=")("%=")("<<=")(">>=")("&=")("^=")("?=")
            .Left()(',');

    }

    String_ S_Expression(const AST_& ast, const BracketList_& brackets)
    {
        if (ast.children_.empty())
            return ast.head_.val_;
        String_ retval = "(" + ast.head_.val_;
        if (char ket = ast.children_.empty() ? 0 : ast.children_[0]->bra_)
        {
            if (auto p = brackets.ketToBra_.find(ket); p != brackets.ketToBra_.end())
                retval += p->second->kind_;
            retval += ket;
        }
        retval += " ";
        for (auto c : ast.children_)
            retval += S_Expression(*c, brackets) + " ";
        retval.back() = ')';
        return retval;
    }

/*IF--------------------------------------------------------------------------
public Parse_SExpression
    Convert infix to S-expression
&inputs
formula is string
    Using infix notation
&optional
repeat is integer (1)
    Number of times to parse before emitting
&outputs
s_expression is string
    The same formula, parsed to AST and output as S-expression
-IF-------------------------------------------------------------------------*/

    void Parse_SExpression
        (const String_& formula,
         int repeat,
         String_* s_expression)
    {
        // example operators:
        OperatorList_ ops;
        ops.Left()("::").Left()('.')("->").Unary()("++")("--")('+')('-')('*')('&')('!').Left()(".*")("->*")
                .Left()('*')('/')('%').Left()('+')('-').Left()("<<")(">>").Left()("<=>")
                .Left()('<')("<=")('>')(">=").Left()("==")("!=").Left()('&').Left()('^').Left()('|')
                .Left()("&&").Left()("||").Right()('?')(':')
                .Right()('=')("+=")("-=")("*=")("/=")("%=")("<<=")(">>=")("&=")("^=")("?=")
                .Left()(',');

        BracketList_ brackets(Vector_<Bracket_>({{ '(', ')' }, {'{', '}'}, {'[', ']'} }));

        Parser_ parse(ops, brackets);
        const char* start = &formula[0];
        AST_ ast = parse(start, start + formula.size());
        while (--repeat > 0)
            ast = parse(start, start + formula.size());
        *s_expression = S_Expression(ast, brackets);
    }
}   // leave local

#include "MG_Parse_SExpression_public.inc"

#ifdef ZZZ

// implement various arithmetic classes
struct Defines_
{
    std::map<string, double> vals_;
    const VolCurve_& v_;

    Defines_(const VolCurve_& v) : v_(v)
    {
        vals_.emplace("f", v_.RefFwd());
        vals_.emplace("t", v_.TTE());
    }
};

struct Value_
{
    virtual ~Value_() {}
    virtual double evaluate(const Defines_& known) const = 0;
};

struct Number_ : Value_
{
    double v_;
    double evaluate(const Defines_&) const override
    {
        return v_;
    }
};

template<typename F> struct Binary_ : Value_
{
    F func_;
    Handle_<Value_> lhs_, rhs_;

    Binary_(F func, const Handle_<Value_>& lhs, const Handle_<Value_>& rhs) : func_(func), lhs_(lhs), rhs_(rhs) {}

    double evaluate(const Defines_& known) const override
    {
        return op_(lhs_->evaluate(known), rhs_->evaluate(known));
    }
};
template<typename F> Handle_<Value_> MakeBinary(F func, const Handle_<Value_>& lhs, const Handle_<Value_>& rhs)
{
    return new Binary_<F>(func, lhs, rhs);
}


template<typename F> struct Unary_ : Value_
{
    F func_;
    Handle_<Value_> arg_;

    Unary_(F func, const Handle_<Value_>& arg) : func_(func), arg_(arg) {}

    double evaluate(const Defines_& known) const override
    {
        return func_(arg_->evaluate(known));
    }
};
struct MakeUnaryBase_
{
    virtual ~MakeUnaryBase_() {}
    virtual Handle_<Value_> operator()(const Handle_<Value_>& arg) const = 0;
};
template<typename F> Handle_<MakeUnaryBase_> MakeUnary_(F func)
{
    struct Mine_ : MakeUnary_
    {
        F func_;
        Handle_<Value_> operator()(const Handle_<Value_>& arg) const override
        {
            return new Unary_<F>(func_, arg);
        }
    };
    return new Mine_{ func };
};

map<String_, Handle_<MakeUnaryBase_>> UnaryBuiltins()
{
    map<String_, Handle_<MakeUnaryBase_>> retval;
    retval["exp"] = MakeUnary_([](double x) { REQUIRE(x < 60.0, "Overflow in exp"); return x > -60.0 ? exp(x) : 0; });
    retval["log"] = MakeUnary_([](double x) { REQUIRE(x > 0.0, "Nonpositive argument to log"); return log(x); });
    retval["sqrt"] = MakeUnary_([](double x) { REQUIRE(x >= 0.0, "Negative argument to sqrt"); return sqrt(x); });
    return retval;
}

Handle_<Value_> MakeUnary(const String_& func, const Handle_<Value_>& arg)
{
    static const map<String_, Handle_<MakeUnaryBase_>> KNOWN = UnaryBuiltins();
    auto p = KNOWN.find(func);
    REQUIRE(p != KNOWN.end(), "No unary function " + func);
    return (*p->second)(arg);
}


struct BaseVol_ : Value_
{
    Handle_<Value_> arg_;
    double evaluate(const Defines_& known) const override
    {
        return known.v_(arg_->evaluate(known));
    }
};

struct NamedVar_ : Value_
{
    String_ name_;
    double evaluate(const Defines_& known) const override
    {
        auto p = known.vals_.find(name_); 
        REQUIRE(p != known.vals_.end(), "That name is not known");
        return p->second;
    }
};

struct With_ : Value_
{
    String_ name_;
    Handle_<Value_> value_, result_;

    With_(const String_& name, const Handle_<Value_>& value, const Handle_<Value_>& result) : name_(name), value_(value), result_(result) {}

    double evaluate(const Defines_& known) const override
    {
        REQUIRE(!known.vals_.count(name_), "Redefinition is not supported");
        Defines_ inner(known.v_);
        inner.vals_ = known.vals_;
        inner.vals_.emplace(name_, value_->evaluate(known));
        return result_->evaluate(inner);
    }
};

Handle_<Value_> ActivateAST(const AST_& formula)
{
    auto children = Apply(ActivateAST, formula.children_);
    switch (formula.head_.kind_)
    {
    case 0:  // NONE; should mean a totally parenthesized expression
        REQUIRE(children.size() == 1, "Unexpected children");
        return children[0];

    case NUMERIC_KIND:
        REQUIRE(children.empty(), "Unexpected children");
        return new Number_(String::ToDouble(formula.head_.val_));

    case ALPHA_KIND:
        if (formula.head_.val_ == "with")
        {   // our special predefined function
            REQUIRE(children.size() == 2, "WITH must have two arguments (assignment, result)");
            REQUIRE(formula.children_[0]->head_.kind_ = '=', "First argument to WITH must be an assignment");
            REQUIRE(formula.children_[0]->children_.size() == 2, "Assignment in a WITH requires left and right sides");
            const AST_& name = *formula.children_[0]->children_[0];
            const AST_& value = *formula.children_[0]->children_[1];
            const AST_& result = *formula.children_[1];
            REQUIRE(name.head_.kind_ == ALPHA_KIND && name.children_.empty(), "Assigned name in WITH must be a simple name");
            return new With_(name.head_.val_, ActivateAST(value), ActivateAST(result));
        }
        else if (formula.head_.val_ == "v")
        {   // calculate vol from base curve
            REQUIRE(formula.children_.size() == 1, "V must have one argument (strike)");
            return new BaseVol_(ActivateAST(*formula.children_[0]));
        }
        else if (formula.children_.empty())
        {   // variable name
            return new NamedVar_(formula.head_.val_);
        }
        else if (formula.children_.size() == 1)
        {   // unary function, must be one of ours

        }
    }
}


class PostShockCurve_ : public VolCurve_
{
    Handle_<VolCurve_> baseCurve_;
    double priceShock_;
    AST_ formula_;


    double operator()(double strike) const override
    {
        Defines_ known(*baseCurve_);
        known.vals_.emplace("x", priceShock_);
        known.vals_.emplace("k", strike);
    }
};

#endif

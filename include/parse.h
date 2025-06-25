#ifndef PARSE_H_
#define PARSE_H_

#include <iostream>
#include <string>
#include <string_view>
#include <map>
#include <memory>
#include <functional>
#include <format>

enum class token_t {
    T_num,
    T_string,
    T_identifier,
    T_plus,
    T_minus,
    T_star, // '*'
    T_div,
    T_open_paren,  /* '(' */
    T_close_paren, /* ')' */
    T_semicolon,   /* ';' */
    T_eof,
};

#ifdef DEBUG
std::map<token_t,std::string> token_str {
    { token_t::T_num,"T_num" },
    { token_t::T_string,"T_string" },
    { token_t::T_identifier,"T_identifier" },
    { token_t::T_plus,"T_plus" },
    { token_t::T_minus,"T_minus" },
    { token_t::T_star,"T_star" },
    { token_t::T_div,"T_div" },
    { token_t::T_open_paren,"T_open_paren" },
    { token_t::T_close_paren,"T_close_paren" },
    { token_t::T_semicolon,"T_semicolon" },
    { token_t::T_eof,"T_eof" },
};
#endif


#define is_identifier(c) ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_')
#define is_number(c) ( c >= '0' && c <= '9')
#define is_eof(c) (c == '\0')
#define is_close_paren(c) (c == ')')
#define is_open_paren(c) (c == '(')
#define is_blank(c) (c == '\n' || c == '\t' || c == '\r' || c == ' ')
#define is_semicolon(c) (c == ';')
#define is_hex_num(c) ( (c >= 'a' && c <= 'f') || (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') )
#define is_operator(c) ( c == '+' || c == '-' || c == '*' || c == '/' )

class Expr {
public:
    Expr()=default;
    virtual int eval()=0;
    virtual ~Expr()=default;
};

class numericExpr : public Expr {
public:
    numericExpr(int _val):value(_val) {}
    ~numericExpr()=default;
    int eval() { return value; }
    int value;
};


class prefixExpr : public Expr {
public:
    prefixExpr(std::unique_ptr<Expr>& _e,token_t _op):e(std::move(_e)),op(_op) {}
    int eval() {
        return 0-e->eval();
    }
    std::unique_ptr<Expr> e;
    token_t op;
};

class binaryExpr : public Expr {
public:
    binaryExpr(std::unique_ptr<Expr> &_l,std::unique_ptr<Expr> &_r,token_t _op)
                                    :lhs(std::move(_l)),rhs(std::move(_r)),op(_op) {}
    ~binaryExpr()=default;
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
    token_t op;
    int eval() {
        switch(op) {
            case token_t::T_plus: {
                return lhs->eval() + rhs->eval();
            }
            case token_t::T_minus: {
                return lhs->eval() - rhs->eval();
            }
            case token_t::T_star: {
                return lhs->eval() * rhs->eval();
            }
            case token_t::T_div: {
                return lhs->eval() / rhs->eval();
            }
        }
    }
};

enum class precedence_t {
    P_none,
    P_atom,   /* numeric,identifier */
    P_factor, /* '+', '-'  */
    P_term,   /* '*','/' */
    P_prefix, /* -1,-2,'-' as prefix*/
};


std::unique_ptr<Expr> parse_numeric();
std::unique_ptr<Expr> parse_prefix();
std::unique_ptr<Expr> parse_binary_expr(std::unique_ptr<Expr>& lhs);
std::unique_ptr<Expr> parse_group_expr();
std::unique_ptr<Expr> parse_expr(precedence_t );

using  prefix_call = std::function<std::unique_ptr<Expr>()>;
using  infix_call = std::function<std::unique_ptr<Expr>(std::unique_ptr<Expr>& )>;


std::unique_ptr<Expr> parse();
#endif
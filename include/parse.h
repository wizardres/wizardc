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
    T_open_paren,   /* '(' */
    T_close_paren,  /* ')' */
    T_semicolon,    /* ';' */
    T_lt,           /* '<' */
    T_gt,           /* '>' */
    T_not,          /* '!' */
    T_assign,       /* '=' */
    T_eq,           /* '=='*/
    T_le,           /* '<='*/
    T_ge,           /* '>='*/
    T_neq,          /* '!='*/
    T_eof,
};


#define is_identifier(c) ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_')
#define is_number(c) ( c >= '0' && c <= '9')
#define is_eof(c) (c == '\0')
#define is_close_paren(c) (c == ')')
#define is_open_paren(c) (c == '(')
#define is_blank(c) (c == '\n' || c == '\t' || c == '\r' || c == ' ')
#define is_semicolon(c) (c == ';')
#define is_hex_num(c) ( (c >= 'a' && c <= 'f') || (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') )
#define is_operator(c) ( c == '+' || c == '-' || c == '*' || c == '/' || c == '=' || c == '<' || c == '>' || c == '!') 
#define is_semecolon(c) ( c == ';')

class Expr {
public:
    Expr()=default;
    virtual ~Expr()=default;
    virtual void codegen()=0;
};

class numericExpr : public Expr {
public:
    numericExpr(int _val):value(_val){}
    ~numericExpr()=default;
    void codegen()override;
    int value;
};


class prefixExpr : public Expr {
public:
    prefixExpr(std::unique_ptr<Expr>& _e,token_t _op):e(std::move(_e)),op(_op){}
    void codegen()override;
    std::unique_ptr<Expr> e;
    token_t op;
};

class binaryExpr : public Expr {
public:
    binaryExpr(std::unique_ptr<Expr> &_l,std::unique_ptr<Expr> &_r,token_t _op)
                                    :lhs(std::move(_l)),rhs(std::move(_r)),op(_op){}
    ~binaryExpr()=default;
    void codegen()override;
    void push() { std::cout << std::format("  push %rax\n"); }
    void pop(std::string_view reg) { std::cout << std::format("  pop {}\n",reg); }

    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
    token_t op;
};

enum class precedence_t {
    P_none,
    P_atom,   /* numeric,identifier */
    P_comparison,  /* '>' '<' '!=' '==' '<=' '>=' */
    P_factor, /* '+'  '-'  */
    P_term,   /* '*' '/' */
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
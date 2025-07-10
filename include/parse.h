#ifndef PARSE_H_
#define PARSE_H_

#include "visitor.h"

#include <iostream>
#include <string>
#include <string_view>
#include <map>
#include <unordered_set>
#include <memory>
#include <functional>
#include <format>
#include <vector>

enum class token_t {
    T_num,
    T_string,
    T_identifier,
    T_keyword,
    T_plus,
    T_minus,
    T_star, // '*'
    T_div,
    T_open_paren,   /* '(' */
    T_close_paren,  /* ')' */
    T_open_square,   /* '[' */
    T_close_square,  /* ']' */
    T_open_block,   /* '{' */
    T_close_block,  /* '}' */
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

enum class node_t {
    N_numeric,
    N_identifier,
    N_prefix,
    N_binary,
};

#define is_identifier(c) ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_')
#define is_number(c) ( c >= '0' && c <= '9')
#define is_eof(c) (c == '\0')
#define is_bracket(c) ( c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' )
#define is_blank(c) (c == '\n' || c == '\t' || c == '\r' || c == ' ')
#define is_semicolon(c) (c == ';')
#define is_hex_num(c) ( (c >= 'a' && c <= 'f') || (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') )
#define is_operator(c) ( c == '+' || c == '-' || c == '*' || c == '/' || c == '=' || c == '<' || c == '>' || c == '!') 
#define is_semecolon(c) ( c == ';')


class Expr {
public:
    Expr()=default;
    Expr(node_t _type,int _idx,int _len):
                   ntype(_type),
                   charidx(_idx),
                   charlen(_len){}
    virtual ~Expr()=default;

    virtual void accept(visitor& vis)=0;

    node_t ntype;
    int charidx;
    int charlen;
};

class numericExpr final: public Expr {
public:
    numericExpr(int _val,int idx,int len):
                    value(_val),
                    Expr(node_t::N_numeric,idx,len){}
    ~numericExpr()=default;

    void accept(visitor& vis)override{
        vis.visit(*this);
    }
    int value;
};

class identifierExpr final: public Expr {
public:
    identifierExpr(int _offset,int idx,int len):
                    offset(_offset),
                    Expr(node_t::N_identifier,idx,len) {}
    ~identifierExpr()=default;

    void accept(visitor& vis)override{
        vis.visit(*this);
    }

    int offset;
    static inline std::map<std::string_view,int> local_vars; 
    static inline int var_offset{0};
};

class prefixExpr final: public Expr {
public:
    prefixExpr(std::unique_ptr<Expr>& _e,token_t _op,int idx,int len)
                :e(std::move(_e)),
                op(_op),
                Expr(node_t::N_prefix,idx,len){}

    void accept(visitor& vis)override{
        vis.visit(*this);
    }

    std::unique_ptr<Expr> e;
    token_t op;
};

class binaryExpr final: public Expr {
public:
    binaryExpr(std::unique_ptr<Expr> &_l,std::unique_ptr<Expr> &_r,token_t _op,int idx,int len)
                                    :lhs(std::move(_l)),
                                     rhs(std::move(_r)),
                                     op(_op),
                                     Expr(node_t::N_binary,idx,len) {}
    ~binaryExpr()=default;
    void push() { std::cout << std::format("  push %rax\n"); }
    void pop(std::string_view reg) { std::cout << std::format("  pop {}\n",reg); }

    void accept(visitor& vis)override{
        vis.visit(*this);
    }

    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
    token_t op;
};

class Stmt {
public:
    Stmt()=default;
    ~Stmt()=default;    
    virtual void accept(visitor& vis)=0;
};

class exprStmt final: public Stmt {
public:
    exprStmt(std::unique_ptr<Expr>& _expr):e(std::move(_expr)){};
    ~exprStmt()=default;

    void accept(visitor& vis)override{
        vis.visit(*this);
    }
    std::unique_ptr<Expr> e;
};


class blockStmt final: public Stmt{
public:
    blockStmt(std::vector<std::unique_ptr<Stmt>>& _stmts):stmts(std::move(_stmts)){}
    ~blockStmt()=default;
    void accept(visitor& vis)override{
        vis.visit(*this);
    }
    std::vector<std::unique_ptr<Stmt>> stmts;
};


class ifStmt final: public Stmt{
public:
    ifStmt(std::unique_ptr<Expr> &_cond,
           std::unique_ptr<Stmt>& _then,
           std::unique_ptr<Stmt>& _else):
                    cond(std::move(_cond)),
                    then(std::move(_then)),
                    elseStmt(std::move(_else)) { }
    ~ifStmt()=default;

    void accept(visitor& vis)override{
        vis.visit(*this);
    }
    std::unique_ptr<Expr> cond;
    std::unique_ptr<Stmt> then;
    std::unique_ptr<Stmt> elseStmt;
    static inline int level{1};
};


enum class precedence_t {
    P_none,
    P_atom,   /* numeric,identifier */
    P_assign, // variable assignment
    P_comparison,  /* '>' '<' '!=' '==' '<=' '>=' */
    P_factor, /* '+'  '-'  */
    P_term,   /* '*' '/' */
    P_prefix, /* -1,-2,'-' as prefix*/
};


std::unique_ptr<Expr> parse_numeric();
std::unique_ptr<Expr> parse_ident();
std::unique_ptr<Expr> parse_prefix();
std::unique_ptr<Expr> parse_binary_expr(std::unique_ptr<Expr>& lhs);
std::unique_ptr<Expr> parse_group_expr();
std::unique_ptr<Expr> parse_expr(precedence_t );

std::unique_ptr<Stmt> parse_stmt();
std::unique_ptr<Stmt> if_stmt();

using  prefix_call = std::function<std::unique_ptr<Expr>()>;
using  infix_call = std::function<std::unique_ptr<Expr>(std::unique_ptr<Expr>& )>;


std::unique_ptr<Stmt> parse();
#endif
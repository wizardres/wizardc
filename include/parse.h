#ifndef PARSE_H_
#define PARSE_H_

#include "visitor.h"
#include "lexer.h"

#include <iostream>
#include <string>
#include <string_view>
#include <map>
#include <unordered_set>
#include <memory>
#include <functional>
#include <format>
#include <vector>


enum class node_t {
    N_numeric,
    N_identifier,
    N_prefix,
    N_binary,
    N_funcall,
};

class Expr {
public:
    Expr()=default;
    Expr(node_t _nt,token _t): ntype(_nt),tok(_t) {}
    virtual ~Expr()=default;

    virtual void accept(visitor& vis)=0;

    node_t ntype;
    token tok;
};

class numericExpr final: public Expr {
public:
    numericExpr(int _val,token _t):
                    value(_val),
                    Expr(node_t::N_numeric,_t){}
    ~numericExpr()=default;

    void accept(visitor& vis)override{
        vis.visit(*this);
    }
    int value;
};

class identExpr final: public Expr {
public:
    identExpr(int _offset,token _t):
                    offset(_offset),
                    Expr(node_t::N_identifier,_t) {}
    ~identExpr()=default;

    void accept(visitor& vis)override{
        vis.visit(*this);
    }

    int offset;
    static inline std::map<std::string,int> local_vars; 
    static inline int var_offset{0};
};

class prefixExpr final: public Expr {
public:
    prefixExpr(std::unique_ptr<Expr>& _e,token _t)
                :e(std::move(_e)),
                Expr(node_t::N_prefix,_t){}

    void accept(visitor& vis)override{
        vis.visit(*this);
    }

    std::unique_ptr<Expr> e;
};

class binaryExpr final: public Expr {
public:
    binaryExpr(std::unique_ptr<Expr> &_l,std::unique_ptr<Expr> &_r,token _t)
                                    :lhs(std::move(_l)),
                                     rhs(std::move(_r)),
                                     Expr(node_t::N_binary,_t) {}
    ~binaryExpr()=default;
    void push() { std::cout << std::format("  push %rax\n"); }
    void pop(std::string_view reg) { std::cout << std::format("  pop {}\n",reg); }

    void accept(visitor& vis)override{
        vis.visit(*this);
    }

    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
};

class funcallExpr final : public Expr {
public:
    funcallExpr(std::string_view name,token _t)
        :funcname(name),
        Expr(node_t::N_funcall,_t) {}

    void accept(visitor& vis)override{
        vis.visit(*this);
    }
    std::string_view funcname;
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

    void accept(visitor& vis)override {
        vis.visit(*this);
    }
    std::unique_ptr<Expr> cond;
    std::unique_ptr<Stmt> then;
    std::unique_ptr<Stmt> elseStmt;
    static inline int level{1};
};

class declStmt final : public Stmt {
public:
    declStmt(std::vector<std::unique_ptr<Expr>>& _decls):decls(std::move(_decls)) {}
    ~declStmt()=default;

    void accept(visitor& vis)override {
        vis.visit(*this);
    }
    std::vector<std::unique_ptr<Expr>> decls;
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
std::unique_ptr<Stmt> decl_stmt();

using  prefix_call = std::function<std::unique_ptr<Expr>()>;
using  infix_call = std::function<std::unique_ptr<Expr>(std::unique_ptr<Expr>& )>;

std::unique_ptr<Stmt> parse(const char *str);
#endif
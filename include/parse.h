#ifndef PARSE_H_
#define PARSE_H_

#include "visitor.h"
#include "lexer.h"
#include "scope.h"

#include <iostream>
#include <string>
#include <string_view>
#include <map>
#include <unordered_set>
#include <memory>
#include <functional>
#include <format>
#include <vector>
#include <array>


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
                    Expr(node_t::N_numeric,_t),value(_val) {}
    ~numericExpr()=default;

    void accept(visitor& vis)override{
        vis.visit(*this);
    }
    int value;
};

class identExpr final: public Expr {
public:
    identExpr(int _offset,token _t):
                    Expr(node_t::N_identifier,_t),offset(_offset) {}
    ~identExpr()=default;

    void accept(visitor& vis)override{
        vis.visit(*this);
    }

    int offset;
};

class prefixExpr final: public Expr {
public:
    prefixExpr(std::unique_ptr<Expr>& _e,token _t)
                :Expr(node_t::N_prefix,_t),e(std::move(_e)) {}

    void accept(visitor& vis)override{
        vis.visit(*this);
    }

    std::unique_ptr<Expr> e;
};

class binaryExpr final: public Expr {
public:
    binaryExpr(std::unique_ptr<Expr> &_l,std::unique_ptr<Expr> &_r,token _t)
                                    :Expr(node_t::N_binary,_t),
                                     lhs(std::move(_l)),
                                     rhs(std::move(_r)) {}
    ~binaryExpr()=default;

    void accept(visitor& vis)override{
        vis.visit(*this);
    }

    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
};

class funcallExpr final : public Expr {
public:
    funcallExpr(std::string_view name,std::vector<std::unique_ptr<Expr>> &_args,token _t)
        :Expr(node_t::N_funcall,_t),
         funcname(name),
         args(std::move(_args)) {}

    void accept(visitor& vis)override{
        vis.visit(*this);
    }
    std::string funcname;
    std::vector<std::unique_ptr<Expr>> args;
};

class Stmt {
public:
    Stmt()=default;
    ~Stmt()=default;    
    virtual void accept(visitor& vis)=0;
};

class exprStmt final: public Stmt {
public:
    exprStmt(std::unique_ptr<Expr>& _expr):e(std::move(_expr)){}
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

class retStmt final : public Stmt {
public:
    retStmt(std::unique_ptr<Stmt>&_e):e(std::move(_e)) {}
    void accept(visitor& vis)override{
        vis.visit(*this);
    }
    std::unique_ptr<Stmt> e;
    static inline std::string fname;
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

class vardef final : public Stmt {
public:
    vardef(std::vector<std::unique_ptr<Expr>>& _decls):decls(std::move(_decls)) {}
    ~vardef()=default;

    void accept(visitor& vis)override {
        vis.visit(*this);
    }
    std::vector<std::unique_ptr<Expr>> decls;
};

class funcdef final : public Stmt {
public:
    funcdef( std::unique_ptr<Stmt>& _b,std::string _n):
                    body(std::move(_b)),
                    name(std::move(_n)){}
    void accept(visitor& vis)override{
        vis.visit(*this);
    }
    std::unique_ptr<Stmt> body;
    std::string name;
    static inline int stacksize{0};
};


class Prog {
public:
    Prog()=default;
    Prog(std::vector<std::unique_ptr<Stmt>> &p):stmts(std::move(p)){}
    void accept(visitor& vis){
        vis.visit(*this);
    }
    std::vector<std::unique_ptr<Stmt>> stmts;
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

Prog parse(const char *str);
#endif
#ifndef AST_H_
#define AST_H_

#include "visitor.h"
#include "lexer.h"
#include "scope.h"
#include "type.h"

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

class Expr {
public:
    Expr()=default;
    virtual ~Expr()=default;
    virtual std::shared_ptr<Type> getType()const=0;
    virtual size_t typeSize()const=0;
    virtual void accept(visitor& vis)=0;
};

class numericExpr final: public Expr {
public:
    numericExpr(int val, std::shared_ptr<Type> type): _value(val), _type(type) {}
    ~numericExpr()=default;
    
    void accept(visitor& vis)override{ vis.visit(*this); }
    std::shared_ptr<Type> getType()const override { return _type; }
    size_t typeSize()const override { return _type->getSize(); };
    int Value()const { return _value; }
private:
    int _value;
    std::shared_ptr<Type> _type;
};


class identExpr final: public Expr {
public:
    identExpr(std::string& name,std::shared_ptr<Obj> obj):
             _name(std::move(name)),
             _obj(obj) {}

    std::string_view getName()const { return _name; }
    std::shared_ptr<Type> getType()const override { return _obj->getType(); }
    size_t typeSize()const override { return _obj->getTypeSize(); }
    bool isGlobal() { return static_cast<varObj*>(_obj.get())->isGlobal(); }
    int  getOffset() { return static_cast<varObj*>(_obj.get())->getOffset(); }
    void accept(visitor& vis)override { vis.visit(*this); }
private:
    std::string _name;
    std::shared_ptr<Obj> _obj;
};


class prefixExpr final: public Expr {
public:
    prefixExpr(std::unique_ptr<Expr>& expr,std::shared_ptr<Type> type) :_expr(std::move(expr)), _type(type) {}

    void accept(visitor& vis)override{ vis.visit(*this); }
    std::shared_ptr<Type> getType()const override { return _type; }
    size_t typeSize()const override { return _type->getSize(); };
    const std::unique_ptr<Expr>& getExpr()const { return _expr; }
private:
    std::unique_ptr<Expr> _expr;
    std::shared_ptr<Type> _type;
};


class binaryExpr final: public Expr {
public:
    binaryExpr(token_t op,
               std::unique_ptr<Expr> &lhs,
               std::unique_ptr<Expr> &rhs,
               std::shared_ptr<Type> type):
                    _op(op),
                    _lhs(std::move(lhs)),
                    _rhs(std::move(rhs)),
                    _type(type) {}

    ~binaryExpr()=default;
    void accept(visitor& vis)override{ vis.visit(*this); }
    virtual std::shared_ptr<Type> getType()const override { return _type; }
    virtual size_t typeSize()const override { return _type->getSize(); }

    token_t getOp()const { return _op; }
    const std::unique_ptr<Expr>& getLhs()const { return _lhs; }
    const std::unique_ptr<Expr>& getRhs()const { return _rhs; }
private:
    token_t _op;
    std::unique_ptr<Expr> _lhs;
    std::unique_ptr<Expr> _rhs;
    std::shared_ptr<Type> _type;
};


class funcallExpr final : public Expr {
public:
    funcallExpr(std::string& name, 
                std::vector<std::unique_ptr<Expr>>& args,
                std::shared_ptr<Type> type):
               _name(name), 
               _args(std::move(args)),
               _type(type) {}

    void accept(visitor& vis)override{ vis.visit(*this); }
    std::string_view getName()const { return _name; }
    const std::vector<std::unique_ptr<Expr>> &getArgs()const { return _args; }

    std::shared_ptr<Type> getType()const override { return _type; }
    size_t typeSize()const override { return _type->getSize(); }

private:
    std::string _name;
    std::vector<std::unique_ptr<Expr>> _args;
    std::shared_ptr<Type> _type;
};


class Stmt {
public:
    Stmt()=default;
    ~Stmt()=default;    
    virtual void accept(visitor& vis)=0;
};

class exprStmt final: public Stmt {
public:
    exprStmt(std::unique_ptr<Expr>& expr):_e(std::move(expr)){}
    ~exprStmt()=default;

    void accept(visitor& vis)override{ vis.visit(*this); }

    const std::unique_ptr<Expr>& getExpr()const {  return _e;}
private:
    std::unique_ptr<Expr> _e;
};


class blockStmt final: public Stmt{
public:
    blockStmt(std::vector<std::unique_ptr<Stmt>>& _stmts):stmts(std::move(_stmts)){}
    ~blockStmt()=default;
    void accept(visitor& vis)override{ vis.visit(*this); }
    const std::vector<std::unique_ptr<Stmt>> &getStmts()const { return stmts; }

private:
    std::vector<std::unique_ptr<Stmt>> stmts;
};

class retStmt final : public Stmt {
public:
    retStmt(std::unique_ptr<Stmt>&e):_e(std::move(e)) {}
    void accept(visitor& vis)override{ vis.visit(*this); }

    const std::unique_ptr<Stmt>& getStmt()const { return _e; }
    static std::string_view getName() { return _funcname; }
    static void setFuncName(const std::string& name) { _funcname = name; }
private:
    std::unique_ptr<Stmt> _e;
    static inline std::string _funcname;
};


class ifStmt final: public Stmt{
public:
    ifStmt(std::unique_ptr<Expr> &cond,
           std::unique_ptr<Stmt>& then,
           std::unique_ptr<Stmt>& elseStmt):
                    _cond(std::move(cond)),
                    _then(std::move(then)),
                    _elseStmt(std::move(elseStmt)) { }
    ~ifStmt()=default;

    void accept(visitor& vis)override { vis.visit(*this); }
    int levelUp() { return _level++; }

    const std::unique_ptr<Expr>& getCond()const { return _cond; }
    const std::unique_ptr<Stmt>& getThen()const { return _then; }
    const std::unique_ptr<Stmt>& getElse()const { return _elseStmt; }
private:
    std::unique_ptr<Expr> _cond;
    std::unique_ptr<Stmt> _then;
    std::unique_ptr<Stmt> _elseStmt;
    static inline int _level{1};
};


class vardef final : public Stmt {
public:
    vardef(std::vector<std::unique_ptr<Expr>>& _decls):decls(std::move(_decls)) {}
    ~vardef()=default;

    void accept(visitor& vis)override { vis.visit(*this); }
    bool isGlobal() { return static_cast<identExpr*>(decls.front().get())->isGlobal(); }
    const std::vector<std::unique_ptr<Expr>>& getDeclas()const { return decls; }
private:
    std::vector<std::unique_ptr<Expr>> decls;
};

class funcdef final : public Stmt {
public:
    funcdef( std::unique_ptr<Stmt>& _b,
             std::string& _n,
             std::vector<std::unique_ptr<Expr>> &_params,
             int _stackoff):
                    _body(std::move(_b)),
                    _name(std::move(_n)),
                    _params(std::move(_params)),
                    _stackoff(_stackoff) {}

    static int newlocalVar(int size) {
        _stacksize += size;
        return -_stacksize;
    }
    static int getStaksize() { return _stacksize; }
    static void stackrelease() { _stacksize = 0; }

    static int align(int align) { return (_stacksize + align -1) / align * align; }
    void accept(visitor& vis)override{ vis.visit(*this); }

    static std::unique_ptr<Stmt> newFunction(
                          std::unique_ptr<Stmt>& _b,
                          std::string& _n,
                          std::vector<std::unique_ptr<Expr>> &_params) {
        std::unique_ptr<Stmt> func = std::make_unique<funcdef>(_b,_n,_params,align(16));
        stackrelease();
        return func;
    }

    const std::unique_ptr<Stmt>& getBody()const { return _body; }
    const std::string& getName()const { return _name;}
    const std::vector<std::unique_ptr<Expr>>& getParams()const { return _params; }
    int getStackOff() { return _stackoff; }
private:
    std::unique_ptr<Stmt> _body;
    std::string _name;
    std::vector<std::unique_ptr<Expr>> _params;
    int _stackoff;
    static inline int _stacksize{0};
};


class Prog {
public:
    Prog()=default;
    ~Prog()=default;
    Prog(std::vector<std::unique_ptr<Stmt>> &stmt):_stmts(std::move(stmt)){}
    void accept(visitor& vis){ vis.visit(*this); }
    std::vector<std::unique_ptr<Stmt>> _stmts;
};
#endif
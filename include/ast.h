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
    enum class Kind { N_number,N_identifier,N_deref,N_addr,N_trivial,N_funcall,N_binary };
    Expr()=default;
    virtual ~Expr()=default;
    virtual std::shared_ptr<Type> getType()const=0;
    virtual size_t typeSize()const=0;
    virtual Kind getKind()const=0;
    virtual size_t strLength()const=0;
    virtual size_t strStart()const=0;
    virtual std::string strView()const=0;
    virtual void accept(visitor& vis)=0;

    static bool ndequal(const std::unique_ptr<Expr>& expr,Expr::Kind kind) { return expr->getKind() == kind; }
};


class numericExpr final: public Expr {
public:
    numericExpr(int val, std::shared_ptr<Type> type,token &tok):
               _value(val),
               _type(type),
               _tok(tok) {}
    ~numericExpr()=default;
    
    std::shared_ptr<Type> getType()const override { return _type; }
    size_t typeSize()const override { return _type->getSize(); };
    Kind getKind()const override { return Kind::N_number; }
    
    size_t strLength()const override{ return _tok.str.length(); }
    size_t strStart()const override{ return _tok.start; }
    std::string strView()const override { return std::string(_tok.str.data(),strLength()); }

    int Value()const { return _value; }

    void accept(visitor& vis) override{ vis.visit(*this); }
private:
    int _value;
    std::shared_ptr<Type> _type;
    token _tok;
};


class identExpr final: public Expr {
public:
    identExpr(std::string_view name,std::shared_ptr<Obj> obj,token &tok):
             _name(name),
             _obj(obj),
             _tok(tok) {}

    std::string_view getName()const { return _name; }

    std::shared_ptr<Type> getType()const override { return _obj->getType(); }
    size_t typeSize()const override { return _obj->getTypeSize(); }
    Kind getKind()const override { return Kind::N_identifier; }

    bool isGlobal() { return static_cast<varObj*>(_obj.get())->isGlobal(); }
    int  getOffset() { return static_cast<varObj*>(_obj.get())->getOffset(); }

    size_t strLength()const override{ return _tok.str.length(); }
    std::string strView()const override { return std::string(_tok.str.data(),strLength()); }
    size_t strStart()const override{ return _tok.start; }

    void accept(visitor& vis)override { vis.visit(*this); }
private:
    std::string_view _name;
    std::shared_ptr<Obj> _obj;
    token _tok;
};


class prefixExpr final: public Expr {
public:
    prefixExpr(std::unique_ptr<Expr>& expr,
               std::shared_ptr<Type> type,
               Kind kind,
               token &tok):
              _expr(std::move(expr)),
              _type(type),
              _kind(kind),
              _tok(tok) {}

    void accept(visitor& vis)override{ vis.visit(*this); }

    std::shared_ptr<Type> getType()const override { return _type; }
    size_t typeSize()const override { return _type->getSize(); };

    const std::unique_ptr<Expr>& getExpr()const { return _expr; }
    Kind getKind()const override { return _kind; }

    size_t strLength()const override{ return _tok.str.length() + _expr->strLength(); }
    size_t strStart()const override{ return _tok.start; }
    std::string strView()const override { return std::string(_tok.str.data(),1) + _expr->strView(); }
private:
    std::unique_ptr<Expr> _expr;
    std::shared_ptr<Type> _type;
    Kind _kind;
    token _tok;
};


class binaryExpr final: public Expr {
public:
    binaryExpr(token &op,
               std::unique_ptr<Expr> &lhs,
               std::unique_ptr<Expr> &rhs,
               std::shared_ptr<Type> type):
               _op(op),
               _lhs(std::move(lhs)),
               _rhs(std::move(rhs)),
               _type(type){}

    ~binaryExpr()=default;
    void accept(visitor& vis)override{ vis.visit(*this); }

    virtual std::shared_ptr<Type> getType()const override { return _type; }
    virtual size_t typeSize()const override { return _type->getSize(); }

    token_t getOp()const { return _op.type; }
    const std::unique_ptr<Expr>& getLhs()const { return _lhs; }
    const std::unique_ptr<Expr>& getRhs()const { return _rhs; }
    Kind getKind()const override { return Kind::N_binary; }

    size_t strLength()const override { return _op.str.length() + _lhs->strLength() + _rhs->strLength(); }
    size_t strStart()const override{ return _lhs->strStart(); }
    std::string strView()const override { return _lhs->strView() + std::string(_op.str.data(),_op.str.length()) + _rhs->strView();}
private:
    token _op;
    std::unique_ptr<Expr> _lhs;
    std::unique_ptr<Expr> _rhs;
    std::shared_ptr<Type> _type;
};


class funcallExpr final : public Expr {
public:
    funcallExpr(token &tok,
                std::string_view name, 
                std::vector<std::unique_ptr<Expr>>& args,
                std::shared_ptr<Type> type):
               _tok(tok),
               _name(name), 
               _args(std::move(args)),
               _retType(type) {}

    std::string_view getName()const { return _name; }
    const std::vector<std::unique_ptr<Expr>> &getArgs()const { return _args; }

    std::shared_ptr<Type> getType()const override { return _retType; }
    size_t typeSize()const override { return _retType->getSize(); }
    Kind getKind()const override { return Kind::N_funcall; }

    size_t strLength()const override { return _tok.str.length(); }
    size_t strStart()const override{ return _tok.start; }
    std::string strView()const override { return std::string(_tok.str.data(),_tok.str.length()); }

    void accept(visitor& vis)override{ vis.visit(*this); }
private:
    token _tok;
    std::string_view _name;
    std::vector<std::unique_ptr<Expr>> _args;
    std::shared_ptr<Type> _retType;
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
    static void setFuncName(std::string_view name) { _funcname = name; }
private:
    std::unique_ptr<Stmt> _e;
    static inline std::string_view _funcname;
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
    vardef(std::vector<std::unique_ptr<Expr>>& _decls,bool isglobal):
           decls(std::move(_decls)),
           _isglobal(isglobal) {}
    ~vardef()=default;

    void accept(visitor& vis)override { vis.visit(*this); }
    bool isGlobal() { return _isglobal; }
    const std::vector<std::unique_ptr<Expr>>& getDeclas()const { return decls; }
private:
    std::vector<std::unique_ptr<Expr>> decls;
    bool _isglobal;
};

class funcdef final : public Stmt {
public:
    funcdef( std::unique_ptr<Stmt>& _b,
             std::string_view _n,
             std::vector<std::unique_ptr<Expr>> &_params,
             int _stackoff):
                    _body(std::move(_b)),
                    _name(_n),
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
                          std::string_view _n,
                          std::vector<std::unique_ptr<Expr>> &_params) {
        std::unique_ptr<Stmt> func = std::make_unique<funcdef>(_b,_n,_params,align(16));
        stackrelease();
        return func;
    }

    const std::unique_ptr<Stmt>& getBody()const { return _body; }
    std::string_view getName()const { return _name;}
    const std::vector<std::unique_ptr<Expr>>& getParams()const { return _params; }
    int getStackOff() { return _stackoff; }
private:
    std::unique_ptr<Stmt> _body;
    std::string_view _name;
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
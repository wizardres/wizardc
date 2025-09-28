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

class Node {
public:
    enum class Kind { N_number,N_identifier,N_deref,N_addr,N_trivial,N_funcall,N_binary,N_arrayvisit,N_arraydef };
    Node()=default;
    virtual ~Node()=default;
    virtual std::shared_ptr<Type> getType()const=0;
    virtual size_t typeSize()const=0;
    virtual size_t strLength()const=0;
    virtual size_t strStart()const=0;
    virtual std::string strView()const=0;
    virtual bool equal(Node::Kind kind)const=0;
    virtual void accept(visitor& vis)=0;
};


class numericNode final: public Node {
public:
    numericNode(int val,token tok):
               _value(val),
               _tok(tok) {}
    numericNode()=default;
    ~numericNode()=default;
    
    std::shared_ptr<Type> getType()const override { return typeFactor::getInt(Type::Kind::T_int); }
    size_t typeSize()const override { return getType()->getSize(); };
    bool equal(Node::Kind kind)const override { return kind == Kind::N_number; }
    
    size_t strLength()const override{ return _tok.str.length(); }
    size_t strStart()const override{ return _tok.start; }
    std::string strView()const override { return std::string(_tok.str.data(),strLength()); }

    int Value()const { return _value; }

    void accept(visitor& vis) override{ vis.visit(*this); }
private:
    int _value;
    token _tok;
};



class identNode final: public Node {
public:
    identNode(std::shared_ptr<Obj> obj): _obj(obj) {}
    identNode()=default;
    ~identNode()=default;

    std::string_view getName()const { return _obj->getToken().str; }

    std::shared_ptr<Type> getType()const override { return _obj->getType(); }
    size_t typeSize()const override { return _obj->getObjSize(); }
    bool equal(Node::Kind kind)const override { return kind == Kind::N_identifier; }
    void accept(visitor& vis) override{ vis.visit(*this); }


    size_t strLength()const override{ return _obj->getToken().str.length(); }
    std::string strView()const override { return std::string(_obj->getToken().str.data(),strLength()); }
    size_t strStart()const override { return _obj->getToken().start; };

    bool isGlobal() { return static_cast<varObj*>(_obj.get())->isGlobal(); }
    int  getOffset() { return static_cast<varObj*>(_obj.get())->getOffset(); }
private:
    std::shared_ptr<Obj> _obj;
};


class arrayVisit final : public Node {
public:
    arrayVisit(std::shared_ptr<Obj> obj,
               size_t idx,token tok):
               _obj(obj),
               _idx(idx),
               _tok(tok) {}
    arrayVisit()=default;
    ~arrayVisit()=default;

    bool isGlobal() { return static_cast<varObj*>(_obj.get())->isGlobal(); }

    std::string_view getName()const { return _obj->getToken().str; }
    int elemOffset()const { return _idx * typeSize(); }
    int arrOffset()const { return static_cast<varObj*>(_obj.get())->getOffset(); }

    size_t typeSize()const override{ return static_cast<arrayObj*>(_obj.get())->getElemSize(); }
    std::shared_ptr<Type> getType()const override {  return static_cast<arrayObj*>(_obj.get())->getElemType(); }
    bool equal(Node::Kind kind)const override { return kind == Kind::N_arrayvisit; }
    
    size_t strLength()const override{ return _tok.str.length(); }
    size_t strStart()const override{ return _tok.start; }
    std::string strView()const override { return std::string(_tok.str.data(),strLength()); }

    void accept(visitor& vis) override{ vis.visit(*this); }
private:
    std::shared_ptr<Obj> _obj;
    size_t _idx;
    token _tok;
};


class arraydef final : public Node {
public:
    arraydef(std::shared_ptr<Obj> obj,
             std::vector<std::shared_ptr<Node>>& init):
            _obj(obj),
            _init_lst(std::move(init)) {}
    arraydef()=default;
    ~arraydef()=default;

    std::string_view getName()const { return _obj->getToken().str; }

    size_t typeSize()const override{ return static_cast<arrayObj*>(_obj.get())->getObjSize(); }
    size_t elemSize()const { return static_cast<arrayObj*>(_obj.get())->getElemSize(); }
    std::shared_ptr<Type> getType()const override {  return _obj->getType(); }
    bool equal(Node::Kind kind)const override { return kind == Kind::N_arraydef; }

    size_t strLength()const override{ return _obj->getToken().str.length(); }
    std::string strView()const override { return std::string(_obj->getToken().str.data(),strLength()); }
    size_t strStart()const override{ return _obj->getToken().start; }

    bool isGlobal() { return static_cast<arrayObj*>(_obj.get())->isGlobal(); }
    int  getOffset() { return static_cast<arrayObj*>(_obj.get())->getOffset(); }
    void accept(visitor& vis) override{ vis.visit(*this); }
    const std::vector<std::shared_ptr<Node>>& get_init_lst()const { return _init_lst; }
private:
    std::shared_ptr<Obj> _obj;
    std::vector<std::shared_ptr<Node>> _init_lst;
};


class prefixNode final: public Node {
public:
    prefixNode(std::shared_ptr<Node> expr,
               std::shared_ptr<Type> type,
               Kind kind,
               token tok):
              _expr(expr),
              _type(type),
              _kind(kind),
              _tok(tok) {}
    prefixNode()=default;
    ~prefixNode()=default;

    void accept(visitor& vis) override{ vis.visit(*this); }

    std::shared_ptr<Type> getType()const override { return _type; }
    size_t typeSize()const override { return _type->getSize(); };

    std::shared_ptr<Node> getNode()const { return _expr; }
    bool equal(Node::Kind kind)const override { return kind == _kind; }

    size_t strLength()const override{ return _tok.str.length() + _expr->strLength(); }
    size_t strStart()const override{ return _tok.start; }
    std::string strView()const override { return std::string(_tok.str.data(),1) + _expr->strView(); }
private:
    std::shared_ptr<Node> _expr;
    std::shared_ptr<Type> _type;
    Kind _kind;
    token _tok;
};


class binaryNode final: public Node {
public:
    binaryNode(token op,
               std::shared_ptr<Node> lhs,
               std::shared_ptr<Node> rhs,
               std::shared_ptr<Type> type):
               _op(op),
               _lhs(lhs),
               _rhs(rhs),
               _type(type){}

    binaryNode()=default;
    ~binaryNode()=default;

    void accept(visitor& vis) override{ vis.visit(*this); }
    virtual std::shared_ptr<Type> getType()const override { return _type; }
    virtual size_t typeSize()const override { return _type->getSize(); }

    tokenType getOp()const { return _op.type; }
    std::shared_ptr<Node> getLhs()const { return _lhs; }
    std::shared_ptr<Node> getRhs()const { return _rhs; }
    bool equal(Node::Kind kind)const override { return kind == Node::Kind::N_binary; }

    size_t strLength()const override { return _op.str.length() + _lhs->strLength() + _rhs->strLength(); }
    size_t strStart()const override{ return _lhs->strStart(); }
    std::string strView()const override { return _lhs->strView() + std::string(_op.str.data(),_op.str.length()) + _rhs->strView();}
private:
    token _op;
    std::shared_ptr<Node> _lhs;
    std::shared_ptr<Node> _rhs;
    std::shared_ptr<Type> _type;
};


class funcallNode final : public Node {
public:
    funcallNode(token tok,
                std::vector<std::shared_ptr<Node>>& args,
                std::shared_ptr<Obj> obj):
               _tok(tok),
               _args(std::move(args)),
               _funcobj(obj) {}
    funcallNode()=default;
    ~funcallNode()=default;

    std::string_view getName()const { return _tok.str; }
    const std::vector<std::shared_ptr<Node>> &getArgs()const { return _args; }

    std::shared_ptr<Type> getType()const override { return static_cast<funcObj*>(_funcobj.get())->getRetType(); }
    size_t typeSize()const override { return _funcobj->getObjSize(); }
    bool equal(Node::Kind kind)const override { return kind == Node::Kind::N_funcall; }

    size_t strLength()const override { return _tok.str.length(); }
    size_t strStart()const override{ return _tok.start; }
    std::string strView()const override { return std::string(_tok.str.data(),_tok.str.length()); }

    void accept(visitor& vis) override{ vis.visit(*this); }
private:
    token _tok;
    std::vector<std::shared_ptr<Node>> _args;
    std::shared_ptr<Obj> _funcobj;
};


class Stmt {
public:
    Stmt()=default;
    virtual ~Stmt()=default;    
    virtual void accept(visitor& vis)=0;
};

class exprStmt final: public Stmt {
public:
    exprStmt(std::shared_ptr<Node>& expr):_e(std::move(expr)){}
    exprStmt()=default;
    ~exprStmt()=default;

    void accept(visitor& vis) override{ vis.visit(*this); }

    const std::shared_ptr<Node>& getNode()const {  return _e;}
private:
    std::shared_ptr<Node> _e;
};


class blockStmt final: public Stmt{
public:
    blockStmt(std::vector<std::shared_ptr<Stmt>>& _stmts):stmts(std::move(_stmts)){}
    blockStmt()=default;
    ~blockStmt()=default;
    void accept(visitor& vis) override{ vis.visit(*this); }
    const std::vector<std::shared_ptr<Stmt>> &getStmts()const { return stmts; }

private:
    std::vector<std::shared_ptr<Stmt>> stmts;
};

class retStmt final : public Stmt {
public:
    retStmt()=default;
    ~retStmt()=default;
    retStmt(std::shared_ptr<Stmt>&e):_e(std::move(e)) {}
    void accept(visitor& vis) override{ vis.visit(*this); }

    const std::shared_ptr<Stmt>& getStmt()const { return _e; }
    static std::string_view getName() { return _funcname; }
    static void setFuncName(std::string_view name) { _funcname = name; }
private:
    std::shared_ptr<Stmt> _e;
    static inline std::string_view _funcname;
};


class ifStmt final: public Stmt{
public:
    ifStmt(std::shared_ptr<Node> &cond,
           std::shared_ptr<Stmt>& then,
           std::shared_ptr<Stmt>& elseStmt):
                    _cond(std::move(cond)),
                    _then(std::move(then)),
                    _elseStmt(std::move(elseStmt)) { }
    ifStmt()=default;
    ~ifStmt()=default;

    void accept(visitor& vis) override{ vis.visit(*this); }
    int levelUp() { return _level++; }

    const std::shared_ptr<Node>& getCond()const { return _cond; }
    const std::shared_ptr<Stmt>& getThen()const { return _then; }
    const std::shared_ptr<Stmt>& getElse()const { return _elseStmt; }
private:
    std::shared_ptr<Node> _cond;
    std::shared_ptr<Stmt> _then;
    std::shared_ptr<Stmt> _elseStmt;
    static inline int _level{1};
};


class vardef final : public Stmt {
public:
    vardef()=default;
    vardef(std::vector<std::shared_ptr<Node>>& _decls,bool isglobal):
           decls(std::move(_decls)),
           _isglobal(isglobal) {}
    ~vardef()=default;

    void accept(visitor& vis) override{ vis.visit(*this); }
    bool isGlobal() { return _isglobal; }
    const std::vector<std::shared_ptr<Node>>& getDeclas()const { return decls; }
private:
    std::vector<std::shared_ptr<Node>> decls;
    bool _isglobal;
};



class funcdef final : public Stmt {
public:
    funcdef()=default;
    funcdef( std::shared_ptr<Stmt>& _b,
             std::string_view _n,
             std::vector<std::shared_ptr<Node>> &_params,
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
    void accept(visitor& vis) override{ vis.visit(*this); }

    static std::shared_ptr<Stmt> newFunction(
                          std::shared_ptr<Stmt>& _b,
                          std::string_view _n,
                          std::vector<std::shared_ptr<Node>> &_params) {
        std::shared_ptr<Stmt> func = std::make_shared<funcdef>(_b,_n,_params,align(16));
        stackrelease();
        return func;
    }

    const std::shared_ptr<Stmt>& getBody()const { return _body; }
    std::string_view getName()const { return _name;}
    const std::vector<std::shared_ptr<Node>>& getParams()const { return _params; }
    int getStackOff() { return _stackoff; }
private:
    std::shared_ptr<Stmt> _body;
    std::string_view _name;
    std::vector<std::shared_ptr<Node>> _params;
    int _stackoff;
    static inline int _stacksize{0};
};


class Prog {
public:
    Prog()=default;
    ~Prog()=default;
    Prog(std::vector<std::shared_ptr<Stmt>> &stmt):_stmts(std::move(stmt)){}
    void accept(visitor& vis){ vis.visit(*this); }
    std::vector<std::shared_ptr<Stmt>> _stmts;
};
#endif
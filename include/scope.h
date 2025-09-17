#ifndef SYMBOL_H_
#define SYMBOL_H_

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include "type.h"

class Obj{
public:
    enum class objKind { variable, function , array };    
    virtual ~Obj()=default;
    virtual objKind getKind()const=0;
    virtual std::shared_ptr<Type> getType()const=0;
    virtual size_t getObjSize()const=0;
    virtual const token &getToken()const=0;
};


class varObj : public Obj{
public:
    varObj()=default;
    ~varObj()=default;
    varObj(const token &tok,
           std::shared_ptr<Type> type,
           bool isglobal,
           int offset):
        _tok(tok),
        _type(std::move(type)),
        _isglobal(isglobal),
        _offset(offset) {}

    objKind getKind()const override { return objKind::variable; };
    std::shared_ptr<Type> getType()const override{ return _type; }
    size_t getObjSize()const override { return _type->getSize(); }
    virtual const token &getToken()const override { return _tok; }

    int getOffset() { return _offset; }
    bool isGlobal() { return _isglobal; }
private:
    token _tok;
    std::shared_ptr<Type> _type;
    bool _isglobal;
    int _offset;
};




class arrayObj : public Obj {
public:
    arrayObj(const token &tok,
             std::shared_ptr<Type> ty,
             bool isglobal,
             int offset):
             _tok(tok),
             _arrayTy(ty),
             _isglobal(isglobal),
             _offset(offset) {}
    ~arrayObj()=default;

    std::shared_ptr<Type> getElemType()const { return static_cast<arrayType*>(_arrayTy.get())->elemTy(); }
    size_t getElemSize()const { return static_cast<arrayType*>(_arrayTy.get())->elemSize(); }
    size_t len()const { return static_cast<arrayType*>(_arrayTy.get())->getlen(); }

    size_t getObjSize()const override { return _arrayTy->getSize(); }
    objKind getKind()const override { return objKind::array; };
    std::shared_ptr<Type> getType()const override{ return _arrayTy; }
    virtual const token &getToken()const override { return _tok; }

    int getOffset() { return _offset; }
    bool isGlobal() { return _isglobal; }
private:
    token _tok;
    std::shared_ptr<Type> _arrayTy;
    bool _isglobal;
    int _offset;
};


class funcObj : public Obj {
public:
    funcObj()=default;
    ~funcObj()=default;
    funcObj(const token& tok,std::shared_ptr<Type> type):
            _tok(tok),
            _type(type) {}

    std::shared_ptr<Type> getType()const override{ return static_cast<funcType*>(_type.get())->getRetType(); }
    const std::vector<std::shared_ptr<Type>> &getParamTypes()const { return static_cast<funcType*>(_type.get())->getParamTypes(); }
    size_t getObjSize()const override { return _type->getSize(); }

    objKind getKind()const override { return objKind::function; };
    const token &getToken()const override { return _tok; }
private:
    token _tok;
    std::shared_ptr<Type> _type;
};


class objFactor {
public:
    static std::shared_ptr<Obj> getVariable(token& tok,std::shared_ptr<Type> type,bool isglobal,int offset) {
        return std::make_shared<varObj>(tok,type,isglobal,offset);
    }
    static std::shared_ptr<Obj> getArray(const token& tok,std::shared_ptr<Type> type,bool isglobal,int offset) {
        return std::make_shared<arrayObj>(tok,type,isglobal,offset);
    }
    static std::shared_ptr<Obj> getFunction(const token& tok,std::shared_ptr<Type> type) {
        return std::make_shared<funcObj>(tok,type);
    }
};



class symbol {
public:
    symbol()=default;
    void insert(std::string_view name,std::shared_ptr<Obj> obj) {
        objs.insert({name,obj});
    }
    std::optional<std::shared_ptr<Obj>> lookup(std::string_view name) {
        auto it = objs.find(name);
        if(it != objs.end()) {
            return it->second;
        }
        return std::nullopt;
    }
private:
    std::unordered_map<std::string_view,std::shared_ptr<Obj>> objs;
};


class Scope{
public:
    void enter(){
        cur++;
        localVars.push_back(symbol{});
    }
    void leave(){
        localVars.pop_back();
        cur--;
    }
    bool isglobal(){
        return cur == -1;
    }

    bool insertObj(bool isglobal,std::string_view name,std::shared_ptr<Obj> obj) {
        if(isglobal) return insert_global(name,obj);
        else return insert_local(name,obj);
    }

    std::optional<std::shared_ptr<Obj>> lookup_allscope(std::string_view name){
        for(int i = cur; i >= 0; i--){
            symbol& s = localVars[i];
            auto it = s.lookup(name);
            if(it.has_value()){
                return it.value();
            }
        }
        return lookup_global(name);
    }
    std::optional<std::shared_ptr<Obj>> lookup_global(std::string_view name) {
        return globalVars.lookup(name);
    }

    std::optional<std::shared_ptr<Obj>> lookup_curscope(std::string_view name) {
        return localVars[cur].lookup(name);
    }
private:
    bool insert_local(std::string_view name,std::shared_ptr<Obj> obj) {
        if(lookup_curscope(name).has_value()) return false;
        localVars[cur].insert(name,obj);
        return true;
    }

    bool insert_global(std::string_view name,std::shared_ptr<Obj> obj) {
        if(globalVars.lookup(name).has_value()) return false;
        globalVars.insert(name,obj);
        return true;
    }
    std::vector<symbol> localVars;
    static inline symbol globalVars;
    int cur{-1};
};
#endif
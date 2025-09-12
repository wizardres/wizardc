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
    enum class objKind { variable, function };    
    virtual ~Obj()=default;
    virtual objKind getKind()const=0;
    virtual std::shared_ptr<Type> getType()const=0;
    virtual size_t getTypeSize()const=0;
};


class varObj : public Obj{
public:
    varObj()=default;
    ~varObj()=default;
    varObj(std::shared_ptr<Type> type,
           bool isglobal,
           int offset = 0):
        _offset(offset),
        _type(std::move(type)),
        _isglobal(isglobal) {}

    objKind getKind()const override { return objKind::variable; };
    std::shared_ptr<Type> getType()const override{ return _type; }
    size_t getTypeSize()const override { return _type->getSize(); }

    int getOffset() { return _offset; }
    bool isGlobal() { return _isglobal; }
private:
    int _offset;
    std::shared_ptr<Type> _type;
    bool _isglobal;
};


class funcObj : public Obj {
public:
    funcObj()=default;
    ~funcObj()=default;
    funcObj(std::shared_ptr<Type> type):_type(std::move(type)){}
    objKind getKind()const override { return objKind::function; };
    std::shared_ptr<Type> getType()const override{ return _type; }
    size_t getTypeSize()const override { return _type->getSize(); }

    //std::shared_ptr<Type> getretuenType() { }
private:
    std::shared_ptr<Type> _type;
};


class objFactor {
public:
    static std::shared_ptr<Obj> getlocalObj(int offset,std::shared_ptr<Type> type) {
        return std::make_shared<varObj>(type,false,offset);
    }
    static std::shared_ptr<Obj> getglobalObj(std::shared_ptr<Type> type) {
        return std::make_shared<varObj>(type,true);
    }
    static std::shared_ptr<Obj> getfuncObj(std::shared_ptr<Type> type) {
        return std::make_shared<funcObj>(type);
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

    bool insert_local(std::string_view name,const std::shared_ptr<Obj> obj) {
        if(lookup_curscope(name).has_value()) return false;
        localVars[cur].insert(name,obj);
        return true;
    }

    bool insert_global(std::string_view name,const std::shared_ptr<Obj> obj) {
        if(globalVars.lookup(name).has_value()) return false;
        globalVars.insert(name,obj);
        return true;
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
    std::vector<symbol> localVars;
    static inline symbol globalVars;
    int cur{-1};
};
#endif
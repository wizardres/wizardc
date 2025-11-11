#ifndef SYMBOL_H_
#define SYMBOL_H_

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include "type.h"

enum class SymbolType { S_var,S_array,S_func };
struct SymbolInfo{
    SymbolInfo(const token& tok,int offset,bool isglobal,const std::shared_ptr<Type> &type,SymbolType sty):
               _tok(tok),
               _offset(offset),
               _isglobal(isglobal),
               _type(type),
               _sTy(sty) {}
    token _tok;
    int _offset;
    bool _isglobal;
    std::shared_ptr<Type> _type;
    SymbolType _sTy;
};



class Scope {
public:
    Scope()=default;
    ~Scope()=default;
    void insert(const std::string& name,const SymbolInfo& info) {
        _symbols.emplace(name,info);
    }

    std::optional<SymbolInfo> lookup(const std::string& name) {
        if(auto it = _symbols.find(name); it != _symbols.end()) {
            return it->second;
        }
        return std::nullopt;
    }
private:
    std::unordered_map<std::string,SymbolInfo> _symbols;
};


class SymbolTable{
public:
    void enter(){ _locals.push_back({}); }
    void leave(){ _locals.pop_back(); }
    bool addSymbol(bool global,const std::string& name,const SymbolInfo& info) {
        if(!reDefined(global,name)) {
            if(global) _globals.insert(name,info);
            else _locals.back().insert(name,info);
            return true;
        }
        return false;
    }
    std::optional<SymbolInfo> lookup(const std::string& name) {
        for(auto p = _locals.rbegin(); p != _locals.rend(); ++p) {
            if(auto it = p->lookup(name); it.has_value()) {
                return it;
            }
        }
        auto p = _globals.lookup(name);
        return p;
    }
    static SymbolInfo newSymbol(const token& tok,int offset,bool isglobal,const std::shared_ptr<Type> &type,SymbolType sty) {
        return SymbolInfo(tok,offset,isglobal,type,sty);
    }
private:
    bool reDefined(bool global,const std::string& name) {
        if(global) return _globals.lookup(name).has_value();
        else return _locals[_locals.size()-1].lookup(name).has_value();
    }
    std::vector<Scope> _locals;
    Scope _globals;
};
#endif
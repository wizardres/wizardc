#ifndef SYMBOL_H_
#define SYMBOL_H_

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
class symbol {
public:
    symbol()=default;
    void insert(std::string& name,int offset) {
        sym.insert({name,offset});
    }
    std::optional<int> lookup(std::string& name) {
        auto it = sym.find(name);
        if(it != sym.end()) {
            return it->second;
        }
        return std::nullopt;
    }
private:
    std::unordered_map<std::string,int> sym;
};


class Scope{
public:
    Scope()=default;
    void enter(){
        cur++;
        syms.push_back(symbol{});
    }
    void leave(){
        syms.pop_back();
        cur--;
    }
    bool isglobal(){
        return cur == -1;
    }
    void insert(std::string& name,int offset) {
        syms[cur].insert(name,offset);
    }
    std::optional<int> allscope_lookup(std::string& name){
        for(int i = cur; i >= 0; i--){
            symbol& s = syms[i];
            auto it = s.lookup(name);
            if(it.has_value()){
                return it.value();
            }
        }
        return std::nullopt;
    }
    std::optional<int> curscope_lookup(std::string& name) {
        return syms[cur].lookup(name);
    }
private:
    std::vector<symbol> syms;
    int cur{-1};
};
#endif
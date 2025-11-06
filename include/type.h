#ifndef TYPE_H_
#define TYPE_H_

#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include "lexer.h"
class Type {
public:
    enum class Kind { T_int, T_char, T_ptr, T_func , T_array };
    Type(Kind kind):_kind(kind) {}
    virtual ~Type()=default;

    Kind getKind()const { return _kind; };
    virtual size_t getSize()const=0;
    virtual std::string typestr()const=0;

    static bool isPointer(std::shared_ptr<Type> type);
    static bool isInteger(std::shared_ptr<Type> type);
    static bool isArray(std::shared_ptr<Type> type);
    static bool arePtrCompatible(const std::shared_ptr<Type>& lhs,const std::shared_ptr<Type>& rhs); 
private:
    Kind _kind;
};


class baseType final : public Type {
public:
    baseType(int size,Kind kind):Type(kind),_size(size){}

    size_t getSize()const override { return _size; }
    std::string typestr()const override { 
        if(getKind() == Type::Kind::T_int) return "int"; 
        else return "char";
    }
private:
    int _size;
};


class pointerType final: public Type {
public:
    pointerType(std::shared_ptr<Type> base):Type(Type::Kind::T_ptr),_base(std::move(base)){}
    size_t getSize()const override { return sizeof(void *); }
    size_t baseTypeSize()const { return _base->getSize(); }
    std::string typestr()const override { return _base->typestr() + " *"; }
    
    std::shared_ptr<Type> getBaseType()const { return _base;}
private:
    std::shared_ptr<Type> _base;
};


class arrayType final : public Type{
public:
    arrayType(size_t len,std::shared_ptr<Type> type):
              Type(Type::Kind::T_array),
              _len(len),
              _elemTy(type){}
    ~arrayType()=default;

    std::shared_ptr<Type> elemTy()const { return _elemTy; }
    size_t elemSize()const { return _elemTy->getSize(); }
    size_t getSize()const override { return _len * _elemTy->getSize(); }
    size_t getlen() { return _len; }

    std::string typestr()const override { return std::format("{}[{}]",_elemTy->typestr(),_len); }
private:
    int _len;
    std::shared_ptr<Type> _elemTy;
};


class funcType final: public Type {
public:
    funcType(std::shared_ptr<Type> ret,std::vector<std::shared_ptr<Type>> &param):
        Type(Type::Kind::T_func),
        _retType(ret),
        _paramTypes(std::move(param)){}

    size_t getSize()const override { return 0; }
    std::shared_ptr<Type> getRetType() { return _retType; }
    const std::vector<std::shared_ptr<Type>> &getParamTypes()const { return _paramTypes; }

    std::string typestr()const override {
        std::string param_type_s{"("};
        for(size_t i = 0; i < _paramTypes.size();i++){
            param_type_s += _paramTypes[i]->typestr();
            if(i < _paramTypes.size()-1)
                param_type_s.push_back(',');
        }
        param_type_s += ")";
        return std::format("{} {}",_retType->typestr(),param_type_s);
    }
private:
    std::shared_ptr<Type> _retType;
    std::vector<std::shared_ptr<Type>> _paramTypes;
};



class typeFactor {
public:
    static std::shared_ptr<Type> getInt() {
        return std::make_shared<baseType>(8,Type::Kind::T_int);
    }
    static std::shared_ptr<Type> getChar() {
        return std::make_shared<baseType>(1,Type::Kind::T_char);
    }
    static std::shared_ptr<Type> getPointerType(std::shared_ptr<Type> base) {
        return std::make_shared<pointerType>(base);
    }

    static std::shared_ptr<Type> getArrayType(int len,std::shared_ptr<Type> datatype) {
        return std::make_shared<arrayType>(len,datatype);
    }

    static std::shared_ptr<Type> getFuncType(
            std::shared_ptr<Type> ret,
            std::vector<std::shared_ptr<Type>> &params) {
                return std::make_shared<funcType>(ret,params);
    }
};

class typeChecker {
public:
    static std::shared_ptr<Type> checkEqual(const std::shared_ptr<Type>& lhs,const std::shared_ptr<Type>& rhs);
    static std::shared_ptr<Type> checkBinaryOp( tokenType op, const std::shared_ptr<Type>& lhs, const std::shared_ptr<Type>& rhs);
private:
    static bool isPointer(const std::shared_ptr<Type>& type) {
        return type && type->getKind() == Type::Kind::T_ptr;
    }
    static bool isInteger(const std::shared_ptr<Type>& type) {
        return type && type->getKind() == Type::Kind::T_int;
    }
    static std::shared_ptr<Type> decayArrayToPointer(const std::shared_ptr<Type>& type);
};


class pointerTypeCheck {
public:
    static std::shared_ptr<Type> checkBinaryOp(
        tokenType op,
        const std::shared_ptr<Type>& lhs,
        const std::shared_ptr<Type>& rhs);
    
    static std::shared_ptr<Type> checkAddtion(
        const std::shared_ptr<Type>& lhs,
        const std::shared_ptr<Type>& rhs);

    static std::shared_ptr<Type> checkSubtraction(
        const std::shared_ptr<Type>& lhs,
        const std::shared_ptr<Type>& rhs);
};

class integerTypeCheck {
public:
    static std::shared_ptr<Type> checkBinaryOp(
        const std::shared_ptr<Type>& lhs,
        const std::shared_ptr<Type>& rhs){
            return lhs->getSize() > rhs->getSize() ? lhs : rhs;
        }
};
#endif
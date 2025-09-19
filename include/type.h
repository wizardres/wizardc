#ifndef TYPE_H_
#define TYPE_H_

#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include "lexer.h"
class Type {
public:
    enum class Kind { T_int,T_ptr,T_func , T_array };
    Type()=default;
    virtual ~Type()=default;

    virtual Kind getKind()const=0;
    virtual size_t getSize()const=0;
    virtual std::string typestr()const=0;

    static bool isPointer(const std::shared_ptr<Type> type){ return type->getKind() == Kind::T_ptr; }
    static bool isInteger(const std::shared_ptr<Type> type){ return type->getKind() == Kind::T_int; }
    static bool isArray(const std::shared_ptr<Type> type){ return type->getKind() == Kind::T_array; }
    static bool areCompatible(const std::shared_ptr<Type>& lhs,const std::shared_ptr<Type>& rhs) { return true;}
};


class baseType final : public Type {
public:
    baseType(int size,Kind kind):_size(size),_kind(kind){}

    Kind getKind()const override { return _kind; }
    size_t getSize()const override { return _size; }
    std::string typestr()const override { return "int"; }
private:
    int _size;
    Kind _kind;
};

class pointerType final: public Type {
public:
    pointerType(std::shared_ptr<Type> base):_base(std::move(base)){}
    Kind getKind()const override { return Kind::T_ptr; }
    size_t getSize()const override { return sizeof(void *); }
    std::string typestr()const override { return _base->typestr() + " *"; }
    
    std::shared_ptr<Type> getBaseType()const { return _base;}
private:
    std::shared_ptr<Type> _base;
};


class arrayType final : public Type{
public:
    arrayType(size_t len,std::shared_ptr<Type> type):
              _len(len),
              _elemTy(type){}
    ~arrayType()=default;

    std::shared_ptr<Type> elemTy()const { return _elemTy; }
    size_t elemSize()const { return _elemTy->getSize(); }
    size_t getSize()const override { return _len * _elemTy->getSize(); }
    size_t getlen() { return _len; }

    std::string typestr()const override { return std::format("{}[{}]",_elemTy->typestr(),_len); }
    Kind getKind()const override { return Kind::T_array; };
private:
    int _len;
    std::shared_ptr<Type> _elemTy;
};


class funcType final: public Type {
public:
    funcType(std::shared_ptr<Type> ret,std::vector<std::shared_ptr<Type>> &param):
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
    Kind getKind()const override { return Kind::T_func; }
private:
    std::shared_ptr<Type> _retType;
    std::vector<std::shared_ptr<Type>> _paramTypes;
};



class typeFactor {
public:
    static std::shared_ptr<Type> getInt() {
        static auto intType =  std::make_shared<baseType>(8,Type::Kind::T_int);
        return intType;
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
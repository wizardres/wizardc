#ifndef TYPE_H_
#define TYPE_H_

#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include "lexer.h"
class Type {
public:
    enum class Kind { T_int,T_ptr,T_func };
    Type()=default;
    virtual ~Type()=default;

    virtual Kind getType()const=0;
    virtual size_t getSize()const=0;

    static bool isPointer(const std::shared_ptr<Type> type){ return type->getType() == Kind::T_ptr; }
    static bool isInteger(const std::shared_ptr<Type> type){ return type->getType() == Kind::T_int; }
    static bool areCompatible(const std::shared_ptr<Type>& lhs,const std::shared_ptr<Type>& rhs) { return true;}
};


class baseType final : public Type {
public:
    baseType(int size,Kind kind):_size(size),_kind(kind){}

    Kind getType()const override { return _kind; }
    size_t getSize()const override { return _size; }
private:
    int _size;
    Kind _kind;
};

class pointerType final: public Type {
public:
    pointerType(std::shared_ptr<Type> base):_base(std::move(base)){}
    Kind getType()const override { return Kind::T_ptr; }
    size_t getSize()const override { return sizeof(void *); }
    
    std::shared_ptr<Type> getBaseType()const { return _base;}
private:
    std::shared_ptr<Type> _base;
};


class funcType final: public Type {
public:
    funcType(std::shared_ptr<Type> ret,std::vector<std::shared_ptr<Type>> &param):
        _retType(ret),
        _paramTypes(std::move(param)){}
    Kind getType()const override { return Kind::T_func; }
    size_t getSize()const override { return 0; }
    std::shared_ptr<Type> getRetType() { return _retType; }
    const std::vector<std::shared_ptr<Type>> &getParamTypes()const { return _paramTypes; }
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

    static std::shared_ptr<Type> getFuncType(
            std::shared_ptr<Type> ret,
            std::vector<std::shared_ptr<Type>> &params) {
                return std::make_shared<funcType>(ret,params);
    }
};

class typeChecker {
public:
    static bool checkEqual(const std::shared_ptr<Type>& type,Type::Kind kind) {
        return type->getType() == kind;
    }
    static bool checkEqual(const std::shared_ptr<Type>& lhs,const std::shared_ptr<Type>& rhs) {
        if(isPointer(lhs) && isPointer(rhs)){
            auto lbase = static_cast<pointerType*>(lhs.get())->getBaseType();
            auto rbase = static_cast<pointerType*>(rhs.get())->getBaseType();
            return checkEqual(lbase,rbase); 
        }
        else {
            lhs->getType() == rhs->getType();
        }
    }

    static std::shared_ptr<Type> checkBinaryOp(
        token_t op,
        const std::shared_ptr<Type>& lhs,
        const std::shared_ptr<Type>& rhs);
private:
    static bool isPointer(const std::shared_ptr<Type>& type) {
        return type && type->getType() == Type::Kind::T_ptr;
    }
    static bool isInteger(const std::shared_ptr<Type>& type) {
        return type && type->getType() == Type::Kind::T_int;
    }
};


class pointerTypeCheck {
public:
    static std::shared_ptr<Type> checkBinaryOp(
        token_t op,
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
#ifndef TYPE_H_
#define TYPE_H_

#include <iostream>
#include <vector>
#include <map>
#include <memory>
class Type {
public:
    enum class Kind { T_int,T_ptr,T_func };
    Type()=default;
    virtual ~Type()=default;

    virtual Kind getType()const=0;
    virtual size_t getSize()const=0;
};

class baseType final : public Type {
public:
    baseType(int size,Kind kind):_size(size),_kind(kind){}

    Kind getType()const override { return _kind; }
    size_t getSize()const override { return _size; }
private:
    int _size;
    Kind _kind;
    size_t _align;
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
        std::map<Type*,std::weak_ptr<Type>> basetypes;
        auto ptr = basetypes.find(base.get());
        if(ptr != basetypes.end()) {
            return ptr->second.lock();
        }
        std::shared_ptr<pointerType> ptrType = std::make_shared<pointerType>(base);
        basetypes.insert({base.get(),ptrType});
        return ptrType;
    }

    static std::shared_ptr<Type> getFuncType(
            std::shared_ptr<Type> ret,
            std::vector<std::shared_ptr<Type>> &params) {
                return std::make_shared<funcType>(ret,params);
    }
};
#endif
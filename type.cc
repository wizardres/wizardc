#include "include/type.h"

std::shared_ptr<Type> typeChecker::checkBinaryOp(
    tokenType op,
    const std::shared_ptr<Type>& lhs,
    const std::shared_ptr<Type>& rhs){
    
    if(op == tokenType::T_assign) return checkEqual(lhs,rhs);
    auto l = decayArrayToPointer(lhs);
    auto r = decayArrayToPointer(rhs);
    if(isPointer(l) || isPointer(r)) {
        return pointerTypeCheck::checkBinaryOp(op,l,r);
    }else {
        return integerTypeCheck::checkBinaryOp(l,r);
    }
}


std::shared_ptr<Type> pointerTypeCheck::checkBinaryOp(
        tokenType op,
        const std::shared_ptr<Type>& lhs,
        const std::shared_ptr<Type>& rhs){

        switch(op) {
            case tokenType::T_plus:
                return checkAddtion(lhs,rhs);
            case tokenType::T_minus:
                return checkSubtraction(lhs,rhs);
            case tokenType::T_star:
                throw std::format("invalid operand of '{}' and '{}' to '*'",lhs->typestr(),rhs->typestr());
            case tokenType::T_div:
                throw std::format("invalid operand of '{}' and '{}' to '/'",lhs->typestr(),rhs->typestr());
            default:{
                throw std::format("invalid operator");
            }
        }
}


std::shared_ptr<Type> pointerTypeCheck::checkAddtion(
    const std::shared_ptr<Type>& lhs,
    const std::shared_ptr<Type>& rhs) {

    bool leftPtr = Type::isPointer(lhs);
    bool leftInteger = Type::isInteger(lhs);
    bool rightPtr = Type::isPointer(rhs);
    bool rightInteger = Type::isInteger(rhs);

    if(leftPtr && rightInteger) return lhs;
    else if(leftInteger && rightPtr) return rhs; 
    else {
        throw std::format("invalid operand of '{}' and '{}' to '+'",lhs->typestr(),rhs->typestr());
    }
}


std::shared_ptr<Type> pointerTypeCheck::checkSubtraction(
    const std::shared_ptr<Type>& lhs,
    const std::shared_ptr<Type>& rhs) {

    bool leftPtr = Type::isPointer(lhs);
    bool rightPtr = Type::isPointer(rhs);
    bool rightInteger = Type::isInteger(rhs);

    if(leftPtr && rightInteger) return lhs;

    else if(leftPtr && rightPtr){
        if(Type::areCompatible(lhs,rhs))
            return typeFactor::getInt(); 
        else 
            throw std::format("incompatible pointer type");
    }
    throw std::format("invalid operand of '{}' and '{}' to '-'",lhs->typestr(),rhs->typestr());
}

std::shared_ptr<Type> typeChecker::decayArrayToPointer(const std::shared_ptr<Type>& type) {
    if(Type::isArray(type)) {
        return typeFactor::getPointerType(static_cast<arrayType*>(type.get())->elemTy());
    }
    return type;
}


std::shared_ptr<Type> typeChecker::checkEqual(const std::shared_ptr<Type>& lhs,const std::shared_ptr<Type>& rhs) {
    auto decay_r = decayArrayToPointer(rhs);

    if(!isPointer(lhs) || !isPointer(decay_r)){
        if(lhs->getKind() == decay_r->getKind()){
            return lhs;
        }
        throw std::format("'=' has different type at both side:'{}' at left and '{}' at right",lhs->typestr(),rhs->typestr());
    }else {
        std::shared_ptr<Type> l = lhs,r = decay_r;
        while(isPointer(l) && isPointer(r)){
            l = static_cast<pointerType*>(l.get())->getBaseType();
            r = static_cast<pointerType*>(r.get())->getBaseType();
        }
        if(l->getKind() == r->getKind()){
            return lhs;
        }
        throw std::format("'=' has different type at both side:'{}' at left and '{}' at right",lhs->typestr(),rhs->typestr());
    }
}
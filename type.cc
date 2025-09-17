#include "include/type.h"

std::shared_ptr<Type> typeChecker::checkBinaryOp(
    tokenType op,
    const std::shared_ptr<Type>& lhs,
    const std::shared_ptr<Type>& rhs){

        auto l = decayArrayToPointer(lhs);
        auto r = decayArrayToPointer(lhs);
    if(isPointer(lhs) || isPointer(rhs)) {
        return pointerTypeCheck::checkBinaryOp(op,lhs,rhs);
    }else {
        return integerTypeCheck::checkBinaryOp(lhs,rhs);
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
                return checkAddtion(lhs,rhs);
            case tokenType::T_star:
                throw "invalid operand of 'int *' type to '*'";
            case tokenType::T_div:
                throw "invalid operand of 'int *' type to '/'";
            default:
            throw "two invalid operands";
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
        throw "invalid operands to '+' (two operands have 'int *' type)";
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
            throw "incompatible pointer type";
    }
    else throw "invalid operands to '-' (operands have 'int' and 'int *')"; 
}

std::shared_ptr<Type> typeChecker::decayArrayToPointer(const std::shared_ptr<Type>& type) {
    if(Type::isArray(type)) {
        return typeFactor::getPointerType(static_cast<arrayType*>(type.get())->elemTy());
    }
    return type;
}


bool typeChecker::checkEqual(const std::shared_ptr<Type>& lhs,const std::shared_ptr<Type>& rhs) {
    auto decay_r = decayArrayToPointer(rhs);
    if(isPointer(lhs) && isPointer(decay_r)){
        auto lbase = static_cast<pointerType*>(lhs.get())->getBaseType();
        auto rbase = static_cast<pointerType*>(decay_r.get())->getBaseType();
        return checkEqual(lbase,rbase); 
    }
    else {
        return lhs->getKind() == decay_r->getKind();
    }
}
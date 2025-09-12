#include "include/type.h"

std::shared_ptr<Type> typeChecker::checkBinaryOp(
    token_t op,
    const std::shared_ptr<Type>& lhs,
    const std::shared_ptr<Type>& rhs){

    if(isPointer(lhs) || isPointer(rhs)) {
        return pointerTypeCheck::checkBinaryOp(op,lhs,rhs);
    }else {
        return integerTypeCheck::checkBinaryOp(lhs,rhs);
    }
}

std::shared_ptr<Type> pointerTypeCheck::checkBinaryOp(
        token_t op,
        const std::shared_ptr<Type>& lhs,
        const std::shared_ptr<Type>& rhs){

        switch(op) {
            case token_t::T_plus:
                return checkAddtion(lhs,rhs);
            case token_t::T_minus:
                return checkAddtion(lhs,rhs);
            case token_t::T_star:
                throw "invalid operand of 'int *' type to '*'";
            case token_t::T_div:
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
    bool leftInteger = Type::isInteger(lhs);
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

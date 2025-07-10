#ifndef CODEGENERATOR_H_
#define CODEGENERATOR_H_

#include "visitor.h"
#include "parse.h"

class codegenerator final: public visitor {
public:
    codegenerator();
    virtual ~codegenerator();

    void visit(numericExpr&)override;
    void visit(identifierExpr&)override;
    void visit(prefixExpr&)override;
    void visit(binaryExpr&)override;
    void visit(ifStmt&)override;
    void visit(exprStmt&)override;
    void visit(blockStmt&)override;
};
#endif
#ifndef VISITOR_H_
#define VISITOR_H_

class numericExpr;
class identifierExpr;
class prefixExpr;
class binaryExpr;
class ifStmt;
class exprStmt;
class blockStmt;

class visitor {
public:
    visitor()=default;
    virtual ~visitor()=default;

    virtual void visit(numericExpr&)=0;
    virtual void visit(identifierExpr&)=0;
    virtual void visit(prefixExpr&)=0;
    virtual void visit(binaryExpr&)=0;
    virtual void visit(ifStmt&)=0;
    virtual void visit(exprStmt&)=0;
    virtual void visit(blockStmt&)=0;
};

#endif
#ifndef VISITOR_H_
#define VISITOR_H_

class numericExpr;
class identExpr;
class prefixExpr;
class binaryExpr;
class ifStmt;
class funcallExpr;
class exprStmt;
class blockStmt;
class declStmt;

class visitor {
public:
    visitor()=default;
    virtual ~visitor()=default;

    virtual void visit(numericExpr&)=0;
    virtual void visit(identExpr&)=0;
    virtual void visit(prefixExpr&)=0;
    virtual void visit(binaryExpr&)=0;
    virtual void visit(funcallExpr&)=0;
    virtual void visit(ifStmt&)=0;
    virtual void visit(exprStmt&)=0;
    virtual void visit(blockStmt&)=0;
    virtual void visit(declStmt&)=0;
};

#endif
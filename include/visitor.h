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
class retStmt;
class vardef;
class funcdef;
class Prog;

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
    virtual void visit(retStmt&)=0;
    virtual void visit(vardef&)=0;
    virtual void visit(funcdef&)=0;
    virtual void visit(Prog&)=0;
};

#endif
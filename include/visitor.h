#ifndef VISITOR_H_
#define VISITOR_H_

class numericNode;
class stringNode;
class identNode;
class prefixNode;
class binaryNode;
class funcallNode;
class arrayVisit;
class arraydef;
class ifStmt;
class exprStmt;
class whileStmt;
class forStmt;
class blockStmt;
class retStmt;
class vardef;
class funcdef;
class Prog;

class visitor {
public:
    visitor()=default;
    virtual ~visitor()=default;

    virtual void visit(numericNode&)=0;
    virtual void visit(stringNode&)=0;
    virtual void visit(identNode&)=0;
    virtual void visit(prefixNode&)=0;
    virtual void visit(binaryNode&)=0;
    virtual void visit(funcallNode&)=0;
    virtual void visit(arrayVisit&)=0;
    virtual void visit(arraydef&)=0;
    virtual void visit(ifStmt&)=0;
    virtual void visit(exprStmt&)=0;
    virtual void visit(blockStmt&)=0;
    virtual void visit(forStmt&)=0;
    virtual void visit(whileStmt&)=0;
    virtual void visit(retStmt&)=0;
    virtual void visit(vardef&)=0;
    virtual void visit(funcdef&)=0;
    virtual void visit(Prog&)=0;
};

#endif
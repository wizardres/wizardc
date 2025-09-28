#ifndef CODEGENERATOR_H_
#define CODEGENERATOR_H_

#include "visitor.h"
#include "parse.h"

class codegenerator final: public visitor {
public:
    codegenerator(){}
    virtual ~codegenerator(){}

    void visit(numericNode&)override;
    void visit(identNode&)override;
    void visit(prefixNode&)override;
    void visit(binaryNode&)override;
    void visit(funcallNode&)override;
    void visit(arrayVisit&)override;
    void visit(arraydef&)override;
    void visit(ifStmt&)override;
    void visit(exprStmt&)override;
    void visit(blockStmt&)override;
    void visit(retStmt&)override;
    void visit(vardef&)override;
    void visit(funcdef&)override;
    void visit(Prog&)override;

    void push(std::string_view reg);
    void pop(std::string_view reg);
    void gen_addr(Node& ident);
    void store(const Node& node);
    void load(const Node& node);
};
#endif
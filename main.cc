#include <iostream>
#include <string>
#include "include/codegenerator.h"
#include "include/lexer.h"

int main(int argc,char *argv[]) {
    std::unique_ptr<Stmt> stmt = parse(argv[1]);
    codegenerator gen;
    stmt->accept(gen);
    return 0;
}

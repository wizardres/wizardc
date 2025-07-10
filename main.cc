#include <iostream>
#include <string>
#include "include/codegenerator.h"

extern const char *src;

int main(int argc,char *argv[]) {
    src = argv[1];
    std::unique_ptr<Stmt> stmt = parse();
    codegenerator gen;
    stmt->accept(gen);
    return 0;
}

#include <iostream>
#include <string>
#include "include/parse.h"
#include "include/codegen.h"

using namespace std;
extern const char *src;

int main(int argc,char *argv[]) {
    src = argv[1];
    std::unique_ptr<Stmt> stmt = parse();
    codegen(stmt);
    return 0;
}

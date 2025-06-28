#include <iostream>
#include <string>
#include "include/parse.h"
#include "include/codegen.h"

using namespace std;
extern const char *src;

int main(int argc,char *argv[]) {
    src = argv[1];
    std::unique_ptr<Expr> e = parse();
    codegen(e);
    return 0;
}

#include <iostream>
#include <string>
#include "include/parse.h"

using namespace std;
extern const char *src;

int main(int argc,char *argv[]) {
    src = argv[1];
    cout << "input:" << src << "\n";
    std::unique_ptr<Expr> e = parse();
    cout << "result:" << e->eval() << "\n";
    return 0;
}

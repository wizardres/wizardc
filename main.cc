#include <iostream>
#include <string>
#include "include/codegenerator.h"
#include "include/lexer.h"

int main(int argc,char *argv[]) {
    if(argc < 2){
        std::cerr << "usage:./wizardc [input]";
        exit(-1);
    }
    Parser parser(argv[1]);
    Prog prog = parser.start();
    codegenerator gen;
    prog.accept(gen);
    return 0;
}

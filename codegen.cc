#include "include/codegen.h"

void numericExpr::codegen() {
    std::cout << std::format("  mov ${},%rax\n",value);
}

void prefixExpr::codegen() {
    e->codegen();
    std::cout << std::format("  neg %rax\n");
}

void binaryExpr::codegen() {
    rhs->codegen();
    push();
    lhs->codegen();
    pop("%rdi");
    switch(op) {
        case token_t::T_plus: {
            std::cout << std::format("  add %rdi,%rax\n");break;
        }
        case token_t::T_minus: {
            std::cout << std::format("  sub %rdi,%rax\n");break;
        }
        case token_t::T_star: {
            std::cout << std::format("  imul %rdi,%rax\n");break;
        }
        case token_t::T_div: {
            std::cout << std::format("  cqo\n  idiv %rdi\n");
        }
        case token_t::T_lt:
        case token_t::T_le:
        case token_t::T_gt:
        case token_t::T_ge:
        case token_t::T_eq:
        case token_t::T_neq:
         std::cout << std::format("  cmp %rdi,%rax\n");
         if(op == token_t::T_lt)
             std::cout << std::format("  setl %al\n");
         else if(op == token_t::T_le)
             std::cout << std::format("  setle %al\n");
         else if(op == token_t::T_gt)
             std::cout << std::format("  setg %al\n");
         else if(op == token_t::T_ge)
             std::cout << std::format("  setge %al\n");
         else if(op == token_t::T_eq)
             std::cout << std::format("  sete %al\n");
         else if(op == token_t::T_neq)
             std::cout << std::format("  setne %al\n");
         std::cout << std::format("  movzb %al,%rax\n");
         return;
    }
}


void codegen(const std::unique_ptr<Expr>& e) {
    std::cout << std::format("   .global main\nmain:\n");
    e->codegen();
    std::cout << std::format("  ret\n");
}
#include "include/codegenerator.h"


codegenerator::codegenerator() {
    std::string s {"   .global main\nmain:\n"};
    s += std::format("  push %rbp\n  mov %rsp,%rbp\n  sub ${},%rsp\n",-identifierExpr::var_offset);
    std::cout << s;
}

codegenerator::~codegenerator() {
    std::cout <<  "  mov %rbp,%rsp\n  pop %rbp\n  ret\n";
}

void codegenerator::visit(numericExpr& E) {
    std::cout << std::format("  mov ${},%rax\n",E.value);
}

void codegenerator::visit(identifierExpr& E) {
    std::cout << std::format("  mov {}(%rbp),%rax\n",E.offset);
}

void codegenerator::visit(prefixExpr& E) {
    E.e->accept(*this);
    std::cout << std::format("  neg %rax\n");
}

void codegenerator::visit(binaryExpr& E) {
    token_t op = E.op;
    if(op == token_t::T_assign) {
        std::cout << std::format("  lea {}(%rbp),%rax\n",static_cast<identifierExpr*>(E.lhs.get())->offset);
        E.push();
        E.rhs->accept(*this);
        E.pop("%rdi");
        std::cout << "  mov %rax,(%rdi)\n";
        return;
    }
    E.rhs->accept(*this);
    E.push();
    E.lhs->accept(*this);
    E.pop("%rdi");
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

void codegenerator::visit(ifStmt& S) {
    int l = S.level++;
    S.cond->accept(*this);
    std::string out;
    std::cout << "  cmp $0,%rax\n";
    std::string label = S.elseStmt == nullptr ? std::format(".L.end.{}",l) : std::format(".L.else.{}",l);
    std::cout << std::format("  je {}\n",label);
    S.then->accept(*this);
    if(S.elseStmt != nullptr) {
        std::cout << std::format("  jmp .L.end.{}\n",l);
        std::cout << std::format(".L.else.{}:\n",l);
        S.elseStmt->accept(*this);
    }
    std::cout << std::format(".L.end.{}:\n",l);
}

void codegenerator::visit(exprStmt& S) {
    S.e->accept(*this);
}

void codegenerator::visit(blockStmt& S) {
    for(auto& s:S.stmts) {
        s->accept(*this);
    }
}


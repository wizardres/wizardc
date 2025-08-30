#include "include/codegenerator.h"



void codegenerator::push(std::string_view reg) {
    std::cout << "  push %" << reg << "\n";
}
void codegenerator::pop(std::string_view reg) {
    std::cout << "  pop %" << reg << "\n";
}

void codegenerator::visit(numericExpr& E) {
    std::cout << std::format("  mov ${},%rax\n",E.value);
}

void codegenerator::visit(identExpr& E) {
    std::cout << std::format("  mov {}(%rbp),%rax\n",E.offset);
}

void codegenerator::visit(prefixExpr& E) {
    E.e->accept(*this);
    std::cout << std::format("  neg %rax\n");
}

void codegenerator::visit(funcallExpr& E) {
    std::array<const char *,6> regs{ "%rdi","%rsi","%rdx","%rcx","%r8","%r9" };
    int nargs = 0;
    for(auto &arg : E.args) {
        arg->accept(*this);
        std::cout << "  push %rax\n";
        nargs++;
    }
    for(int i = nargs-1; i >= 0; i--) {
        std::cout << std::format("  pop {}\n",regs[i]);
    }
    std::cout << std::format("  call {}\n",E.funcname);
}

void codegenerator::visit(binaryExpr& E) {
    token_t op = E.tok.type;
    if(op == token_t::T_assign) {
        if(E.rhs != nullptr) {
            std::cout << std::format("  lea {}(%rbp),%rax\n",static_cast<identExpr*>(E.lhs.get())->offset);
            push("rax");
            E.rhs->accept(*this);
            pop("rdi");
            std::cout << "  mov %rax,(%rdi)\n";
        }
        return;
    }
    E.rhs->accept(*this);
    push("rax");
    E.lhs->accept(*this);
    pop("rdi");
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
            std::cout << std::format("  cqo\n  idiv %rdi\n");break;
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
        default:{
            std::cerr << "invalid arithmetic operator:::" << E.tok.str << "\n";
            exit(-1);
        }
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


void codegenerator::visit(retStmt& S) {
    S.e->accept(*this);
    std::cout << std::format("  jmp .L.{}.ret\n",retStmt::fname);
}

void codegenerator::visit(exprStmt& S) {
    S.e->accept(*this);
}

void codegenerator::visit(vardef& decl) {
    for(auto& equ : decl.decls) {
        equ->accept(*this);
    }
}

void codegenerator::visit(funcdef& f) {
    std::cout << std::format("  .global {}\n{}:\n",f.name,f.name);
    std::cout << std::format("  push %rbp\n  mov %rsp,%rbp\n  sub ${},%rsp\n",f.stackoff);
    retStmt::fname = f.name;
    std::array<const char *,6> regs{ "%rdi","%rsi","%rdx","%rcx","%r8","%r9" };
    for(size_t i = 0; i < f.params.size(); i++) {
        std::cout << std::format("  mov {},{}(%rbp)\n",regs[i],static_cast<identExpr*>(f.params[i].get())->offset);
    }
    f.body->accept(*this);
    std::cout <<  std::format(".L.{}.ret:\n  mov %rbp,%rsp\n  pop %rbp\n  ret\n",f.name);
}

void codegenerator::visit(blockStmt& S) {
    for(auto& s:S.stmts) {
        s->accept(*this);
    }
}

void codegenerator::visit(Prog& p) {
    for(auto &stmt : p.stmts) {
        stmt->accept(*this);
    }
}


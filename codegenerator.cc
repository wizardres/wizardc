#include "include/codegenerator.h"



void codegenerator::push(std::string_view reg) {
    std::cout << "  push %" << reg << "\n";
}
void codegenerator::pop(std::string_view reg) {
    std::cout << "  pop %" << reg << "\n";
}

void codegenerator::visit(numericExpr& E) {
    std::cout << std::format("  mov ${},%rax\n",E.Value());
}

void codegenerator::visit(identExpr& E) {
    if(E.isGlobal()) {
        std::cout << std::format("  lea {}(%rip),%rax\n  mov (%rax),%rax\n",E.getName());
    }else{
        std::cout << std::format("  mov {}(%rbp),%rax\n",E.getOffset());
    }
}

void codegenerator::visit(prefixExpr& E) {
    Expr::Kind kind = E.getKind();
    if(kind == Expr::Kind::N_addr) {
        auto ident = static_cast<identExpr*>(E.getExpr().get());
        std::cout << std::format("  lea {}(%rbp),%rax\n",ident->getOffset());
    }else if(kind == Expr::Kind::N_deref) {
        E.getExpr()->accept(*this);
        std::cout << "  mov (%rax),%rax\n";
    }else {
        E.getExpr()->accept(*this);
        std::cout << std::format("  neg %rax\n");
    }
}

void codegenerator::visit(funcallExpr& E) {
    std::array<const char *,6> regs{ "%rdi","%rsi","%rdx","%rcx","%r8","%r9" };
    int nargs = 0;
    auto &args = E.getArgs();
    for(auto &arg : args) {
        arg->accept(*this);
        std::cout << "  push %rax\n";
        nargs++;
    }
    for(int i = nargs-1; i >= 0; i--) {
        std::cout << std::format("  pop {}\n",regs[i]);
    }
    std::cout << std::format("  call {}\n",E.getName());
}

void codegenerator::gen_addr(const std::unique_ptr<Expr>& expr) {
    if(expr->getKind() == Expr::Kind::N_identifier) {
        auto ident = static_cast<identExpr*>(expr.get());
        if(ident->isGlobal())  
            std::cout << std::format("  lea {}(%rip),%rax\n",ident->getName());
        else
            std::cout << std::format("  lea {}(%rbp),%rax\n",ident->getOffset());
    }
    else {
        auto prefix = static_cast<prefixExpr*>(expr.get());
        prefix->accept(*this);
    }
}


void codegenerator::visit(binaryExpr& E) {
    token_t op = E.getOp();
    auto &lhs = E.getLhs();
    auto &rhs = E.getRhs();
    if(op == token_t::T_assign) {
        if(rhs != nullptr) {
            gen_addr(lhs);
            push("rax");
            rhs->accept(*this);
            pop("rdi");
            std::cout << "  mov %rax,(%rdi)\n";
        }
        return;
    }
    rhs->accept(*this);
    push("rax");
    lhs->accept(*this);
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
        default: return;
    }
}

void codegenerator::visit(ifStmt& S) {
    int l = S.levelUp();
    auto &cond = S.getCond();
    auto &then = S.getThen();
    auto &elseStmt = S.getElse();
    cond->accept(*this);
    std::cout << "  cmp $0,%rax\n";
    std::string label = elseStmt == nullptr ? std::format(".L.end.{}",l) : std::format(".L.else.{}",l);
    std::cout << std::format("  je {}\n",label);
    then->accept(*this);
    if(elseStmt != nullptr) {
        std::cout << std::format("  jmp .L.end.{}\n",l);
        std::cout << std::format(".L.else.{}:\n",l);
        elseStmt->accept(*this);
    }
    std::cout << std::format(".L.end.{}:\n",l);
}


void codegenerator::visit(retStmt& S) {
    S.getStmt()->accept(*this);
    std::cout << std::format("  jmp .L.{}.ret\n",retStmt::getName());
}

void codegenerator::visit(exprStmt& S) {
    S.getExpr()->accept(*this);
}

void codegenerator::visit(vardef& vars) {
    auto &decls = vars.getDeclas();
    if(vars.isGlobal()) {
        for(auto &var : decls) {
            auto ident = static_cast<identExpr*>(var.get());
            std::cout << std::format("  .globl {}\n  .data\n",ident->getName());
            std::cout << std::format("{}:\n  .zero {}\n",ident->getName(),ident->typeSize());
        }
    }
    else{
        for(auto &var : decls) {
            var->accept(*this);
        }
    }
}

void codegenerator::visit(funcdef& f) {
    auto name = f.getName();
    int stackoff = f.getStackOff();
    auto &params = f.getParams();
    auto &body = f.getBody();

    std::cout << std::format("  .globl {}\n  .text\n{}:\n",name,name);
    std::cout << std::format("  push %rbp\n  mov %rsp,%rbp\n  sub ${},%rsp\n",stackoff);
    retStmt::setFuncName(name);
    std::array<const char *,6> regs{ "%rdi","%rsi","%rdx","%rcx","%r8","%r9" };
    for(size_t i = 0; i < params.size(); i++) {
        std::cout << std::format("  mov {},{}(%rbp)\n",regs[i],static_cast<identExpr*>(params[i].get())->getOffset());
    }
    body->accept(*this);
    std::cout <<  std::format(".L.{}.ret:\n  mov %rbp,%rsp\n  pop %rbp\n  ret\n",name);
}

void codegenerator::visit(blockStmt& S) {
    auto &stmts = S.getStmts();
    for(auto& s: stmts) {
        s->accept(*this);
    }
}

void codegenerator::visit(Prog& p) {
    for(auto &stmt : p._stmts) {
        stmt->accept(*this);
    }
}


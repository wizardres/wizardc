#include "include/codegenerator.h"


int level() {
    static int level = 0;
    return level++;
}

void codegenerator::push(std::string_view reg) {
    std::cout << "  push %" << reg << "\n";
}

void codegenerator::pop(std::string_view reg) {
    std::cout << "  pop %" << reg << "\n";
}

void gen_offset(visitor& vis,arrayVisit &v) {
    size_t size = v.typeSize();
    std::cout << std::format("  mov ${},%rax\n  push %rax\n",size);
    auto idx = v.get_idx();
    idx->accept(vis);
    std::cout << "  pop %rdi\n  imul %rdi,%rax\n  push %rax\n";
}

void codegenerator::visit(numericNode& node) {
    std::cout << std::format("  mov ${},%rax\n",node.Value());
}

void codegenerator::visit(identNode& node) {
    gen_addr(node);
    load(node);
}

void codegenerator::visit(stringNode& node) {
    gen_addr(node);
}


void codegenerator::visit(prefixNode& node) {
    if(node.equal(Node::Kind::N_addr)) {
        gen_addr(*node.getNode());
    }else if(node.equal(Node::Kind::N_deref)) {
        node.getNode()->accept(*this);
        std::cout << "  mov (%rax),%rax\n";
    }else {
        node.getNode()->accept(*this);
        std::cout << std::format("  neg %rax\n");
    }
}


void codegenerator::visit(funcallNode& node) {
    std::array<const char *,6> regs{ "%rdi","%rsi","%rdx","%rcx","%r8","%r9" };
    int nargs = 0;
    auto &args = node.getArgs();
    for(auto &arg : args) {
        arg->accept(*this);
        std::cout << "  push %rax\n";
        nargs++;
    }
    for(int i = nargs-1; i >= 0; i--) {
        std::cout << std::format("  pop {}\n",regs[i]);
    }
    std::cout << std::format("  call {}\n",node.getName());
}


void codegenerator::gen_addr(Node& node) {
    if(node.equal(Node::Kind::N_identifier)) {
        auto ident = dynamic_cast<identNode&>(node);
        if(ident.isGlobal())  
            std::cout << std::format("  lea {}(%rip),%rax\n",ident.getName());
        else
            std::cout << std::format("  lea {}(%rbp),%rax\n",ident.getOffset());
    }
    else if(node.equal(Node::Kind::N_arrayvisit)) {
        auto arr = dynamic_cast<arrayVisit&>(node);
        gen_offset(*this,arr);
        if(arr.isGlobal()) {
            std::cout << std::format("  lea {}(%rip),%rax\n",arr.getName());
        }else {
            std::cout << std::format("  lea {}(%rbp),%rax\n",arr.getOffset());
        }
        std::cout << "  pop %rdi\n  add %rdi,%rax\n";
    }
    else if(node.equal(Node::Kind::N_deref)){
        auto prefix = dynamic_cast<prefixNode&>(node);
        prefix.getNode()->accept(*this);
    }
    else if(node.equal(Node::Kind::N_string)) {
        auto str = dynamic_cast<stringNode&>(node);
        std::cout << std::format("  lea .str.{}(%rip),%rax\n",str.get_label());
    }else {
        exit(-1);
    }
}


void codegenerator::store(const Node &node) {
    pop("rdi");
    if(node.getType()->getSize() == 1) {
        std::cout << "  mov %al,(%rdi)\n";
    }else {
        std::cout << "  mov %rax,(%rdi)\n";
    }
}

void codegenerator::load(const Node& node) {
    if(!Type::isArray(node.getType())){
        if(node.getType()->getSize() == 1)
            std::cout << "  movsbq (%rax),%rax\n";
        else 
            std::cout << "  mov (%rax),%rax\n";
    }
}


void codegenerator::visit(binaryNode& node) {
    tokenType op = node.getOp();
    auto lhs = node.getLhs();
    auto rhs = node.getRhs();
    if(op == tokenType::T_assign) {
        if(rhs != nullptr) {
            gen_addr(*lhs);
            push("rax");
            rhs->accept(*this);
            store(*lhs);
        }
        return;
    }
    rhs->accept(*this);
    push("rax");
    lhs->accept(*this);
    pop("rdi");
    switch(op) {
        case tokenType::T_plus: {
            std::cout << std::format("  add %rdi,%rax\n");break;
        }
        case tokenType::T_minus: {
            std::cout << std::format("  sub %rdi,%rax\n");break;
        }
        case tokenType::T_star: {
            std::cout << std::format("  imul %rdi,%rax\n");break;
        }
        case tokenType::T_div: {
            std::cout << std::format("  cqo\n  idiv %rdi\n");break;
        }
        case tokenType::T_lt:
        case tokenType::T_le:
        case tokenType::T_gt:
        case tokenType::T_ge:
        case tokenType::T_eq:
        case tokenType::T_neq:
         std::cout << std::format("  cmp %rdi,%rax\n");
         if(op == tokenType::T_lt)
             std::cout << std::format("  setl %al\n");
         else if(op == tokenType::T_le)
             std::cout << std::format("  setle %al\n");
         else if(op == tokenType::T_gt)
             std::cout << std::format("  setg %al\n");
         else if(op == tokenType::T_ge)
             std::cout << std::format("  setge %al\n");
         else if(op == tokenType::T_eq)
             std::cout << std::format("  sete %al\n");
         else if(op == tokenType::T_neq)
             std::cout << std::format("  setne %al\n");
         std::cout << std::format("  movzb %al,%rax\n");
        default: return;
    }
}

void codegenerator::visit(whileStmt& S) {
    std::string label = std::format(".while.{}", level());
    std::string end_label = ".while.end";
    std::cout << std::format("{}:\n",label);
    S.compileCond(*this);
    std::cout << std::format("  cmp $0,%rax\n  je {}\n",end_label);
    S.compileBody(*this);
    std::cout << std::format("  jmp {}\n{}:\n",label,end_label);
}

void codegenerator::visit(forStmt &S) {
    S.compileInit(*this);
    std::string label = std::format(".for.{}",level());
    std::string end_label = ".for.end";
    std::cout << std::format("{}:\n",label);
    if(S.compileCond(*this))
        std::cout << std::format("  cmp $0,%rax\n  je {}\n",end_label);
    S.compileBody(*this);
    S.compileInc(*this);
    std::cout << std::format("  jmp {}\n{}:\n",label,end_label);
}


void codegenerator::visit(ifStmt& S) {
    int l = level();
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
    S.compileStmt(*this);
    std::cout << std::format("  jmp .L.{}.ret\n",retStmt::getName());
}

void codegenerator::visit(exprStmt& S) {
    S.getNode()->accept(*this);
}


void codegenerator::visit(arraydef& def) {
    int offset = def.getOffset();
    int size = def.elemSize();
    const auto &init_lst = def.get_init_lst();
    for(const auto& init : init_lst) {
        init->accept(*this);
        std::cout << std::format("  mov %rax,{}(%rbp)\n",offset);
        offset += size;
    }
}


void codegenerator::visit(arrayVisit& v) {
    if(v.isArray()) {
        gen_addr(v);
        load(v);
    }
    else {
        gen_offset(*this,v);
        if(v.isGlobal()) {
            std::cout << std::format("  lea {}(%rip),%rax\n",v.getName());
        }else {
            std::cout << std::format("  lea {}(%rbp),%rax\n",v.getOffset());
        }
        std::cout << "  mov (%rax),%rax\n  pop %rdi\n  add %rdi,%rax\n";
        if(v.getType()->getSize() == 1)
            std::cout << "  movsbq (%rax),%rax\n";
        else 
            std::cout << "  mov (%rax),%rax\n";
    }
}


void codegenerator::visit(vardef& vars) {
    auto &decls = vars.getDeclas();
    if(vars.isGlobal()) {
        for(auto &var : decls) {
            if(var->equal(Node::Kind::N_string)) {
                auto str = dynamic_cast<stringNode*>(var.get());
                std::cout << std::format("  .globl .str.{}\n  .data\n.str.{}:\n",str->get_label(),str->get_label());
                std::cout << std::format("  .string \"{}\"\n",str->strView());
            } else if(var->equal(Node::Kind::N_identifier) || var->equal(Node::Kind::N_arraydef)) {
                std::cout << std::format("  .globl {}\n  .data\n{}:\n",var->strView(),var->strView());
                std::cout << std::format("  .zero {}\n",var->typeSize());
            } else {
                return;
            }
        }
    }
    else{
        for(auto &var : decls) {
            if(var->equal(Node::Kind::N_binary) || var->equal(Node::Kind::N_arraydef)) {
                var->accept(*this);
            }
        }
    }
}


void codegenerator::visit(funcdef& f) {
    auto name = f.getName();
    int stackoff = f.getStackOff();
    auto &params = f.getParams();
    auto &body = f.getBody();

    retStmt::setFuncName(name);
    std::cout << std::format("  .globl {}\n  .text\n{}:\n",name,name);
    std::cout << std::format("  push %rbp\n  mov %rsp,%rbp\n  sub ${},%rsp\n",stackoff);
    std::array<const char *,6> regs{ "%rdi","%rsi","%rdx","%rcx","%r8","%r9" };
    for(size_t i = 0; i < params.size(); i++) {
        std::cout << std::format("  mov {},{}(%rbp)\n",regs[i],static_cast<identNode*>(params[i].get())->getOffset());
    }
    body->accept(*this);
    std::cout <<  std::format(".L.{}.ret:\n  mov %rbp,%rsp\n  pop %rbp\n  ret\n",name);
}


void codegenerator::visit(blockStmt& S) {
    S.compileStmts(*this);
}

void codegenerator::visit(Prog& p) {
    for(auto &stmt : p._stmts) {
        stmt->accept(*this);
    }
}


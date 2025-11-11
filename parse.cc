#include "include/parse.h"

std::vector<std::shared_ptr<Stmt>> global_def;
#ifdef DEBUG
std::map<tokenType,std::string> tokenstrs {
    {tokenType::T_num,"T_num"},
    {tokenType::T_string,"T_string"},
    {tokenType::T_identifier,"T_identifier"},
    {tokenType::T_keyword,"T_keyword"},
    {tokenType::T_plus,"T_plus"},
    {tokenType::T_minus,"T_minus"},
    {tokenType::T_star,"T_star"},
    {tokenType::T_div,"T_div"},
    {tokenType::T_open_paren,"T_open_paren"},
    {tokenType::T_close_paren,"T_close_paren"},
    {tokenType::T_open_square,"T_open_square"},
    {tokenType::T_close_square,"T_close_square"},
    {tokenType::T_open_block,"T_open_block"},
    {tokenType::T_close_block,"T_close_block"},
    {tokenType::T_semicolon,"T_semicolon"},
    {tokenType::T_lt,"T_lt"},
    {tokenType::T_gt,"T_gt"},
    {tokenType::T_not,"T_not"},
    {tokenType::T_assign,"T_assign"},
    {tokenType::T_eq,"T_eq"},
    {tokenType::T_le,"T_le"},
    {tokenType::T_ge,"T_ge"},
    {tokenType::T_neq,"T_neq"},
    {tokenType::T_comma,"T_comma"},
    {tokenType::T_period,"T_period"},
    {tokenType::T_if,"T_if"},
    {tokenType::T_else,"T_else"},
    {tokenType::T_return,"T_return"},
    {tokenType::T_int,"T_int"},
    {tokenType::T_eof,"T_eof"},
    {tokenType::T_addr,"T_addr"},
    {tokenType::T_char,"T_char"},
};
#endif

std::map<tokenType,precType> precedence {
    { tokenType::T_num,precType::P_none },
    { tokenType::T_eof,precType::P_none },
    { tokenType::T_identifier,precType::P_none },
    { tokenType::T_bit_and,precType::P_bit },
    { tokenType::T_plus,precType::P_factor },
    { tokenType::T_minus,precType::P_factor },
    { tokenType::T_star,precType::P_term },
    { tokenType::T_div,precType::P_term },
    { tokenType::T_assign,precType::P_assign },
    { tokenType::T_lt,precType::P_comparison },
    { tokenType::T_le,precType::P_comparison },
    { tokenType::T_gt,precType::P_comparison },
    { tokenType::T_ge,precType::P_comparison },
    { tokenType::T_neq,precType::P_comparison },
    { tokenType::T_eq,precType::P_comparison },
};


void Parser::error(const token& t,std::string_view msg){
    lex.error_at(t.start,t.str.length(),msg);
}


void Parser::error(int start,int hint_len,std::string_view msg){
    lex.error_at(start,hint_len,msg);
}

void Parser::tkskip(tokenType expected,const std::string& msg) {
    if(curToken().type != expected) error(curToken(),msg);
    else tokenMove();
}


bool Parser::tkequal(tokenType expect) {
    return curToken().type == expect;
}


void Parser::tkexpect(tokenType expect,const std::string& msg) {
    if(tokens[cur].type != expect) {
        error(curToken(),msg);
    }
}

bool Parser::tkconsume(tokenType expect) {
    if(tkequal(expect)) {
        tokenMove();
        return true;
    }
    return false;
}


precType get_precedence(tokenType t) {
    auto it = precedence.find(t);
    if(it != precedence.end()) {
        return it->second;
    }
    return precType::P_none;
}


Parser::prefixcall Parser::get_prefix_call(tokenType t) {
    auto it = prefixcalls.find(t);
    if(it == prefixcalls.end()) {
        if(tokens[prev].type == tokenType::T_eof){
            error(prevToken(),"expect a expression");
        }else{
            error(prevToken(),std::format("invalid prefix '{}'",prevToken().str));
        }
    }
    return it->second;
}


Parser::infixcall Parser::get_infix_call(tokenType t) {
    auto it = infixcalls.find(t);
    if(it == infixcalls.end()) {
        if(tokens[prev].type == tokenType::T_eof){
            error(prevToken(),"expect a expression");
        }else{
            error(prevToken(),std::format("invalid infix '{}'",prevToken().str));
        }
    }
    return it->second;
}


std::shared_ptr<Type> Parser::pointerPrefix(std::shared_ptr<Type> base) {
    while(tkconsume(tokenType::T_star)) {
        base = typeFactor::getPointerType(base);
    }
    return base;
}


std::shared_ptr<Type> Parser::declspec() {
    std::shared_ptr<Type> type;
    if(tkequal(tokenType::T_int)) {
        type = typeFactor::getInt();
    }else if(tkequal(tokenType::T_char)) {
        type = typeFactor::getChar();
    }else{
        error(curToken(),std::format("invalid type name:",curToken().str));
    }
    tokenMove();
    return type;
}


std::shared_ptr<Type> Parser::declType() {
    std::shared_ptr<Type> base = declspec();
    base = pointerPrefix(base);
    return base;
}


std::shared_ptr<Node> Parser::parse_numeric() {
    return std::make_shared<numericNode>(prevToken().val,prevToken());
}

std::shared_ptr<Node> Parser::parse_string() {
    std::shared_ptr<Node> str_node = std::make_shared<stringNode>(prevToken());
    std::vector<std::shared_ptr<Node>> decls{str_node};
    global_def.push_back(std::make_shared<vardef>(decls,true));
    return str_node;
}


std::shared_ptr<Node> Parser::identifier() {
    const std::string& str = prevToken().str;
    auto result = sTable.lookup(str);
    if(!result.has_value()) {
        error(prevToken(),std::format("'{}' undeclared",str));
    }
    return std::make_shared<identNode>(result.value());
}


std::shared_ptr<Node> Parser::funcall() {
    token tok = prevToken();
    const std::string& name = tok.str;
    auto result = sTable.lookup(name);
    if(!result.has_value()) {
        error(tok,std::format("function '{}' not found",name));
    }
    const auto& info = result.value();
    auto param_types = static_cast<funcType*>(info._type.get())->getParamTypes();

    std::vector<std::shared_ptr<Node>> args;
    tokenMove();
    size_t i = 0;
    while(!tkconsume(tokenType::T_close_paren)) {
        if(i++ > 0) 
            tkconsume(tokenType::T_comma);
        args.emplace_back(parse_expr(precType::P_none));
    }
    if(param_types.size() != i) {
        error(tok,std::format("'{}' reqiures {} arguments,but {} given",name,param_types.size(),args.size()));
    }
    for(size_t j = 0; j < i; j++) {
        auto lhs = param_types[j];
        auto rhs = args[j]->getType();
        try {
            typeChecker::checkEqual(lhs,rhs);
        }catch(const std::string& msg){
            error(args[j]->strStart(),args[j]->strLength(),std::format("parameter expected '{}' but argument has '{}'",param_types[j]->typestr(),msg));
        }
    }
    auto retTy = static_cast<funcType*>(info._type.get())->getRetType();
    return std::make_shared<funcallNode>(tok,args,SymbolInfo(tok,0,true,retTy,SymbolType::S_func));
}


std::shared_ptr<Node> Parser::arrayvisit() {
    const std::string& name = prevToken().str;
    auto result = sTable.lookup(name);
    if(!result.has_value()) {
        error(prevToken(),std::format("'{}' not found",name));
    }
    const auto& info = result.value();
    auto ty = info._type;
    if(!Type::isArray(ty) && !Type::isPointer(ty)) {
        error(prevToken(),"subscripted value is neither array nor pointer\n");
    }
    tokenMove();
    std::shared_ptr<Node> idx = parse_expr(precType::P_none);
    tkskip(tokenType::T_close_square,"expect ']'");
    return std::make_shared<arrayVisit>(info,idx);
}


std::shared_ptr<Node> Parser::parse_ident() {
    if(tkequal(tokenType::T_open_paren)) {
        return funcall();
    }else if(tkequal(tokenType::T_open_square)) {
        return arrayvisit();
    }
    return identifier();
}


std::shared_ptr<Node> Parser::parse_prefix() {
    token prefix = prevToken();
    int start = curToken().start;
    Node::Kind kind;
    std::shared_ptr<Node> expr = parse_expr(precType::P_prefix);

    if(prefix.type == tokenType::T_addr) {
        if(!expr->equal(Node::Kind::N_identifier) && !expr->equal(Node::Kind::N_arrayvisit)) {
            error(prefix,"'&' requires a lvalue");
        }
        kind = Node::Kind::N_addr;
        return std::make_shared<prefixNode>(expr,typeFactor::getPointerType(expr->getType()),kind,prefix);
    }
    else if(prefix.type == tokenType::T_star) {
        kind = Node::Kind::N_deref;
        bool is_pointer = Type::isPointer(expr->getType()); 
        bool is_array = Type::isArray(expr->getType()); 
        if(!is_pointer && !is_array){
            error(start,expr->strLength(),"'*' reqiures  argument of pointer type");
        }
        if(is_array){
            auto elem_ty = static_cast<arrayType*>(expr->getType().get())->elemTy();
            return std::make_shared<prefixNode>(expr,elem_ty,kind,prefix);
        }else{
            auto base = static_cast<pointerType*>(expr->getType().get())->getBaseType();
            return std::make_shared<prefixNode>(expr,base,kind,prefix);
        }
    }else{
        return std::make_shared<prefixNode>(expr,expr->getType(),Node::Kind::N_trivial,prefix);
    }
}


std::shared_ptr<Node> Parser::parse_group_expr() {
    std::shared_ptr<Node> e = parse_expr(precType::P_none);
    tkskip(tokenType::T_close_paren,"expect ')'");
    return e;
}


std::shared_ptr<Node> Parser::ptr_add(token& op,std::shared_ptr<Node> lhs,std::shared_ptr<Node> rhs) {
    if(Type::isInteger(lhs->getType()) && Type::isInteger(rhs->getType())) {
        return std::make_shared<binaryNode>(op,lhs,rhs,lhs->getType());
    }
    if(Type::isInteger(lhs->getType())) {
        std::swap(lhs,rhs);
    }
    size_t size;
    if(Type::isArray(lhs->getType())) size = static_cast<arrayType*>(lhs->getType().get())->elemSize();  
    else {
        auto ptr = static_cast<pointerType*>(lhs->getType().get());
        size = ptr->getBaseType()->getSize();
    }
    op.type = tokenType::T_star;
    std::shared_ptr<Type> type = typeFactor::getInt();
    std::shared_ptr<Node> num_node = std::make_shared<numericNode>(size,op);
    std::shared_ptr<Node> new_node = std::make_shared<binaryNode>(op,rhs,num_node,type);
    op.type = tokenType::T_plus;
    return std::make_shared<binaryNode>(op,lhs,new_node,lhs->getType());
}


std::shared_ptr<Node> Parser::ptr_sub(token& op,std::shared_ptr<Node> lhs,std::shared_ptr<Node> rhs) {
    if(Type::isInteger(lhs->getType()) && Type::isInteger(rhs->getType())) {
        return std::make_shared<binaryNode>(op,lhs,rhs,lhs->getType());
    }

    size_t size;
    if(Type::isInteger(rhs->getType())) {
        if(Type::isArray(lhs->getType())) size = static_cast<arrayType*>(lhs->getType().get())->elemSize();
        else size = static_cast<pointerType*>(lhs->getType().get())->getSize();
        op.type = tokenType::T_star;
        std::shared_ptr<Type> type = typeFactor::getInt();
        std::shared_ptr<Node> num_node = std::make_shared<numericNode>(size,op);
        std::shared_ptr<Node> new_node = std::make_shared<binaryNode>(op,rhs,num_node,type);
        op.type = tokenType::T_minus;
        return std::make_shared<binaryNode>(op,lhs,new_node,lhs->getType());
    }else {
        if(Type::isArray(lhs->getType())) size = static_cast<arrayType*>(lhs->getType().get())->elemSize();  
        else {
            auto ptr = static_cast<pointerType*>(lhs->getType().get());
            size = ptr->getBaseType()->getSize();
        }
        op.type = tokenType::T_minus;
        std::shared_ptr<Type> type = typeFactor::getInt();
        std::shared_ptr<Node> minus_node = std::make_shared<binaryNode>(op,lhs,rhs,type);
        std::shared_ptr<Node> num_node = std::make_shared<numericNode>(lhs->typeSize(),op);
        op.type = tokenType::T_div;
        return std::make_shared<binaryNode>(op,minus_node,num_node,type);
    }
}



std::shared_ptr<Node> Parser::parse_binary_expr(std::shared_ptr<Node> lhs) {
    token op = prevToken();
    precType prec = get_precedence(op.type);
    std::shared_ptr<Node> rhs = parse_expr(prec);
    std::shared_ptr<Type> type;
    try{
        type = typeChecker::checkBinaryOp(op.type,lhs->getType(),rhs->getType());
    }catch(const std::string& msg){
        error(op,msg);
    }
    if(op.assert(tokenType::T_plus)) {
        return ptr_add(op,lhs,rhs);
    }else if(op.assert(tokenType::T_minus)) {
        return ptr_sub(op,lhs,rhs);
    }
    return std::make_shared<binaryNode>(op,lhs,rhs,type);
}



std::shared_ptr<Node> Parser::parse_expr(precType prec) {
    tokenMove();
    auto prefixcall = get_prefix_call(prevToken().type);
    std::shared_ptr<Node> left = prefixcall();
    while(!tkequal(tokenType::T_eof) && prec < get_precedence(curToken().type)) {
        auto infixcall = get_infix_call(curToken().type);
        tokenMove();
        left = infixcall(left);
    }
    return left;
}


std::shared_ptr<Stmt> Parser::expr_stmt() {
    if(tkconsume(tokenType::T_semicolon)) return nullptr;
    std::shared_ptr<Node> e = parse_expr(precType::P_none);
    tkskip(tokenType::T_semicolon,"expect ';'");
    return std::make_shared<exprStmt>(e);
}


std::shared_ptr<Stmt> Parser::if_stmt() {
    tkskip(tokenType::T_open_paren,"expect '(' after 'if' of if-statement");
    if(tkequal(tokenType::T_close_paren)) {
        error(curToken(),"expect an expression");
    }
    std::shared_ptr<Node> _cond = parse_expr(precType::P_none);
    tkskip(tokenType::T_close_paren,"expect ')' after condition");
    std::shared_ptr<Stmt> _then = parse_stmt();
    std::shared_ptr<Stmt> _else;
    if(tkequal(tokenType::T_else)) {
        tokenMove();
        _else = parse_stmt();
    }
    return std::make_shared<ifStmt>(_cond,_then,_else);
}

void Parser::keywordCheck(const token &tok,const std::string& name) {
    if(lex.iskeyword(name))
        error(tok,std::format("variable name couldn't be a keyword:'{}'",name));
}

bool Parser::is_array() {
    return tkequal(tokenType::T_open_square);
}

bool Parser::is_function() {
    return tkequal(tokenType::T_open_paren);
}


SymbolInfo Parser::varTypeSuffix(std::shared_ptr<Type> type,bool global) {
    type = pointerPrefix(type);
    token tok = curToken();
    tkskip(tokenType::T_identifier,"expect an identifier");
    const std::string& name = tok.str;
    int off = 0;
    if(is_array()) {
            tokenMove();
            int len = curToken().val;
            tkskip(tokenType::T_num,"expect a number");
            tkskip(tokenType::T_close_square,"expect ']'");
            if(!global)
                off = funcdef::newlocalVar(type->getSize() * len);
            auto info = SymbolTable::newSymbol(tok,off,global,typeFactor::getArrayType(len,type),SymbolType::S_array);
            sTable.addSymbol(global,name,info);
            return info;
    }
    else{
        keywordCheck(tok,name);
        if(!global) {
            off = funcdef::newlocalVar(type->getSize());
        }
        auto info = SymbolTable::newSymbol(tok,off,global,type,SymbolType::S_var);
        bool result = sTable.addSymbol(global,name,info); 
        if(!result){
            error(curToken(),std::format("redefine variable: '{}'",name));
        }
        return info;
    }
}


std::shared_ptr<Node> Parser::var_init(const SymbolInfo& info) {
    if(!tkconsume(tokenType::T_assign)) {
        return std::make_shared<identNode>(info);
    }
    else {
        token op = prevToken();
        std::shared_ptr<Node> var = std::make_shared<identNode>(info);
        std::shared_ptr<Node> value = parse_expr(precType::P_none);
        try{
            typeChecker::checkEqual(var->getType(),value->getType());
        }catch(std::string& msg){
            std::string var_type_s = var->getType()->typestr();
            error(op,std::format("{}",msg));
        }
        return std::make_shared<binaryNode>(op,var,value,var->getType()); 
    }
}


std::shared_ptr<Node> Parser::array_init(const SymbolInfo& info) {
    std::vector<std::shared_ptr<Node>> init_lst;
    if(tkconsume(tokenType::T_assign)) {
        tkskip(tokenType::T_open_block,"expect '{'");
        while(1) {
            std::shared_ptr<Node> elem = parse_expr(precType::P_none);
            auto elemty = static_cast<arrayType*>(info._type.get())->elemTy();
            try{
                typeChecker::checkEqual(elem->getType(),elemty);
             } catch(std::string& msg) {
                std::string array_type_s = elemty->typestr();
                std::string errmsg = std::format("array expected '{}' type for initialization,but '{}' has '{}'",array_type_s,elem->strView(),msg);
                error(elem->strStart(),elem->strLength(),errmsg);
            }
            init_lst.push_back(std::move(elem));
            if(tkconsume(tokenType::T_comma)){
                continue;
            }
            if(tkconsume(tokenType::T_close_block)){
                break;
            }else{
                error(curToken(),"expect '}'");
            }
        }
        size_t len = static_cast<arrayType*>(info._type.get())->getlen();
        if(len < init_lst.size()) {
            error(info._tok,std::format("array initialization needs {} elements,but {} in {}",len,init_lst.size(),"{...}"));
        }
    }
    return std::make_shared<arraydef>(info,init_lst);
}




std::shared_ptr<Stmt> Parser::local_vars() {
    std::shared_ptr<Type> type = declType();
    std::vector<std::shared_ptr<Node>> vars;
    bool first = true;
    while(1) {
        if(!first) {
            if(Type::isPointer(type)) type = static_cast<pointerType*>(type.get())->getBaseType();
        }
        auto info = varTypeSuffix(type,false);
        if(info._sTy == SymbolType::S_var) {
            vars.push_back(var_init(info));
        }else{
            vars.push_back(array_init(info));
        }
        if(tkconsume(tokenType::T_comma)){
            first = false;
            continue;
        }
        if(!tkconsume(tokenType::T_semicolon)) {
            error(curToken(),"expect ';'");
        }else{
            break;
        }
    }
    return std::make_shared<vardef>(vars,false);
}



std::shared_ptr<Stmt> Parser::block_stmt() {
    tkskip(tokenType::T_open_block,"expect '{'");
    std::vector<std::shared_ptr<Stmt>> stmts;
    while(!tkequal(tokenType::T_eof) && !tkequal(tokenType::T_close_block)) {
        if(tkequal(tokenType::T_int) || tkequal(tokenType::T_char)) {
            stmts.emplace_back(local_vars());
        }else{
            stmts.emplace_back(parse_stmt());
        }
    }
    tkskip(tokenType::T_close_block,"a block-statament expect '}'");
    return std::make_shared<blockStmt>(stmts);
}


std::shared_ptr<Stmt> Parser::ret_stmt() {
    std::shared_ptr<Stmt> s = expr_stmt();
    return std::make_shared<retStmt>(s);
}

std::shared_ptr<Stmt> Parser::while_stmt() {
    tkskip(tokenType::T_open_paren,"expect '('");
    std::shared_ptr<Node> cond = parse_expr(precType::P_none);
    tkskip(tokenType::T_close_paren,"expect ')'");
    std::shared_ptr<Stmt> body;
    if(!tkconsume(tokenType::T_semicolon)) {
        body = parse_stmt();
    }
    return std::make_shared<whileStmt>(cond,body);
}

std::shared_ptr<Stmt> Parser::init_stmt() {
    if(tkequal(tokenType::T_int) || tkequal(tokenType::T_char)) {
        return local_vars();
    }
    return expr_stmt();
}


std::shared_ptr<Stmt> Parser::for_stmt() {
    tkskip(tokenType::T_open_paren,"expect '(");
    std::shared_ptr<Stmt> init;
    std::shared_ptr<Stmt> cond;
    std::shared_ptr<Node> inc;
    std::shared_ptr<Stmt> body;
    sTable.enter();
    init = init_stmt();
    cond = expr_stmt();
    if(!tkconsume(tokenType::T_close_paren)){
        inc = parse_expr(precType::P_none);
        tkskip(tokenType::T_close_paren,"expect ')'");
    } 
    body = parse_stmt();
    sTable.leave();
    return std::make_shared<forStmt>(init,cond,inc,body);
}


std::shared_ptr<Stmt> Parser::parse_stmt() {
    if(tkequal(tokenType::T_open_block)) {
        sTable.enter();
        std::shared_ptr<Stmt> s = block_stmt();
        sTable.leave();
        return s;
    } 
    if(tkconsume(tokenType::T_if))     return if_stmt();
    if(tkconsume(tokenType::T_return)) return ret_stmt();
    if(tkconsume(tokenType::T_while))  return while_stmt();
    if(tkconsume(tokenType::T_for))    return for_stmt();
    if(tkconsume(tokenType::T_semicolon)) return nullptr;
    return expr_stmt();
}


std::vector<std::shared_ptr<Node>> Parser::funcParams(
    const token& tok,
    std::shared_ptr<Type> retType) {

    bool first = false;
    const std::string& name = tok.str;
    std::vector<std::shared_ptr<Node>> params;
    std::vector<std::shared_ptr<Type>> paramTypes;
    while(!tkequal(tokenType::T_close_paren)) {
        if(first) {
            tkskip(tokenType::T_comma,"expect ','");
        }
        first = true;
        auto type = declType();
        params.push_back(std::make_shared<identNode>(varTypeSuffix(type,false)));
        paramTypes.push_back(std::move(type));
    }
    tkskip(tokenType::T_close_paren,"expect ')'");
    auto info = SymbolTable::newSymbol(tok,0,true,typeFactor::getFuncType(retType,paramTypes),SymbolType::S_func);
    sTable.addSymbol(true,name,info);
    return params;
}



std::shared_ptr<Stmt> Parser::decl_func(std::shared_ptr<Type> retType) {
    token tok = prevToken();
    tokenMove();
    sTable.enter();
    std::vector<std::shared_ptr<Node>> _params = funcParams(tok,retType);
    std::shared_ptr<Stmt> body = block_stmt();
    sTable.leave();
    auto func = funcdef::newFunction(body,tok.str,_params);
    return func;
}


std::shared_ptr<Stmt> Parser::global_vars(std::shared_ptr<Type> type) {
    std::vector<std::shared_ptr<Node>> vars;
    while(1) {
        auto info = varTypeSuffix(type,true);
        if(info._sTy == SymbolType::S_var) {
            vars.push_back(std::make_shared<identNode>(info));
        }else {
            std::vector<std::shared_ptr<Node>> init;
            vars.push_back(std::make_shared<arraydef>(info,init));
        }
        if(tkconsume(tokenType::T_comma)){
            continue;
        }
        if(!tkconsume(tokenType::T_semicolon)) {
            error(curToken(),"expect ';'");
        }else{
            break;
        }
    }
    return std::make_shared<vardef>(vars,true);
}


Prog Parser::start() {
    while(!tkequal(tokenType::T_eof)) {
        std::shared_ptr<Type> type = declType();
        tokenMove();
        if(is_function()) {
            global_def.push_back(decl_func(type));
        }
        else {
            tokenBack();
            global_def.push_back(global_vars(type));
        }
        while(tkequal(tokenType::T_semicolon)) tokenMove();
    }
    return Prog(global_def);
}


void Parser::tokenMove() {
    prev = cur;
    cur++;
    tokens.push_back(lex.newToken());
#ifdef DEBUG
    std::cerr << "token: " << tokenstrs[curToken().type] <<" -> " << curToken().str << "\n";
#endif
}

void Parser::tokenBack() {
    prev--;
    cur--;
}

const token& Parser::curToken() {
    return tokens[cur];
}

const token &Parser::prevToken() {
    return tokens[prev];
}


void Parser::setup() {
   prefixcalls[tokenType::T_num] = [this]() { return parse_numeric(); }; 
   prefixcalls[tokenType::T_identifier] = [this]() { return parse_ident(); }; 
   prefixcalls[tokenType::T_string] = [this]() { return parse_string(); }; 
   prefixcalls[tokenType::T_minus] = [this]() { return parse_prefix(); }; 
   prefixcalls[tokenType::T_star] = [this]() { return parse_prefix(); }; 
   prefixcalls[tokenType::T_addr] = [this]() { return parse_prefix(); }; 
   prefixcalls[tokenType::T_open_paren] = [this]() { return parse_group_expr(); }; 

   infixcalls[tokenType::T_plus] =  [this](std::shared_ptr<Node>& left) {
     return parse_binary_expr(std::move(left)); };
   infixcalls[tokenType::T_minus] = [this](std::shared_ptr<Node>& left) {
     return parse_binary_expr(std::move(left)); };
   infixcalls[tokenType::T_star] =  [this](std::shared_ptr<Node>& left) { 
    return parse_binary_expr(std::move(left)); };
   infixcalls[tokenType::T_div] =   [this](std::shared_ptr<Node>& left) { 
    return parse_binary_expr(std::move(left)); };
   infixcalls[tokenType::T_lt] =    [this](std::shared_ptr<Node>& left) {
     return parse_binary_expr(std::move(left)); };
   infixcalls[tokenType::T_le] =    [this](std::shared_ptr<Node>& left) {
     return parse_binary_expr(std::move(left)); };
   infixcalls[tokenType::T_gt] =    [this](std::shared_ptr<Node>& left) {
     return parse_binary_expr(std::move(left)); };
   infixcalls[tokenType::T_ge] =    [this](std::shared_ptr<Node>& left) { 
    return parse_binary_expr(std::move(left)); };
   infixcalls[tokenType::T_neq] =   [this](std::shared_ptr<Node>& left) {
     return parse_binary_expr(std::move(left)); };
   infixcalls[tokenType::T_eq] =    [this](std::shared_ptr<Node>& left) {
     return parse_binary_expr(std::move(left)); };
   infixcalls[tokenType::T_assign] = [this](std::shared_ptr<Node>& left) {
     return parse_binary_expr(std::move(left)); };
   infixcalls[tokenType::T_bit_and] = [this](std::shared_ptr<Node>& left) {
     return parse_binary_expr(std::move(left)); };

    precedence[tokenType::T_num] = precType::P_none,
    precedence[tokenType::T_eof] = precType::P_none;
    precedence[tokenType::T_plus] = precType::P_factor;
    precedence[tokenType::T_minus] = precType::P_factor;
    precedence[tokenType::T_star] = precType::P_term;
    precedence[tokenType::T_div] = precType::P_term;
    precedence[tokenType::T_assign] = precType::P_assign; 
    precedence[tokenType::T_lt] = precType::P_comparison; 
    precedence[tokenType::T_le] = precType::P_comparison; 
    precedence[tokenType::T_gt] = precType::P_comparison; 
    precedence[tokenType::T_ge] = precType::P_comparison; 
    precedence[tokenType::T_neq] = precType::P_comparison; 
    precedence[tokenType::T_eq] = precType::P_comparison;
    precedence[tokenType::T_identifier]=precType::P_none;
    precedence[tokenType::T_bit_and] = precType::P_bit;
}


Parser::Parser(const char *src):lex(src) {
    setup();
    tokenMove();
}

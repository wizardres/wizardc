#include "include/parse.h"

Scope scope;

#ifdef DEBUG
std::map<tokenType,std::string_view> tokenstrs {
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
    {tokenType::T_addr,"T_addr"}
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

void Parser::tkskip(tokenType expected,std::string_view msg) {
    if(curToken().type != expected) error(curToken(),msg);
    else tokenMove();
}


bool Parser::tkequal(tokenType expect) {
    return curToken().type == expect;
}


void Parser::tkexpect(tokenType expect,std::string_view msg) {
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
            error(prevToken(),std::format("invalid '{}'",prevToken().str));
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
            error(prevToken(),std::format("invalid '{}'",prevToken().str));
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
    tkskip(tokenType::T_int,"expect 'int'");
    return typeFactor::getInt();
}

std::shared_ptr<Type> Parser::declType() {
    std::shared_ptr<Type> base = declspec();
    base = pointerPrefix(base);
    return base;
}


std::unique_ptr<Node> Parser::parse_numeric() {
    return std::make_unique<numericNode>(prevToken().val,typeFactor::getInt(),prevToken());
}


std::unique_ptr<Node> Parser::identifier() {
    std::string_view str = prevToken().str;
    auto result = scope.lookup_allscope(str);
    if(!result.has_value()) {
        error(prevToken(),std::format("'{}' undeclared",str));
    }
    return std::make_unique<identNode>(result.value());
}


std::unique_ptr<Node> Parser::funcall() {
    token tok = prevToken();
    std::string_view name = tok.str;
    auto result = scope.lookup_global(name);
    if(!result.has_value()) {
        error(tok,std::format("function '{}' not found",name));
    }
    auto orig_funcobj = result.value();
    auto retType = static_cast <funcObj*>(orig_funcobj.get())->getType();
    auto orig_param_types = static_cast <funcObj*>(orig_funcobj.get())->getParamTypes();

    std::vector<std::unique_ptr<Node>> args;
    tokenMove();
    size_t i = 0;
    while(!tkconsume(tokenType::T_close_paren)) {
        if(i++ > 0) 
            tkconsume(tokenType::T_comma);
        args.emplace_back(parse_expr(precType::P_none));
    }
    if(orig_param_types.size() != i) {
        error(tok,std::format("'{}' reqiures {} arguments,but {} given",name,orig_param_types.size(),args.size()));
    }
    for(size_t j = 0; j < i; j++) {
        auto lhs = args[j]->getType();
        auto rhs = orig_param_types[j];
        if(!typeChecker::checkEqual(lhs,rhs)) {
            auto &arg = args[j];
            error(arg->strStart(),arg->strLength(),std::format("invalid argument type '{}'",arg->getType()->typestr()));
        }
    }
    return std::make_unique<funcallNode>(tok,args,orig_funcobj);
}


std::unique_ptr<Node> Parser::arrayvisit() {
    token tok = prevToken();
    std::string_view arr_name = tok.str;
    auto result = scope.lookup_allscope(arr_name);
    if(!result.has_value()) {
        error(prevToken(),std::format("'{}' not found",arr_name));
    }
    tokenMove();
    size_t idx = curToken().val;
    tkskip(tokenType::T_num,"expect an num");
    tkskip(tokenType::T_close_square,"expect ']'");
    return std::make_unique<arrayVisit>(result.value(),idx,tok);
}


std::unique_ptr<Node> Parser::parse_ident() {
    if(tkequal(tokenType::T_open_paren)) {
        return funcall();
    }else if(tkequal(tokenType::T_open_square)) {
        return arrayvisit();
    }
    return identifier();
}


std::unique_ptr<Node> Parser::parse_prefix() {
    token prefix = prevToken();
    int start = curToken().start;
    Node::Kind kind;
    std::unique_ptr<Node> expr = parse_expr(precType::P_prefix);

    if(prefix.type == tokenType::T_addr) {
        if(!Node::ndEqual(expr,Node::Kind::N_identifier) && !Node::ndEqual(expr,Node::Kind::N_arrayvisit)) {
            error(prefix,"'&' requires a lvalue");
        }
        kind = Node::Kind::N_addr;
        if(Type::isArray(expr->getType())){
            auto base = static_cast<arrayType*>(expr->getType().get())->elemTy();
            return std::make_unique<prefixNode>(expr,typeFactor::getPointerType(base),kind,prefix);
        }
        return std::make_unique<prefixNode>(expr,typeFactor::getPointerType(expr->getType()),kind,prefix);
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
            return std::make_unique<prefixNode>(expr,elem_ty,kind,prefix);
        }else{
            auto base = static_cast<pointerType*>(expr->getType().get())->getBaseType();
            return std::make_unique<prefixNode>(expr,base,kind,prefix);
        }
    }else{
        return std::make_unique<prefixNode>(expr,expr->getType(),Node::Kind::N_trivial,prefix);
    }
}


std::unique_ptr<Node> Parser::parse_group_expr() {
    std::unique_ptr<Node> e = parse_expr(precType::P_none);
    tkskip(tokenType::T_close_paren,"expect ')'");
    return e;
}


std::unique_ptr<Node> Parser::parse_binary_expr(std::unique_ptr<Node> lhs) {
    token op = prevToken();
    precType prec = get_precedence(op.type);
    std::unique_ptr<Node> rhs = parse_expr(prec);
    std::shared_ptr<Type> type;
    try{
        if(op.type == tokenType::T_assign){
            bool res = typeChecker::checkEqual(lhs->getType(),rhs->getType());
            if(!res){
                std::cerr << " type is not equal\n";
                exit(-1);
            }
        }
        type = typeChecker::checkBinaryOp(op.type,lhs->getType(),rhs->getType());
    }catch(const char *msg){
        error(op,msg);
    }
    return std::make_unique<binaryNode>(op,lhs,rhs,type);
}



std::unique_ptr<Node> Parser::parse_expr(precType prec) {
    tokenMove();
    auto prefixcall = get_prefix_call(prevToken().type);
    std::unique_ptr<Node> left = prefixcall();
    while(!tkequal(tokenType::T_eof) && prec < get_precedence(curToken().type)) {
        auto infixcall = get_infix_call(curToken().type);
        tokenMove();
        left = infixcall(left);
    }
    return left;
}


std::unique_ptr<Stmt> Parser::expr_stmt() {
    std::unique_ptr<Node> e = parse_expr(precType::P_none);
    tkskip(tokenType::T_semicolon,"expect ';'");
    return std::make_unique<exprStmt>(e);
}


std::unique_ptr<Stmt> Parser::if_stmt() {
    tokenMove();
    tkskip(tokenType::T_open_paren,"expect '(' after 'if' of if-statement");
    if(tkequal(tokenType::T_close_paren)) {
        error(curToken(),"expect an expression");
    }
    std::unique_ptr<Node> _cond = parse_expr(precType::P_none);
    tkskip(tokenType::T_close_paren,"expect ')' after condition");
    std::unique_ptr<Stmt> _then = parse_stmt();
    std::unique_ptr<Stmt> _else;
    if(tkequal(tokenType::T_else)) {
        tokenMove();
        _else = parse_stmt();
    }
    return std::make_unique<ifStmt>(_cond,_then,_else);
}

void Parser::keywordCheck(const token &tok,std::string_view name) {
    if(lex.iskeyword(name))
        error(tok,std::format("variable name couldn't be a keyword:'{}'",name));
}

bool Parser::is_array() {
    return tkequal(tokenType::T_open_square);
}

bool Parser::is_function() {
    return tkequal(tokenType::T_open_paren);
}


std::shared_ptr<Obj> Parser::varTypeSuffix(std::shared_ptr<Type> type) {
    type = pointerPrefix(type);
    token tok = curToken();
    tkskip(tokenType::T_identifier,"expect an identifier");
    std::string_view name = tok.str;
    bool isglobal = scope.isglobal();
    int off = 0;
    if(is_array()) {
            tokenMove();
            int len = curToken().val;
            tkskip(tokenType::T_num,"expect a number");
            tkskip(tokenType::T_close_square,"expect ']'");
            if(!isglobal)
                off = funcdef::newlocalVar(type->getSize() * len);
            auto obj = objFactor::getArray(tok,typeFactor::getArrayType(len,type),isglobal,off);
            scope.insertObj(isglobal,name,obj);
            return obj;
    }
    else{
        keywordCheck(tok,name);
        std::shared_ptr<Obj> obj;
        if(!isglobal) {
            off = funcdef::newlocalVar(type->getSize());
        }
        obj = objFactor::getVariable(tok,type,isglobal,off); 
        bool result = scope.insertObj(isglobal,name,obj); 
        if(!result){
            error(curToken(),std::format("redefine variable: '{}'",name));
        }
        return obj;
    }
}

std::unique_ptr<Node> Parser::var_init(std::shared_ptr<Obj> obj) {
    if(!tkconsume(tokenType::T_assign)) {
        return std::make_unique<identNode>(obj);
    }
    else {
        token op = prevToken();
        std::unique_ptr<Node> var = std::make_unique<identNode>(obj);
        std::unique_ptr<Node> value;
        value = parse_expr(precType::P_none);
        if(!typeChecker::checkEqual(var->getType(),value->getType())){
            std::string var_type_s = var->getType()->typestr();
            std::string val_type_s = value->getType()->typestr();
            error(op,std::format("variable reqiures '{}' type,but value has '{}'",var_type_s,val_type_s));
        }
        return std::make_unique<binaryNode>(op,var,value,var->getType()); 
    }
}


std::unique_ptr<Node> Parser::array_init(std::shared_ptr<Obj> obj) {
    std::vector<std::unique_ptr<Node>> init_lst;
    if(tkconsume(tokenType::T_assign)) {
        tkskip(tokenType::T_open_block,"expect '{'");
        while(1) {
            std::unique_ptr<Node> elem = parse_expr(precType::P_none);
            auto elemty = static_cast<arrayObj*>(obj.get())->getElemType();
            if(!typeChecker::checkEqual(elem->getType(),elemty)) {
                std::string array_type_s = elemty->typestr();
                std::string val_type_s = elem->getType()->typestr();
                std::string errmsg = std::format("array requires '{}' type for initialization,but '{}' has '{}'",array_type_s,elem->strView(),val_type_s);
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
        size_t len = static_cast<arrayObj*>(obj.get())->len();
        if(len < init_lst.size()) {
            error(obj->getToken(),std::format("array initialization needs {} elements,but {} in {}",len,init_lst.size(),"{...}"));
        }
    }
    return std::make_unique<arraydef>(obj,init_lst);
}




std::unique_ptr<Stmt> Parser::local_vars(std::shared_ptr<Type> type) {
    std::vector<std::unique_ptr<Node>> vars;
    while(1) {
        auto obj = varTypeSuffix(type);
        if(obj->getKind() == Obj::objKind::variable) {
            vars.push_back(var_init(obj));
        }else{
            vars.push_back(array_init(obj));
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
    return std::make_unique<vardef>(vars,false);
}



std::unique_ptr<Stmt> Parser::block_stmt() {
    tkskip(tokenType::T_open_block,"expect '}'");
    std::vector<std::unique_ptr<Stmt>> stmts;
    while(!tkequal(tokenType::T_eof) && !tkequal(tokenType::T_close_block)) {
        if(tkequal(tokenType::T_int)) {
            stmts.emplace_back(local_vars(declType()));
        }else{
            stmts.emplace_back(parse_stmt());
        }
    }
    tkskip(tokenType::T_close_block,"a block-statament expect '}'");
    return std::make_unique<blockStmt>(stmts);
}


std::unique_ptr<Stmt> Parser::ret_stmt() {
    tokenMove();
    std::unique_ptr<Stmt> s = expr_stmt();
    return std::make_unique<retStmt>(s);
}


std::unique_ptr<Stmt> Parser::parse_stmt() {
    if(tkequal(tokenType::T_open_block)) {
        scope.enter();
        std::unique_ptr<Stmt> s = block_stmt();
        scope.leave();
        return s;
    } 
    if(tkequal(tokenType::T_if)) return if_stmt();
    if(tkequal(tokenType::T_return)) return ret_stmt();
    return expr_stmt();
}


std::vector<std::unique_ptr<Node>> Parser::funcParams(
    const token& tok,
    std::shared_ptr<Type>& retType) {

    bool first = false;
    std::string_view name = tok.str;
    std::vector<std::unique_ptr<Node>> params;
    std::vector<std::shared_ptr<Type>> paramTypes;
    while(!tkequal(tokenType::T_close_paren)) {
        if(first) {
            tkskip(tokenType::T_comma,"expect ','");
        }
        first = true;
        auto type = declType();
        params.push_back(std::make_unique<identNode>(varTypeSuffix(type)));
        paramTypes.push_back(std::move(type));
    }
    tkskip(tokenType::T_close_paren,"expect ')'");
    bool isglobal = true;
    scope.insertObj(isglobal,name,objFactor::getFunction(tok,typeFactor::getFuncType(retType,paramTypes)));
    return params;
}



std::unique_ptr<Stmt> Parser::decl_func(std::shared_ptr<Type> retType) {
    token tok = prevToken();
    tokenMove();
    scope.enter();
    std::vector<std::unique_ptr<Node>> _params = funcParams(tok,retType);
    std::unique_ptr<Stmt> body = block_stmt();
    scope.leave();
    auto func = funcdef::newFunction(body,tok.str,_params);
    return func;
}


std::unique_ptr<Stmt> Parser::global_vars(std::shared_ptr<Type> type) {
    std::vector<std::unique_ptr<Node>> vars;
    while(1) {
        auto obj = varTypeSuffix(type);
        if(obj->getKind() == Obj::objKind::variable) {
            vars.push_back(std::make_unique<identNode>(obj));
        }else {
            std::vector<std::unique_ptr<Node>> init;
            vars.push_back(std::make_unique<arraydef>(obj,init));
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
    return std::make_unique<vardef>(vars,true);
}


Prog Parser::start() {
    std::vector<std::unique_ptr<Stmt>> stmts;
    while(!tkequal(tokenType::T_eof)) {
        std::shared_ptr<Type> type = declType();
        tokenMove();
        if(is_function()) {
            stmts.push_back(decl_func(type));
        }
        else {
            tokenBack();
            stmts.push_back(global_vars(type));
        }
        while(tkequal(tokenType::T_semicolon)) tokenMove();
    }
    return Prog(stmts);
}


void Parser::tokenMove() {
    prev = cur;
    cur++;
    tokens.push_back(lex.newToken());
#ifdef DEBUG
    std::cerr << "token: " << tokenstrs[curTokenType()] <<" -> " << curTokenStr() << "\n";
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
   prefixcalls[tokenType::T_minus] = [this]() { return parse_prefix(); }; 
   prefixcalls[tokenType::T_star] = [this]() { return parse_prefix(); }; 
   prefixcalls[tokenType::T_addr] = [this]() { return parse_prefix(); }; 
   prefixcalls[tokenType::T_open_paren] = [this]() { return parse_group_expr(); }; 

   infixcalls[tokenType::T_plus] =  [this](std::unique_ptr<Node>& left) {
     return parse_binary_expr(std::move(left)); };
   infixcalls[tokenType::T_minus] = [this](std::unique_ptr<Node>& left) {
     return parse_binary_expr(std::move(left)); };
   infixcalls[tokenType::T_star] =  [this](std::unique_ptr<Node>& left) { 
    return parse_binary_expr(std::move(left)); };
   infixcalls[tokenType::T_div] =   [this](std::unique_ptr<Node>& left) { 
    return parse_binary_expr(std::move(left)); };
   infixcalls[tokenType::T_lt] =    [this](std::unique_ptr<Node>& left) {
     return parse_binary_expr(std::move(left)); };
   infixcalls[tokenType::T_le] =    [this](std::unique_ptr<Node>& left) {
     return parse_binary_expr(std::move(left)); };
   infixcalls[tokenType::T_gt] =    [this](std::unique_ptr<Node>& left) {
     return parse_binary_expr(std::move(left)); };
   infixcalls[tokenType::T_ge] =    [this](std::unique_ptr<Node>& left) { 
    return parse_binary_expr(std::move(left)); };
   infixcalls[tokenType::T_neq] =   [this](std::unique_ptr<Node>& left) {
     return parse_binary_expr(std::move(left)); };
   infixcalls[tokenType::T_eq] =    [this](std::unique_ptr<Node>& left) {
     return parse_binary_expr(std::move(left)); };
   infixcalls[tokenType::T_assign] = [this](std::unique_ptr<Node>& left) {
     return parse_binary_expr(std::move(left)); };
   infixcalls[tokenType::T_bit_and] = [this](std::unique_ptr<Node>& left) {
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

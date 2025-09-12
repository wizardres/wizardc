#include "include/parse.h"

token prev;             // for pratt prase
token cur;              // for pratt parse
lexer lex;
Scope scope;


#ifdef DEBUG
std::map<token_t,std::string_view> tokenstrs {
    {token_t::T_num,"T_num"},
    {token_t::T_string,"T_string"},
    {token_t::T_identifier,"T_identifier"},
    {token_t::T_keyword,"T_keyword"},
    {token_t::T_plus,"T_plus"},
    {token_t::T_minus,"T_minus"},
    {token_t::T_star,"T_star"},
    {token_t::T_div,"T_div"},
    {token_t::T_open_paren,"T_open_paren"},
    {token_t::T_close_paren,"T_close_paren"},
    {token_t::T_open_square,"T_open_square"},
    {token_t::T_close_square,"T_close_square"},
    {token_t::T_open_block,"T_open_block"},
    {token_t::T_close_block,"T_close_block"},
    {token_t::T_semicolon,"T_semicolon"},
    {token_t::T_lt,"T_lt"},
    {token_t::T_gt,"T_gt"},
    {token_t::T_not,"T_not"},
    {token_t::T_assign,"T_assign"},
    {token_t::T_eq,"T_eq"},
    {token_t::T_le,"T_le"},
    {token_t::T_ge,"T_ge"},
    {token_t::T_neq,"T_neq"},
    {token_t::T_comma,"T_comma"},
    {token_t::T_period,"T_period"},
    {token_t::T_if,"T_if"},
    {token_t::T_else,"T_else"},
    {token_t::T_return,"T_return"},
    {token_t::T_int,"T_int"},
    {token_t::T_eof,"T_eof"},
    {token_t::T_addr,"T_addr"}
};
#endif

std::map<token_t,precedence_t> precedence {
    { token_t::T_num,precedence_t::P_none },
    { token_t::T_eof,precedence_t::P_none },
    { token_t::T_identifier,precedence_t::P_none },
    { token_t::T_bit_and,precedence_t::P_bit },
    { token_t::T_plus,precedence_t::P_factor },
    { token_t::T_minus,precedence_t::P_factor },
    { token_t::T_star,precedence_t::P_term },
    { token_t::T_div,precedence_t::P_term },
    { token_t::T_assign,precedence_t::P_assign },
    { token_t::T_lt,precedence_t::P_comparison },
    { token_t::T_le,precedence_t::P_comparison },
    { token_t::T_gt,precedence_t::P_comparison },
    { token_t::T_ge,precedence_t::P_comparison },
    { token_t::T_neq,precedence_t::P_comparison },
    { token_t::T_eq,precedence_t::P_comparison },
};

std::map<token_t,prefix_call> prefixcalls {
    { token_t::T_num,parse_numeric },
    { token_t::T_identifier,parse_ident },
    { token_t::T_minus,parse_prefix },
    { token_t::T_star,parse_prefix },
    { token_t::T_addr,parse_prefix },
    { token_t::T_open_paren,parse_group_expr },
};

std::map<token_t,infix_call> infixcalls {
    { token_t::T_plus, parse_binary_expr },
    { token_t::T_minus, parse_binary_expr },
    { token_t::T_star, parse_binary_expr },
    { token_t::T_div, parse_binary_expr },
    { token_t::T_lt, parse_binary_expr },
    { token_t::T_le, parse_binary_expr },
    { token_t::T_gt, parse_binary_expr },
    { token_t::T_ge, parse_binary_expr },
    { token_t::T_neq, parse_binary_expr },
    { token_t::T_eq, parse_binary_expr },
    { token_t::T_assign, parse_binary_expr },
    { token_t::T_bit_and, parse_binary_expr },
};



void next_token() {
    prev = cur;
    cur = lex.newToken();
#ifdef DEBUG
    std::cerr << "token is:" << "'" << tokenstrs[cur.type] << "' -> " << cur.str << "\n";
#endif
}

void error(token& t,std::string_view msg){
    lex.error_at(t.start,t.str.length(),msg);
}

void error(int start,int hint_len,std::string_view msg){
    lex.error_at(start,hint_len,msg);
}

void tkskip(token_t expected,std::string_view msg) {
    if(cur.type != expected) error(cur,msg);
    else next_token();
}

bool tkequal(token_t expect) {
    return cur.type == expect;
}

void tkexpect(token_t expect,std::string_view msg) {
    if(cur.type != expect) {
        error(cur,msg);
    }
}

bool tkconsume(token_t expect) {
    if(tkequal(expect)) {
        next_token();
        return true;
    }
    return false;
}

precedence_t get_precedence(token_t t) {
    auto it = precedence.find(t);
    if(it != precedence.end()) {
        return it->second;
    }
    return precedence_t::P_none;
}

prefix_call get_prefix_call(token_t t) {
    auto it = prefixcalls.find(t);
    if(it == prefixcalls.end()) {
        if(prev.type == token_t::T_eof){
            error(prev,"expect a expression");
        }else{
            error(prev,std::format("invalid '{}'",prev.str));
        }
    }
    return it->second;
}


infix_call get_infix_call(token_t t) {
    auto it = infixcalls.find(t);
    if(it == infixcalls.end()) {
        if(prev.type == token_t::T_eof){
            error(prev,"expect a expression");
        }else{
            error(prev,std::format("invalid '{}'",prev.str));
        }
    }
    return it->second;
}


std::shared_ptr<Type> declspec() {
    tkskip(token_t::T_int,"expect 'int'");
    return typeFactor::getInt();
}


std::shared_ptr<Type> declarator(std::shared_ptr<Type> base) {
    while(tkconsume(token_t::T_star)) {
        base = typeFactor::getPointerType(base);
    }
    return base;
}


std::shared_ptr<Type> declType() {
    std::shared_ptr<Type> base = declspec();
    base = declarator(base);
    return base;
}


std::unique_ptr<Expr> parse_numeric() {
    return std::make_unique<numericExpr>(prev.val,typeFactor::getInt(),prev);
}


std::unique_ptr<Expr> identifier() {
    auto result = scope.lookup_allscope(prev.str);
    if(!result.has_value()) {
        error(prev,std::format("'{}' not found",prev.str));
    }
    return std::make_unique<identExpr>(prev.str,result.value(),prev);
}


std::unique_ptr<Expr> funcall() {
    token tok = prev;
    std::string_view name = tok.str;
    auto orig_funcobj = scope.lookup_global(name);
    if(!orig_funcobj.has_value()) {
        error(tok,std::format("function '{}' not found",name));
    }

    auto orig_functype = static_cast <funcObj*>(orig_funcobj.value().get())->getType();
    auto &orig_param_types = static_cast<funcType*>(orig_functype.get())->getParamTypes();
    auto retType = static_cast<funcType*>(orig_functype.get())->getRetType();

    std::vector<std::unique_ptr<Expr>> args;
    next_token();
    size_t i = 0;
    while(!tkconsume(token_t::T_close_paren)) {
        if(i++ > 0) 
            tkconsume(token_t::T_comma);
        args.emplace_back(parse_expr(precedence_t::P_none));
    }
    if(orig_param_types.size() != i) {
        error(prev,std::format("'{}' reqiures {} arguments,but {} given",name,orig_param_types.size(),args.size()));
    }
    for(size_t j = 0; j < i; j++) {
        auto lhs = args[j]->getType();
        auto rhs = orig_param_types[j];
        if(!typeChecker::checkEqual(lhs,rhs)) {
            auto &arg = args[j];
            error(arg->strStart(),arg->strLength(),std::format("argument '{}' has incompatible type",arg->strView()));
        }
    }
    return std::make_unique<funcallExpr>(tok,name,args,retType);
}


std::unique_ptr<Expr> parse_ident() {
    if(tkequal(token_t::T_open_paren)) {
        return funcall();
    }
    return identifier();
}


std::unique_ptr<Expr> parse_prefix() {
    token prefix = prev;
    int start = cur.start;
    Expr::Kind kind;
    std::unique_ptr<Expr> expr = parse_expr(precedence_t::P_prefix);

    if(prefix.type == token_t::T_addr) {
        if(!Expr::ndequal(expr,Expr::Kind::N_identifier)) {
            error(prefix,"'&' requires a lvalue");
        }
        kind = Expr::Kind::N_addr;
        return std::make_unique<prefixExpr>(expr,typeFactor::getPointerType(expr->getType()),kind,prefix);
    }
    else if(prefix.type == token_t::T_star) {
        if(!typeChecker::checkEqual(expr->getType(),Type::Kind::T_ptr)){
            error(start,expr->strLength(),"'*' reqiures  argument of pointer type");
        }
        kind = Expr::Kind::N_deref;
        auto base = static_cast<pointerType*>(expr->getType().get())->getBaseType();
        return std::make_unique<prefixExpr>(expr,base,kind,prefix);
    }
    return std::make_unique<prefixExpr>(expr,expr->getType(),Expr::Kind::N_trivial,prefix);
}


std::unique_ptr<Expr> parse_group_expr() {
    std::unique_ptr<Expr> e = parse_expr(precedence_t::P_none);
    tkskip(token_t::T_close_paren,"expect ')'");
    return e;
}


std::unique_ptr<Expr> parse_binary_expr(std::unique_ptr<Expr> &lhs) {
    token op = prev;
    precedence_t precedence = get_precedence(op.type);
    std::unique_ptr<Expr> rhs = parse_expr(precedence);
    std::shared_ptr<Type> type;
    try{
        type = typeChecker::checkBinaryOp(op.type,lhs->getType(),rhs->getType());
    }catch(const char *msg){
        error(op,msg);
    }
    return std::make_unique<binaryExpr>(op,lhs,rhs,type);
}



std::unique_ptr<Expr> parse_expr(precedence_t prec) {
    next_token();
    auto prefixcall = get_prefix_call(prev.type);
    std::unique_ptr<Expr> left = prefixcall();
    
    while(!tkequal(token_t::T_eof) && prec < get_precedence(cur.type)) {
        auto infixcall = get_infix_call(cur.type);
        next_token();
        left = infixcall(left);
    }
    return left;
}


std::unique_ptr<Stmt> expr_stmt() {
    std::unique_ptr<Expr> e = parse_expr(precedence_t::P_none);
    tkskip(token_t::T_semicolon,"expect ';'");
    return std::make_unique<exprStmt>(e);
}


std::unique_ptr<Stmt> if_stmt() {
    next_token();
    tkskip(token_t::T_open_paren,"expect '(' after 'if' of if-statement");
    if(tkequal(token_t::T_close_paren)) {
        error(cur,"expect an expression");
    }
    std::unique_ptr<Expr> _cond = parse_expr(precedence_t::P_none);
    tkskip(token_t::T_close_paren,"expect ')' after condition");
    std::unique_ptr<Stmt> _then = parse_stmt();
    std::unique_ptr<Stmt> _else;
    if(tkequal(token_t::T_else)) {
        next_token();
        _else = parse_stmt();
    }
    return std::make_unique<ifStmt>(_cond,_then,_else);
}


std::unique_ptr<Expr> newvar(std::shared_ptr<Type> ty) {
    std::string_view name = cur.str;
    tkskip(token_t::T_identifier,"expect an identifier");
    if(lex.iskeyword(name)) {
        error(prev,std::format("variable name couldn't be a keyword:'{}'",name));
    }

    int off;
    std::shared_ptr<Obj> obj;
    bool result;
    if(!scope.isglobal()) {
        off = funcdef::newlocalVar(ty->getSize());
        obj = objFactor::getlocalObj(off,ty);
        result = scope.insert_local(name,obj);
    }
    else{
        off = 0;
        obj = objFactor::getglobalObj(ty); 
        result = scope.insert_global(name,obj); 
    }
    if(!result){
        error(cur,std::format("redefine variable: '{}'",name));
    }
    auto ident = std::make_unique<identExpr>(name,obj,cur);
    return ident;
}



std::unique_ptr<Stmt> decl_var(std::shared_ptr<Type> type) {
    std::vector<std::unique_ptr<Expr>> vars;
    bool first = false;
    bool isglobal = false;
    while(!tkconsume(token_t::T_semicolon)) {
        if(first) {
            tkskip(token_t::T_comma,"expect ','");
        }
        first = true;

        std::unique_ptr<Expr> var = newvar(type);
        std::unique_ptr<Expr> value;
        token op = cur;
        if(tkequal(token_t::T_assign)) {
            next_token();
            value = parse_expr(precedence_t::P_none);
            try{
                typeChecker::checkEqual(var->getType(),value->getType());
            }catch(const char *msg) {

            }
        }
        vars.push_back(std::make_unique<binaryExpr>(op,var,value,type));
    }
    return std::make_unique<vardef>(vars,isglobal);
}


std::unique_ptr<Stmt> block_stmt() {
    tkskip(token_t::T_open_block,"expect '}'");
    std::vector<std::unique_ptr<Stmt>> stmts;
    while(!tkequal(token_t::T_eof) && !tkequal(token_t::T_close_block)) {
        if(tkequal(token_t::T_int)) {
            stmts.emplace_back(decl_var(declType()));
        }else{
            stmts.emplace_back(parse_stmt());
        }
    }
    tkskip(token_t::T_close_block,"a block-statament expect '}'");
    return std::make_unique<blockStmt>(stmts);
}


std::unique_ptr<Stmt> ret_stmt() {
    next_token();
    std::unique_ptr<Stmt> s = expr_stmt();
    return std::make_unique<retStmt>(s);
}


std::unique_ptr<Stmt> parse_stmt() {
    if(tkequal(token_t::T_open_block)) {
        scope.enter();
        std::unique_ptr<Stmt> s = block_stmt();
        scope.leave();
        return s;
    } 
    if(cur.type == token_t::T_if) return if_stmt();
    if(cur.type == token_t::T_return) return ret_stmt();
    return expr_stmt();
}


std::vector<std::unique_ptr<Expr>> funcParams(std::string_view name,std::shared_ptr<Type>& retType) {
    bool first = false;
    std::vector<std::unique_ptr<Expr>> params;
    std::vector<std::shared_ptr<Type>> paramTypes;
    while(!tkequal(token_t::T_close_paren)) {
        if(first) {
            tkskip(token_t::T_comma,"expect ','");
        }
        first = true;
        auto type = declType();
        params.push_back(newvar(type));
        paramTypes.push_back(std::move(type));
    }
    tkskip(token_t::T_close_paren,"expect ')'");
    scope.insert_global(name,objFactor::getfuncObj(typeFactor::getFuncType(retType,paramTypes)));
    return params;
}



std::unique_ptr<Stmt> decl_func(std::string_view name,std::shared_ptr<Type> retType) {
    scope.enter();
    std::vector<std::unique_ptr<Expr>> _params = funcParams(name,retType);
    std::unique_ptr<Stmt> body = block_stmt();
    scope.leave();
    auto func = funcdef::newFunction(body,name,_params);
    return func;
}


std::unique_ptr<Stmt> decl_global_var(std::shared_ptr<Type> type) {
    std::vector<std::unique_ptr<Expr>> vars;
    bool first = false;
    while(!tkequal(token_t::T_semicolon)) {
        if(first) {
            tkskip(token_t::T_comma,"expect ','");
        } 
        first = true;
        vars.push_back(newvar(type));
    }
    return std::make_unique<vardef>(vars,true);
}


std::unique_ptr<Stmt> declaration() {
    std::shared_ptr<Type> type = declType();
    std::string_view name = cur.str;
    tkskip(token_t::T_identifier,"expect an identifier name");

    if(tkequal(token_t::T_open_paren)) {
        next_token();
        return decl_func(name,type);
    }
    else{ //define global variables
        lex.back(prev.start);
        next_token();
        return decl_global_var(type);
    }
}


Prog parse(const char *src) {
    lex = lexer(src);
    next_token();
    std::vector<std::unique_ptr<Stmt>> stmts;
    while(!tkequal(token_t::T_eof)) {
        stmts.push_back(declaration());
        while(tkconsume(token_t::T_semicolon));
    }
    return Prog(stmts);
}

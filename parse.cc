#include "include/parse.h"

token prev;             // for pratt prase
token cur;              // for pratt parse
lexer lex;
Scope scope;


#ifdef DEBUG
std::map<token_t,std::string_view> tokenstrs {
    {token_t::T_num,"num"},
    {token_t::T_string,"string"},
    {token_t::T_identifier,"identifier"},
    {token_t::T_keyword,"keyword"},
    {token_t::T_plus,"+"},
    {token_t::T_minus,"-"},
    {token_t::T_star,"*"},
    {token_t::T_div,"/"},
    {token_t::T_open_paren,"("},
    {token_t::T_close_paren,")"},
    {token_t::T_open_square,"["},
    {token_t::T_close_square,"]"},
    {token_t::T_open_block,"{"},
    {token_t::T_close_block,"}"},
    {token_t::T_semicolon,";"},
    {token_t::T_lt,"<"},
    {token_t::T_gt,">"},
    {token_t::T_not,"!"},
    {token_t::T_assign,"="},
    {token_t::T_eq,"=="},
    {token_t::T_le,"<="},
    {token_t::T_ge,">="},
    {token_t::T_neq,"!="},
    {token_t::T_comma,","},
    {token_t::T_period,"."},
    {token_t::T_if,"if"},
    {token_t::T_else,"else"},
    {token_t::T_return,"return"},
    {token_t::T_int,"int"},
    {token_t::T_eof,"eof"},
    {token_t::T_addr,"&"}
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
    if(cur.type == token_t::T_addr && prev.type != token_t::T_assign) {
        cur.type = token_t::T_bit_and;
    }
#ifdef DEBUG
    std::cerr << "token is:" << "'" << tokenstrs[cur.type] << "' -> " << cur.str << "\n";
#endif
}

void error(token& t,std::string_view msg){
    lex.error_at(t.start,t.str.length(),msg);
}

void tkskip(token_t expected,std::string_view msg) {
    if(cur.type != expected) {
        error(cur,msg);
    }
    next_token();
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
    return std::make_unique<numericExpr>(prev.val,typeFactor::getInt());
}


std::unique_ptr<Expr> identifier() {
    auto result = scope.lookup_allscope(prev.str);
    if(!result.has_value()) {
        error(prev,std::format("'{}' not found",prev.str));
    }
    return std::make_unique<identExpr>(prev.str,result.value());
}


std::unique_ptr<Expr> funcall() {
    std::string name = prev.str;
    auto funcobj = scope.lookup_global(name);
    if(!funcobj.has_value()) {
        error(prev,std::format("function '{}' not found",name));
    }
    auto type = static_cast <funcObj*>(funcobj.value().get())->getType();
    next_token();
    std::vector<std::unique_ptr<Expr>> args;
    while(!tkconsume(token_t::T_close_paren)) {
        if(args.size() > 0) 
            tkconsume(token_t::T_comma);
        args.emplace_back(parse_expr(precedence_t::P_none));
    }
    return std::make_unique<funcallExpr>(name,args,type);
}

std::unique_ptr<Expr> parse_ident() {
    if(tkequal(token_t::T_open_paren)) {
        return funcall();
    }
    return identifier();
}


std::unique_ptr<Expr> parse_prefix() {
    std::unique_ptr<Expr> e = parse_expr(precedence_t::P_prefix);
    return std::make_unique<prefixExpr>(e,e->getType());
}

std::unique_ptr<Expr> parse_group_expr() {
    std::unique_ptr<Expr> e = parse_expr(precedence_t::P_none);
    tkskip(token_t::T_close_paren,"expect ')'\n");
    return e;
}

std::unique_ptr<Expr> parse_binary_expr(std::unique_ptr<Expr> &lhs) {
    token_t op = prev.type;
    precedence_t precedence = get_precedence(op);
    std::unique_ptr<Expr> rhs = parse_expr(precedence);
    return std::make_unique<binaryExpr>(op,lhs,rhs,lhs->getType());
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
    tkskip(token_t::T_open_paren,"expect '(' after 'if' ");
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
        return std::make_unique<ifStmt>(_cond,_then,_else);
    }
    return std::make_unique<ifStmt>(_cond,_then,_else);
}


std::unique_ptr<Expr> _var(std::shared_ptr<Type> ty) {
    std::string name = cur.str;
    tkskip(token_t::T_identifier,"expect an identifier");
    if(lex.iskeyword(name)) {
        error(prev,std::format("identifier name couldn't be a keyword:'{}'",name));
    }
    bool isglobal = scope.isglobal();
    int off = isglobal ? 0 : funcdef::newlocalVar(ty->getSize());
    auto obj = isglobal ? objFactor::getglobalObj(ty) : objFactor::getlocalObj(off,ty);
    bool result = isglobal ? scope.insert_global(name,obj) : scope.insert_local(name,obj);
    if(!result){
        error(cur,std::format("redefine '{}'",name));
    }
    auto ident = std::make_unique<identExpr>(name,obj);
    return ident;
}


std::unique_ptr<Stmt> decl_var(std::shared_ptr<Type> type) {
    std::vector<std::unique_ptr<Expr>> vars;
    bool first = false;
    while(!tkconsume(token_t::T_semicolon)) {
        if(first) {
            tkskip(token_t::T_comma,"expect ','");
        }
        first = true;
        std::unique_ptr<Expr> var = _var(type);
        std::unique_ptr<Expr> value;
        if(tkequal(token_t::T_assign)) {
            next_token();
            value = parse_expr(precedence_t::P_none);
        }
        vars.push_back(std::make_unique<binaryExpr>(token_t::T_assign,var,value,type));
    }
    return std::make_unique<vardef>(vars);
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
    tkskip(token_t::T_close_block,"expect '}'");
    return std::make_unique<blockStmt>(stmts);
}

std::unique_ptr<Stmt> ret_stmt() {
    next_token();
    std::unique_ptr<Stmt> s = expr_stmt();
    return std::make_unique<retStmt>(s);
}

std::unique_ptr<Stmt> parse_stmt() {
    if(cur.type == token_t::T_open_block){
        scope.enter();
        std::unique_ptr<Stmt> s = block_stmt();
        scope.leave();
        return s;
    } 
    if(cur.type == token_t::T_if) return if_stmt();
    if(cur.type == token_t::T_return) return ret_stmt();
    return expr_stmt();
}


std::vector<std::unique_ptr<Expr>> funcParams(std::string& name,std::shared_ptr<Type>& retType) {
    bool first = false;
    std::vector<std::unique_ptr<Expr>> params;
    std::vector<std::shared_ptr<Type>> paramTypes;
    while(!tkequal(token_t::T_close_paren)) {
        if(first) {
            tkskip(token_t::T_comma,"expect ','");
        }
        first = true;
        auto type = declType();
        params.push_back(_var(type));
        paramTypes.push_back(std::move(type));
    }
    tkskip(token_t::T_close_paren,"expect ')'");
    scope.insert_global(name,objFactor::getfuncObj(typeFactor::getFuncType(retType,paramTypes)));
    return params;
}


std::unique_ptr<Stmt> decl_func(std::string& name,std::shared_ptr<Type> retType) {
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
        auto ident = _var(type);
        vars.push_back(std::move(ident));
    }
    return std::make_unique<vardef>(vars);
}


std::unique_ptr<Stmt> declaration() {
    std::shared_ptr<Type> type = declType();
    std::string name = cur.str;
    tkskip(token_t::T_identifier,"expect an identifier name");
    if(tkequal(token_t::T_open_paren)) {
        next_token();
        return decl_func(name,type);
    }else{
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

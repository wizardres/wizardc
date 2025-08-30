#include "include/parse.h"

token prev;             // for pratt prase
token cur;              // for pratt parse
lexer lex;
Scope scope;
extern std::unordered_map<std::string_view,token_t> keywords;


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
};
#endif

std::map<token_t,precedence_t> precedence {
    { token_t::T_num,precedence_t::P_none },
    { token_t::T_eof,precedence_t::P_none },
    { token_t::T_identifier,precedence_t::P_none },
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

void token_skip(token_t expected,std::string_view msg) {
    if(cur.type != expected) {
        error(cur,msg);
    }
    next_token();
}

bool token_equal(token_t expect) {
    return cur.type == expect;
}

void token_expect(token_t expect,std::string_view msg) {
    if(cur.type != expect) {
        error(cur,msg);
    }
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

template<typename T>
T get_call(token_t) {

}


std::unique_ptr<Expr> parse_numeric() {
    return std::make_unique<numericExpr>(prev.val,prev);
}


std::unique_ptr<Expr> identifier() {
    auto result = scope.allscope_lookup(prev.str);
    if(!result.has_value()) {
        error(prev,std::format("'{}' not found",prev.str));
    }
    return std::make_unique<identExpr>(result.value(),prev);
}


std::unique_ptr<Expr> funcall() {
    std::string name = prev.str;
    next_token();
    std::vector<std::unique_ptr<Expr>> args;
    while(!token_equal(token_t::T_close_paren)) {
        if(args.size() > 0) {
            if(token_equal(token_t::T_comma)) {
                next_token();
            }
        }
        args.emplace_back(parse_expr(precedence_t::P_none));
    }

    next_token();
    return std::make_unique<funcallExpr>(name,args,cur);
}

std::unique_ptr<Expr> parse_ident() {
    if(cur.type == token_t::T_open_paren) {
        return funcall();
    }
    return identifier();
}


std::unique_ptr<Expr> parse_prefix() {
    std::unique_ptr<Expr> e = parse_expr(precedence_t::P_prefix);
    return std::make_unique<prefixExpr>(e,prev);
}

std::unique_ptr<Expr> parse_group_expr() {
    std::unique_ptr<Expr> e = parse_expr(precedence_t::P_none);
    token_skip(token_t::T_close_paren,"expect ')'\n");
    return e;
}

std::unique_ptr<Expr> parse_binary_expr(std::unique_ptr<Expr> &lhs) {
    token tok = prev;
    precedence_t precedence = get_precedence(tok.type);
    std::unique_ptr<Expr> rhs = parse_expr(precedence);
    return std::make_unique<binaryExpr>(lhs,rhs,tok);
}




std::unique_ptr<Expr> parse_expr(precedence_t prec) {
    next_token();
    auto prefixcall = get_prefix_call(prev.type);
    std::unique_ptr<Expr> left = prefixcall();
    
    while(!token_equal(token_t::T_eof) && prec < get_precedence(cur.type)) {
        auto infixcall = get_infix_call(cur.type);
        next_token();
        left = infixcall(left);
    }
    return left;
}


std::unique_ptr<Stmt> expr_stmt() {
    std::unique_ptr<Expr> e = parse_expr(precedence_t::P_none);
    token_skip(token_t::T_semicolon,"expect ';'");
    return std::make_unique<exprStmt>(e);
}


std::unique_ptr<Stmt> if_stmt() {
    next_token();
    token_skip(token_t::T_open_paren,"expect '(' after 'if' ");
    if(token_equal(token_t::T_close_paren)) {
        error(cur,"expect an expression");
    }
    std::unique_ptr<Expr> _cond = parse_expr(precedence_t::P_none);
    token_skip(token_t::T_close_paren,"expect ')' after condition");
    std::unique_ptr<Stmt> _then = parse_stmt();
    std::unique_ptr<Stmt> _else;
    if(token_equal(token_t::T_else)) {
        next_token();
        _else = parse_stmt();
        return std::make_unique<ifStmt>(_cond,_then,_else);
    }
    return std::make_unique<ifStmt>(_cond,_then,_else);
}


std::unique_ptr<Expr> _var() {
    std::string _name = cur.str;
    if(keywords.find(_name) != keywords.end()) {
        error(cur,std::format("invalid identifier name",_name));
    }
    funcdef::stacksize += 8;
    int off = -funcdef::stacksize;
    auto result = scope.curscope_lookup(_name);
    if(result.has_value()) {
        error(cur,std::format("redefine '{}'",_name));
    }
    scope.insert(_name,off);
    next_token();
    return std::make_unique<identExpr>(off,cur);
}


std::unique_ptr<Stmt> decl_var() {
    next_token();
    std::vector<std::unique_ptr<Expr>> decls;
    int i {0};
    while(!token_equal(token_t::T_semicolon)) {
        if(i++ > 0) {
            token_skip(token_t::T_comma,"expect ','");
        }
        std::unique_ptr<Expr> var = _var();
        std::unique_ptr<Expr> value;
        token tok = cur;
        tok.type = token_t::T_assign;
        if(token_equal(token_t::T_assign)) {
            next_token();
            value = parse_expr(precedence_t::P_none);
        }
        decls.push_back(std::make_unique<binaryExpr>(var,value,tok));
    }
    next_token();
    return std::make_unique<vardef>(decls);
}


std::unique_ptr<Stmt> block_stmt() {
    next_token();
    std::vector<std::unique_ptr<Stmt>> stmts;
    while(!token_equal(token_t::T_eof) && !token_equal(token_t::T_close_block)) {
        if(token_equal(token_t::T_int)) {
            stmts.emplace_back(decl_var());
        }else{
            stmts.emplace_back(parse_stmt());
        }
    }
    token_skip(token_t::T_close_block,"expect '}'");
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


std::vector<std::unique_ptr<Expr>> params() {
    int i = 0;
    std::vector<std::unique_ptr<Expr>> params;
    while(!token_equal(token_t::T_close_paren)) {
        if(i++ > 0) {
            token_skip(token_t::T_comma,"expect ','");
        }
        token_skip(token_t::T_int,"expect 'int'");
        token_expect(token_t::T_identifier,"expect an identifier");
        params.push_back(_var());
    }
    return params;
}


std::unique_ptr<Stmt> declaration() {
    token_skip(token_t::T_int,"expect 'int'");
    token_expect(token_t::T_identifier,"expect an identifier name");
    std::string name = cur.str;
    next_token();
    token_skip(token_t::T_open_paren,"expect '('");
    scope.enter();
    std::vector<std::unique_ptr<Expr>> _params;
    if(!token_equal(token_t::T_close_paren)) {
        _params = params();
    }
    token_skip(token_t::T_close_paren,"expect ')'");
    token_expect(token_t::T_open_block,"expect '{' ");
    std::unique_ptr<Stmt> body = block_stmt();
    scope.leave();
    std::unique_ptr<funcdef> func = std::make_unique<funcdef>(body,name,_params,funcdef::stacksize);
    funcdef::stacksize = 0;
    return func;
}


Prog parse(const char *src) {
    lex = lexer(src);
    next_token();
    Prog prog;
    while(!token_equal(token_t::T_eof)) {
        prog.stmts.push_back(declaration());
    }
    return prog;
}

#include "include/parse.h"

token prev_tok;             // for pratt prase
token cur_tok;              // for pratt parse
lexer lex;

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
    prev_tok = cur_tok;
    cur_tok = lex.newToken();
#ifdef DEBUG
    std::cerr << "token is:" << "'" << tokenstrs[cur_tok.type] << "'\n";
#endif
}

void token_expected(token_t expected,const char *msg) {
    if(cur_tok.type != expected) {
        lex.error_at(cur_tok.start,cur_tok.str.length(),msg);
    }
    next_token();
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
        if(prev_tok.type == token_t::T_eof){
            lex.error_at(prev_tok.start,prev_tok.str.length(),"expect an prefix expression");
        }else{
            lex.error_at(prev_tok.start,prev_tok.str.length(),std::format("invalid prefix '{}'",prev_tok.str));
        }
    }
    return it->second;
}

infix_call get_infix_call(token_t t) {
    auto it = infixcalls.find(t);
    if(it == infixcalls.end()) {
        std::string msg;
        if(prev_tok.type == token_t::T_eof){
            lex.error_at(prev_tok.start,prev_tok.str.length(),"expect an infix expression");
        }else{
            lex.error_at(prev_tok.start,prev_tok.str.length(),std::format("invalid infix '{}'",prev_tok.str));
        }
    }
    return it->second;
}


std::unique_ptr<Expr> parse_numeric() {
    return std::make_unique<numericExpr>(prev_tok.val,prev_tok);
}

std::unique_ptr<Expr> identifier() {
    auto vars = identExpr::local_vars;
    auto it = vars.find(prev_tok.str);
    if(it == vars.end()) {
        lex.error_at(prev_tok.start,prev_tok.str.length(),std::format("'{}' not found",prev_tok.str));
    }
    return std::make_unique<identExpr>(it->second,prev_tok);
}


std::unique_ptr<Expr> funcall() {
    std::string_view name = prev_tok.str;
    next_token();
    token_expected(token_t::T_close_paren,"expect ')' in function-call expresstion");
    return std::make_unique<funcallExpr>(name,cur_tok);
}

std::unique_ptr<Expr> parse_ident() {
    if(cur_tok.type == token_t::T_open_paren) {
        return funcall();
    }
    return identifier();
}


std::unique_ptr<Expr> parse_prefix() {
    std::unique_ptr<Expr> e = parse_expr(precedence_t::P_prefix);
    return std::make_unique<prefixExpr>(e,prev_tok);
}

std::unique_ptr<Expr> parse_group_expr() {
    std::unique_ptr<Expr> e = parse_expr(precedence_t::P_none);
    token_expected(token_t::T_close_paren,"expect ')'\n");
    return e;
}

std::unique_ptr<Expr> parse_binary_expr(std::unique_ptr<Expr> &lhs) {
    token tok = prev_tok;
    precedence_t precedence = get_precedence(tok.type);
    std::unique_ptr<Expr> rhs = parse_expr(precedence);
    return std::make_unique<binaryExpr>(lhs,rhs,tok);
}




std::unique_ptr<Expr> parse_expr(precedence_t prec) {
    next_token();
    auto prefixcall = get_prefix_call(prev_tok.type);
    std::unique_ptr<Expr> left = prefixcall();
    
    while(cur_tok.type != token_t::T_eof && prec < get_precedence(cur_tok.type)) {
        auto infixcall = get_infix_call(cur_tok.type);
        next_token();
        left = infixcall(left);
    }
    return left;
}


std::unique_ptr<Stmt> expr_stmt() {
    std::unique_ptr<Expr> e = parse_expr(precedence_t::P_none);
    token_expected(token_t::T_semicolon,"expect ';'");
    return std::make_unique<exprStmt>(e);
}

std::unique_ptr<Stmt> block_stmt() {
    next_token();
    std::vector<std::unique_ptr<Stmt>> stmts;
    while(cur_tok.type != token_t::T_eof && cur_tok.type != token_t::T_close_block) {
        stmts.emplace_back(parse_stmt());
    }
    token_expected(token_t::T_close_block,"expect '}'");
    return std::make_unique<blockStmt>(stmts);
}

std::unique_ptr<Stmt> if_stmt() {
    next_token();
    token_expected(token_t::T_open_paren,"expect '(' after 'if' ");
    if(cur_tok.type == token_t::T_close_paren) {
        lex.error_at(prev_tok.start,prev_tok.str.length(),"condition can't be empty");
    }
    std::unique_ptr<Expr> _cond = parse_expr(precedence_t::P_none);
    token_expected(token_t::T_close_paren,"expect ')' after condition");
    std::unique_ptr<Stmt> _then = parse_stmt();
    std::unique_ptr<Stmt> _else;
    if(cur_tok.type == token_t::T_else && cur_tok.str == "else") {
        next_token();
        _else = parse_stmt();
        return std::make_unique<ifStmt>(_cond,_then,_else);
    }
    return std::make_unique<ifStmt>(_cond,_then,_else);
}

std::unique_ptr<Expr> decl_var() {
    std::string str = cur_tok.str;
    token_expected(token_t::T_identifier,"expect a variable name");
    identExpr::var_offset -= 8;
    identExpr::local_vars.insert({str,identExpr::var_offset});
    return std::make_unique<identExpr>(identExpr::var_offset,cur_tok);
}


std::unique_ptr<Stmt> decl_stmt() {
    next_token();
    std::vector<std::unique_ptr<Expr>> decls;
    int i {0};
    while(cur_tok.type != token_t::T_semicolon) {
        if(i++ > 0) {
            token_expected(token_t::T_comma,"expect ','");
        }
        std::unique_ptr<Expr> var = decl_var();
        token tok = cur_tok;
        if(tok.type == token_t::T_assign) {
            next_token();
            std::unique_ptr<Expr> value = parse_expr(precedence_t::P_none);
            std::unique_ptr<binaryExpr> equ = std::make_unique<binaryExpr>(var,value,tok);
            decls.emplace_back(std::move(equ));
        }else if(cur_tok.type == token_t::T_comma || cur_tok.type == token_t::T_semicolon) {
            std::unique_ptr<Expr> value = std::make_unique<numericExpr>(0,cur_tok);
            std::unique_ptr<binaryExpr> equ = std::make_unique<binaryExpr>(var,value,tok);
            decls.emplace_back(std::move(equ));
            if(cur_tok.type == token_t::T_comma) next_token();
        }
    }
    next_token();
    return std::make_unique<declStmt>(decls);
}

std::unique_ptr<Stmt> parse_stmt() {
    switch(cur_tok.type) {
        case token_t::T_open_block: {
            return block_stmt();
        }
        case token_t::T_if: {
            return if_stmt();
        }
        case token_t::T_int: {
            return decl_stmt();
        }
        case token_t::T_return: {
            next_token();
            return expr_stmt();
        }
        default: {
            return expr_stmt();
        }
    }
}

std::unique_ptr<Stmt> parse(const char *src) {
    lex = lexer(src);
    next_token();
    std::unique_ptr<Stmt> stmt = parse_stmt();
    return stmt;
}

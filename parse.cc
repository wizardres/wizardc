#include "include/parse.h"

const char *src;    // source input                
std::string_view tok_str;     // storing string if token is T_stirng              
int tok_value;                // storing numeric value if token is T_numeric            
token_t prev_tok;             // for pratt prase
token_t cur_tok;              // for pratt parse
int prev_start{0};            
int start{0};
int cur{0};       

std::unordered_map<std::string_view,token_t> keywords = {
    { "if",token_t::T_if },
    { "else",token_t::T_else },
    { "int",token_t::T_int },
};


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

char advance() {
    return src[cur++];
}

char peek() {
    return src[cur];
}

void skip_blank() {
    while(is_blank(peek())) advance();
}

void error_at(int blanklen,int hintlen,std::string_view msg) {
    std::cerr << std::format("error: {}\n{}{} {}\n",src,std::string(blanklen+7,' '),std::string(hintlen,'^'),msg);
    exit(-1);
}


token_t token_number(char c) {
    int value = 0;
    int num = 0;
    if(c == '0' && peek() == 'x') {
        advance();
        c = advance();
        if(!is_hex_num(c)) {
            error_at(cur-1,1,std::format("invalid hex-number character '{}'",c));
        }
        do {
            if(is_number(c)) num = c - '0';
            else if(c >= 'a' && c <= 'f') num = (c - 'a' + 10);
            else num = (c - 'A' + 10);
            value = value*16 + num;
            c = advance();
        }while(is_hex_num(c));
        tok_value = value;
    }else {
        do {
            num = num * 10 + (c - '0');
            c = advance();
        }while(is_number(c));
        tok_value = num;
    }
    cur--;
    return token_t::T_num;
}


token_t token_identifier(char c) {
    while(is_identifier(peek()) || is_number(peek())) advance();
    tok_str = std::string_view(src+start,src+cur);

    auto it = keywords.find(tok_str);
    if(it != keywords.end()) {
        return it->second;
    }
    return token_t::T_identifier;
}


token_t token_operator(char c) {
    char cc = peek();
    token_t token;
    switch(c) {
        case '+': token = token_t::T_plus;break;
        case '-': token = token_t::T_minus;break;
        case '*': token = token_t::T_star;break;
        case '/': token = token_t::T_div;break;
        case '<': token = cc == '=' ? token_t::T_le : token_t::T_lt;break;
        case '>': token = cc == '=' ? token_t::T_ge : token_t::T_gt;break;
        case '=': token = cc == '=' ? token_t::T_eq : token_t::T_assign;break;
        case '!': token = cc == '=' ? token_t::T_neq : token_t::T_not;break;
    }
    switch(token) {
        case token_t::T_le:
        case token_t::T_ge:
        case token_t::T_eq:
        case token_t::T_neq:
        advance();
    }
    return token;
}

token_t token_bracket(char c) {
    switch(c) {
        case '(': return token_t::T_open_paren;
        case ')': return token_t::T_close_paren;
        case '{': return token_t::T_open_block;
        case '}': return token_t::T_close_block;
        case '[': return token_t::T_open_square;
        case ']': return token_t::T_close_square;
    }
}

token_t token_puct(char c) {
    switch(c) {
        case ',': return token_t::T_comma;
        case ';': return token_t::T_semicolon;
        case '.': return token_t::T_period;
    }
}

token_t scan_token() {
    skip_blank();
    prev_start = start;
    start = cur;
    char c = advance();

    if(is_number(c)) {
       return token_number(c);
    }else if(is_identifier(c)) {
       return token_identifier(c);
    }else if(is_bracket(c)) {
        return token_bracket(c);
    }else if(is_eof(c)) {
        return token_t::T_eof;
    }else if(is_operator(c)){
        return token_operator(c);
    }else if(is_puct(c)){
        return token_puct(c);
    }else{
        error_at(start,1,std::format("invalid character:'{}'",c));
    }
}


void next_token() {
    prev_tok = cur_tok;
    cur_tok = scan_token();
}

void token_expected(token_t expected,const char *msg) {
    if(cur_tok != expected) {
        error_at(prev_start+1,start-prev_start,std::format("{}",msg));
    }
    next_token();
}

precedence_t get_precedence(token_t token) {
    auto it = precedence.find(token);
    if(it != precedence.end()) {
        return it->second;
    }
    return precedence_t::P_none;
}

prefix_call get_prefix_call(token_t t) {
    auto it = prefixcalls.find(t);
    if(it == prefixcalls.end()) {
        std::string msg;
        if(prev_tok == token_t::T_eof){
            msg = "expect an expression";
        }else{
            msg = "invalid '" + std::string(src+prev_start,src+start) + "'";
        }
        error_at(prev_start,start-prev_start,std::format("{}",msg));
    }
    return it->second;
}

infix_call get_infix_call(token_t t) {
    auto it = infixcalls.find(t);
    if(it == infixcalls.end()) {
        std::string msg;
        if(prev_tok == token_t::T_eof){
            msg = "expect an expression";
        }else{
            msg = "invalid '" + std::string(src+prev_start,src+start) + "'";
        }
        error_at(prev_start,start-prev_start,std::format("{}",msg));
    }
    return it->second;
}


std::unique_ptr<Expr> parse_numeric() {
    return std::make_unique<numericExpr>(tok_value,prev_start,start-prev_start);
}

std::unique_ptr<Expr> parse_ident() {
    auto &locals = identExpr::local_vars;
    int &cur_offset = identExpr::var_offset;

    auto it = locals.find(tok_str);
    if(it == locals.end()) {
        error_at(prev_start,start-prev_start,"undefined variable");
    }
    return std::make_unique<identExpr>(it->second,prev_start,cur-start);
}

std::unique_ptr<Expr> parse_prefix() {
    token_t op = prev_tok;
    std::unique_ptr<Expr> e = parse_expr(precedence_t::P_prefix);
    return std::make_unique<prefixExpr>(e,op,prev_start,start-prev_start);
}

std::unique_ptr<Expr> parse_group_expr() {
    std::unique_ptr<Expr> e = parse_expr(precedence_t::P_none);
    token_expected(token_t::T_close_paren,"expect ')'\n");
    return e;
}

std::unique_ptr<Expr> parse_binary_expr(std::unique_ptr<Expr> &lhs) {
    token_t op = prev_tok;
    auto precedence = get_precedence(prev_tok);
    std::unique_ptr<Expr> rhs = parse_expr(precedence);
    return std::make_unique<binaryExpr>(lhs,rhs,op,prev_start,start-prev_start);
}


std::unique_ptr<Expr> parse_expr(precedence_t prec) {
    next_token();
    auto prefixcall = get_prefix_call(prev_tok);
    std::unique_ptr<Expr> left = prefixcall();
    
    while(cur_tok != token_t::T_eof && prec < get_precedence(cur_tok)) {
        auto infixcall = get_infix_call(cur_tok);
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
    while(cur_tok != token_t::T_eof && cur_tok != token_t::T_close_block) {
        stmts.emplace_back(parse_stmt());
    }
    token_expected(token_t::T_close_block,"expect '}'");
    return std::make_unique<blockStmt>(stmts);
}

std::unique_ptr<Stmt> if_stmt() {
    next_token();
    token_expected(token_t::T_open_paren,"expect '(' after 'if' ");
    if(cur_tok == token_t::T_close_paren) {
        error_at(prev_start,start-prev_start+1,"condition can't be empty");
    }
    std::unique_ptr<Expr> _cond = parse_expr(precedence_t::P_none);
    token_expected(token_t::T_close_paren,"expect ')' after condition");
    std::unique_ptr<Stmt> _then = parse_stmt();
    std::unique_ptr<Stmt> _else;
    if(cur_tok == token_t::T_else && tok_str == "else") {
        next_token();
        _else = parse_stmt();
        return std::make_unique<ifStmt>(_cond,_then,_else);
    }
    return std::make_unique<ifStmt>(_cond,_then,_else);
}

std::unique_ptr<Expr> decl_var() {
    std::string_view str = tok_str;
    token_expected(token_t::T_identifier,"expect a variable name");
    identExpr::var_offset -= 8;
    identExpr::local_vars.insert({str,identExpr::var_offset});
    return std::make_unique<identExpr>(identExpr::var_offset,prev_start,start-prev_start);
}


std::unique_ptr<Stmt> decl_stmt() {
    next_token();
    std::vector<std::unique_ptr<Expr>> decls;
    int i {0};
    while(cur_tok != token_t::T_semicolon) {
        if(i++ > 0) {
            token_expected(token_t::T_comma,"expect ','");
        }
        std::unique_ptr<Expr> var = decl_var();
        if(cur_tok == token_t::T_assign) {
            next_token();
            int tok_start = start,tok_len = cur-start;
            std::unique_ptr<Expr> value = parse_expr(precedence_t::P_none);
            std::unique_ptr<binaryExpr> equ = std::make_unique<binaryExpr>(var,value,token_t::T_assign,tok_start,tok_len);
            decls.emplace_back(std::move(equ));
        }else if(cur_tok == token_t::T_comma || cur_tok == token_t::T_semicolon) {
            std::unique_ptr<Expr> value = std::make_unique<numericExpr>(0,prev_start,start-prev_start);
            std::unique_ptr<binaryExpr> equ = std::make_unique<binaryExpr>(var,value,token_t::T_assign,prev_start,start-prev_start);
            decls.emplace_back(std::move(equ));
            if(cur_tok == token_t::T_comma) next_token();
        }
    }
    next_token();
    return std::make_unique<declStmt>(decls);
}

std::unique_ptr<Stmt> parse_stmt() {
    switch(cur_tok) {
        case token_t::T_open_block: {
            return block_stmt();
        }
        case token_t::T_if: {
            return if_stmt();
        }
        case token_t::T_int: {
            return decl_stmt();
        }
        default: {
            return expr_stmt();
        }
    }
}

std::unique_ptr<Stmt> parse() {
    next_token();
    std::unique_ptr<Stmt> stmt = parse_stmt();
    return stmt;
}

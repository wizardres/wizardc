#include "include/parse.h"


const char *src {nullptr};                    /* input string */
std::string_view tok_str;                    /* "tok_str" is the tokenized string when token type is 'T_string' */
int tok_value;                              /* "tok_value" point to tokenized numeric value if token type is 'T_num' */
token_t prev_tok,cur_tok;                  /* these two variables are for "pratt parse" */
int prev_start{0},start{0},cur {0};       /* these three variables point to begining and ending of word that was tokenized */

char advance() {
    return src[cur++];
}

char peek() {
    return src[cur];
}

char peek1() {
    return src[cur+1];
}

void skip_blank() {
    char c = advance();
    while(is_blank(c)) c = advance();
    cur--;
}

void error_at(int blen,int len,std::string_view msg) {
    std::cerr << std::format("error: {}\n{}{} {}\n",src,std::string(blen+7,' '),std::string(len,'^'),msg);
    exit(-1);
}


token_t token_number(char c) {
    int value = 0;
    int num = 0;
    if(c == '0' && peek() == 'x') {
        cur++;
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

token_t scan_token() {
    skip_blank();
    prev_start = start;
    start = cur;
    char c = advance();
    if(is_number(c)) {
       return token_number(c);
    }else if(is_identifier(c)) {
       return token_identifier(c);
    }else if(is_open_paren(c)) {
        return token_t::T_open_paren;
    }else if(is_close_paren(c)) {
        return token_t::T_close_paren;
    }else if(is_eof(c)) {
        return token_t::T_eof;
    }else if(is_operator(c)){
        return token_operator(c);
    }else if(is_semicolon(c)){
        return token_t::T_semicolon;
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

std::map<token_t,precedence_t> precedence {
    { token_t::T_num,precedence_t::P_none },
    { token_t::T_eof,precedence_t::P_none },
    { token_t::T_identifier,precedence_t::P_none },
    { token_t::T_plus,precedence_t::P_factor },
    { token_t::T_minus,precedence_t::P_factor },
    { token_t::T_star,precedence_t::P_term },
    { token_t::T_div,precedence_t::P_term },
    { token_t::T_lt,precedence_t::P_comparison },
    { token_t::T_le,precedence_t::P_comparison },
    { token_t::T_gt,precedence_t::P_comparison },
    { token_t::T_ge,precedence_t::P_comparison },
    { token_t::T_neq,precedence_t::P_comparison },
    { token_t::T_eq,precedence_t::P_comparison },
};

std::map<token_t,prefix_call> prefixcalls {
    { token_t::T_num,parse_numeric },
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
};

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
    Expr *n = new numericExpr{ tok_value };
    return std::unique_ptr<Expr>(n);
}

std::unique_ptr<Expr> parse_prefix() {
    token_t op = prev_tok;
    std::unique_ptr<Expr> e = parse_expr(precedence_t::P_prefix);
    Expr *pe = new prefixExpr{ e,op};
    return std::unique_ptr<Expr>(pe);
}

std::unique_ptr<Expr> parse_group_expr() {
    std::unique_ptr<Expr> e = parse_expr(precedence_t::P_none);
    token_expected(token_t::T_close_paren,"expect ')'\n");
    return e;
}

std::unique_ptr<Expr> parse_binary_expr(std::unique_ptr<Expr> &lhs) {
    token_t op = cur_tok;
    auto precedence = get_precedence(cur_tok);
    next_token();
    std::unique_ptr<Expr> rhs = parse_expr(precedence);
    Expr *e = new binaryExpr{ lhs,rhs,op };
    return std::unique_ptr<Expr>(e);
}


std::unique_ptr<Expr> parse_expr(precedence_t prec) {
    next_token();
    auto prefixcall = get_prefix_call(prev_tok);
    std::unique_ptr<Expr> left = prefixcall();
    
    while(cur_tok != token_t::T_eof && prec < get_precedence(cur_tok)) {
        auto infixcall = get_infix_call(cur_tok);
        left = infixcall(left);
    }
    return left;
}

std::unique_ptr<Expr> expr_stmt() {
    std::unique_ptr<Expr> e = parse_expr(precedence_t::P_none);
    token_expected(token_t::T_semicolon,"expect ';'");
    return e;
}

std::unique_ptr<Expr> parse() {
#ifdef DEBUG
    std::cerr << "get tokens:\n";
#endif
    next_token();
    return expr_stmt();
}



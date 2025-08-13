#include "./include/lexer.h"

std::unordered_map<std::string_view,token_t> keywords = {
    { "if",token_t::T_if },
    { "else",token_t::T_else },
    { "int",token_t::T_int },
    { "return",token_t::T_return },
};

void lexer::error_at(int start,int hintlen,std::string_view errmsg) {
    std::cerr << std::format("error:{}\n",src);
    std::cerr << std::format("{}{} {}\n",std::string(start+6,' '),std::string(hintlen,'^'),errmsg);
    exit(-1);
}

token lexer::identifier() {
    while(is_identifier(peek()) || is_number(peek())) advance();
    
    token_t type = token_t::T_identifier;
    int len = cur-start;
    std::string s{src.substr(start,len)};
    auto it = keywords.find(s);
    if(it != keywords.end()) {
        type = it->second;
    }
    return token(0,start,s,type);
}

token lexer::number() {
    char c = peek();
    int start1;
    int num = 0;
    if(c == '0' && peeknext() == 'x') {
        cur += 2;
        while(is_hex_num(peek())) {
            c = peek();
            if(c >= '0' && c <= '9') {
                num = num * 16 + c - '0';
            }else if(c >= 'a' && c <= 'z'){
                num = num * 16 + c - 'a';
            }else{
                num = num * 16 + c - 'A';
            }
            advance();
        }
        if(is_alpha(peek())) {
            start1 = cur;
            while(is_alpha(peek()) || is_number(peek())) advance();
            error_at(start1,cur-start1,std::format("invalid suffix '{}' on integer constant",src.substr(start1,cur-start1)));
        }
    }else {
        while(is_number(peek())) {
            c = peek();
            num = num * 10 + c - '0';
            advance();
        }
        if(is_alpha(peek())) {
            start1 = cur;
            while(is_alpha(peek())) advance();
            error_at(start1,cur-start1,std::format("invalid suffix '{}' on integer constant",src.substr(start1,cur-start1)));
        }
    }
    return token(num,start,src.substr(start,cur-start),token_t::T_num);
}

token lexer::operator_sign() {
    char cc = peeknext();
    token_t type;
    switch(peek()) {
        case '+': type = token_t::T_plus;break;
        case '-': type = token_t::T_minus;break;
        case '*': type = token_t::T_star;break;
        case '/': type = token_t::T_div;break;
        case '<': type = cc == '=' ? token_t::T_le : token_t::T_lt;break;
        case '>': type = cc == '=' ? token_t::T_ge : token_t::T_gt;break;
        case '=': type = cc == '=' ? token_t::T_eq : token_t::T_assign;break;
        case '!': type = cc == '=' ? token_t::T_neq : token_t::T_not;break;
    }
    advance();
    switch(type) {
        case token_t::T_le:
        case token_t::T_ge:
        case token_t::T_eq:
        case token_t::T_neq:
        advance();
    }
    return token(0,start,src.substr(start,cur-start),type);
}

token lexer::bracket() {
    token_t type;
    switch(peek()) {
        case '(': type = token_t::T_open_paren;break;
        case ')': type = token_t::T_close_paren;break;
        case '{': type = token_t::T_open_block;break;
        case '}': type = token_t::T_close_block;break;
        case '[': type = token_t::T_open_square;break;
        case ']': type = token_t::T_close_square;break;
    }
    advance();
    return token(0,start,src.substr(start,cur-start),type);
}

token lexer::puct() {
    token_t type;
    switch(peek()) {
        case ',': type = token_t::T_comma;break;
        case ';': type = token_t::T_semicolon;break;
        case '.': type = token_t::T_period;break;
    }
    advance();
    return token(0,start,src.substr(start,1),type);
}

token lexer::eof() {
    return token(0,start," ",token_t::T_eof);
}

token lexer::newToken() {
    skip_blank();
    char c = peek();
    start = cur;

    if(is_number(c)) {
       return number();
    }else if(is_identifier(c)) {
       return identifier();
    }else if(is_bracket(c)) {
        return bracket();
    }else if(is_eof(c)){
        return eof();
    }else if(is_operator(c)){
        return operator_sign();
    }else if(is_puct(c)){
        return puct();
    }else{
        error_at(start,1,std::format("unrecongenized character:{}",peek()));
    }
}


void lexer::advance() {
    cur++;
}

char lexer::peek() {
    return src[cur];
}
char lexer::peeknext() {
    return src[cur+1];
}


void lexer::skip_blank() {
    while(is_blank(peek())) advance();
}


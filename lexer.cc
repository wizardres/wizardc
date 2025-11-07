#include "./include/lexer.h"

std::unordered_map<std::string,tokenType> keywords = {
    { "if",tokenType::T_if },
    { "else",tokenType::T_else },
    { "int",tokenType::T_int },
    { "return",tokenType::T_return },
    { "char",tokenType::T_char },
    { "while",tokenType::T_while },
};

void lexer::error_at(int start,int hintlen,std::string_view errmsg) {
    std::cerr << std::format("error:{}\n",src);
    std::cerr << std::format("{}{} {}\n",std::string(start+6,' '),std::string(hintlen,'^'),errmsg);
    exit(-1);
}

token lexer::identifier() {
    while(is_identifier(peek()) || is_number(peek())) advance();
    
    tokenType type = tokenType::T_identifier;
    const std::string& str = src.substr(start,cur-start);
    auto it = keywords.find(str);
    if(it != keywords.end()) {
        type = it->second;
    }
    return token(0,start,src.substr(start,cur-start),type);
}

token lexer::string() {
    advance();
    while(!is_eof(peek()) && !is_string(peek())) advance();
    if(is_eof(peek())) {
        error_at(start,cur-start,"expect \"");
    }
    advance();
    return token(0,start,src.substr(start+1,cur-start-2),tokenType::T_string);
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
    return token(num,start,src.substr(start,cur-start),tokenType::T_num);
}

token lexer::operator_sign() {
    char cc = peeknext();
    tokenType type;
    switch(peek()) {
        case '+': type = tokenType::T_plus;break;
        case '-': type = tokenType::T_minus;break;
        case '*': type = tokenType::T_star;break;
        case '/': type = tokenType::T_div;break;
        case '<': type = cc == '=' ? tokenType::T_le : tokenType::T_lt;break;
        case '>': type = cc == '=' ? tokenType::T_ge : tokenType::T_gt;break;
        case '=': type = cc == '=' ? tokenType::T_eq : tokenType::T_assign;break;
        case '!': type = cc == '=' ? tokenType::T_neq : tokenType::T_not;break;
        default:{
            std::cerr << "invalid arithmetic operator:" << cc << "\n";
            exit(-1);
        }
    }
    advance();
    switch(type) {
        case tokenType::T_le:
        case tokenType::T_ge:
        case tokenType::T_eq:
        case tokenType::T_neq: advance();
        default:break;
    }
    return token(0,start,src.substr(start,cur-start),type);
}

token lexer::bracket() {
    tokenType type;
    switch(peek()) {
        case '(': type = tokenType::T_open_paren;break;
        case ')': type = tokenType::T_close_paren;break;
        case '{': type = tokenType::T_open_block;break;
        case '}': type = tokenType::T_close_block;break;
        case '[': type = tokenType::T_open_square;break;
        case ']': type = tokenType::T_close_square;break;
    }
    advance();
    return token(0,start,src.substr(start,cur-start),type);
}

token lexer::puct() {
    tokenType type;
    switch(peek()) {
        case ',': type = tokenType::T_comma;break;
        case ';': type = tokenType::T_semicolon;break;
        case '.': type = tokenType::T_period;break;
        case '&': type = tokenType::T_addr;break;
    }
    advance();
    return token(0,start,src.substr(start,1),type);
}

token lexer::eof() {
    return token(0,start,src.substr(start,1),tokenType::T_eof);
}

token lexer::newToken() {
    skip_blank();
    char c = peek();
    start = cur;

    if(is_number(c))     return number(); 
    if(is_identifier(c)) return identifier(); 
    if(is_string(c))     return string();
    if(is_bracket(c))    return bracket(); 
    if(is_eof(c))        return eof(); 
    if(is_operator(c))   return operator_sign(); 
    if(is_puct(c))       return puct(); 
    error_at(start,1,std::format("unrecongenized character:{}",peek()));
}


bool lexer::iskeyword(const std::string& name) {
    auto it = keywords.find(name);
    return it != keywords.end();
}

void lexer::advance() {
    if(cur < src.size()) cur++;
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


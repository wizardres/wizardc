#ifndef LEXER_H_
#define LEXER_H_

#include <vector>
#include <iostream>
#include <format>
#include <unordered_map>
#include <map>

enum class tokenType {
    T_num,
    T_string,
    T_identifier,
    T_keyword,
    T_plus,
    T_minus,
    T_star, // '*'
    T_div,
    T_open_paren,   /* '(' */
    T_close_paren,  /* ')' */
    T_open_square,   /* '[' */
    T_close_square,  /* ']' */
    T_open_block,   /* '{' */
    T_close_block,  /* '}' */
    T_semicolon,    /* ';' */
    T_lt,           /* '<' */
    T_gt,           /* '>' */
    T_not,          /* '!' */
    T_assign,       /* '=' */
    T_eq,           /* '=='*/
    T_le,           /* '<='*/
    T_ge,           /* '>='*/
    T_neq,          /* '!='*/
    T_comma,        /* ',' */
    T_period,       /* '.' */
    T_addr,         /* '&' */
    T_bit_and,

    T_if,
    T_else,
    T_return,
    T_int,
    T_char,
    T_eof,
};

#define is_alpha(c) ( (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') )
#define is_identifier(c) (is_alpha(c) || c == '_')
#define is_number(c) ( c >= '0' && c <= '9' )
#define is_bracket(c) ( c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' )
#define is_blank(c) (c == '\n' || c == '\t' || c == '\r' || c == ' ')
#define is_eof(c) (c == '\0')
#define is_semicolon(c) (c == ';')
#define is_hex_num(c) ( (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') || is_number(c) )
#define is_operator(c) ( c == '+' || c == '-' || c == '*' || c == '/' || c == '=' || c == '<' || c == '>' || c == '!') 
#define is_puct(c) ( c == ';' || c == ',' || c == '.' || c == '&' )
#define is_string(c) ( c == '"' )


struct token {
    token()=default;
    token(int value,int st,const std::string& s,tokenType t): val(value), start(st), str(s), type(t){}
    bool assert(tokenType ty)const { return type == ty;}
    int val;
    int start;
    std::string str;
    tokenType type;
};


class lexer {
public:
    lexer()=default;
    lexer(std::string_view str):src(str) {}
    token newToken();

    void advance();
    char peek();
    char peeknext();
    void skip_blank();

    token identifier();
    token number();
    token operator_sign();
    token bracket();
    token puct();
    token eof();
    token string();
    bool iskeyword(const std::string& name);

    void error_at(int start,int hintlen,std::string_view errmsg);
private:
    token scan();

    size_t start{0};
    size_t cur{0};
    std::string src;
};
#endif
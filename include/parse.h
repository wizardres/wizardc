#ifndef PARSE_H_
#define PARSE_H_

#include "visitor.h"
#include "lexer.h"
#include "scope.h"
#include "type.h"
#include "ast.h"

#include <iostream>
#include <string>
#include <string_view>
#include <map>
#include <unordered_set>
#include <memory>
#include <functional>
#include <format>
#include <vector>
#include <array>


enum class precType {
    P_none,
    P_atom,   /* numeric,identifier */
    P_assign, // variable assignment
    P_comparison,  /* '>' '<' '!=' '==' '<=' '>=' */
    P_bit,         /* '&' '|' '^' */
    P_factor, /* '+'  '-'  */
    P_term,   /* '*' '/' */
    P_prefix, /* -1,-2,'-' as prefix*/
};

class Parser {
public:
    Parser(const char *src);
    ~Parser()=default;
    Prog start();
    using prefixcall = std::function<std::unique_ptr<Node>()>;
    using infixcall = std::function<std::unique_ptr<Node>(std::unique_ptr<Node>& )>;
private:
    void setup();
private:
    std::unique_ptr<Node> parse_numeric();
    std::unique_ptr<Node> parse_ident();
    std::unique_ptr<Node> parse_prefix();
    std::unique_ptr<Node> parse_binary_expr(std::unique_ptr<Node> lhs);
    std::unique_ptr<Node> parse_group_expr();
    std::unique_ptr<Node> parse_expr(precType);

    std::unique_ptr<Stmt> parse_stmt();
    std::unique_ptr<Stmt> ret_stmt();
    std::unique_ptr<Stmt> block_stmt();
    std::unique_ptr<Stmt> if_stmt();
    std::unique_ptr<Stmt> expr_stmt();
    
    std::unique_ptr<Stmt> local_vars(std::shared_ptr<Type> type);
    std::unique_ptr<Stmt> global_vars(std::shared_ptr<Type> type);
    std::unique_ptr<Stmt> decl_func(std::shared_ptr<Type> retType);
    std::vector<std::unique_ptr<Node>> funcParams(const token& tok,std::shared_ptr<Type>& retType);
    std::unique_ptr<Node> funcall();
    std::unique_ptr<Node> identifier();
    std::unique_ptr<Node> arrayvisit();

    std::unique_ptr<Node> var_init(std::shared_ptr<Obj> obj);
    std::unique_ptr<Node> array_init(std::shared_ptr<Obj> obj);
private:

    std::shared_ptr<Type> declType();
    std::shared_ptr<Type> declspec();
    std::shared_ptr<Type> pointerPrefix(std::shared_ptr<Type> base);
    std::shared_ptr<Obj> varTypeSuffix(std::shared_ptr<Type> type);
    
    void error(const token& t,std::string_view msg);
    void error(int start,int hint_len,std::string_view msg);
    void keywordCheck(const token& tok,std::string_view name);

    void tkskip(tokenType expected,std::string_view msg);
    bool tkequal(tokenType expect);
    void tkexpect(tokenType expect,std::string_view msg);
    bool tkconsume(tokenType expect);
    void tokenMove();
    void tokenBack();
    
    const token& curToken();
    const token& prevToken();

    bool is_array();
    bool is_function();
private:
    lexer lex;
    int prev{-1};
    int cur{-1};
    std::vector<token> tokens;

    std::map<tokenType, std::function<std::unique_ptr<Node>()>> prefixcalls;
    std::map<tokenType, std::function<std::unique_ptr<Node>(std::unique_ptr<Node>& )>> infixcalls;
    infixcall get_infix_call(tokenType t);
    prefixcall get_prefix_call(tokenType t);
};
#endif
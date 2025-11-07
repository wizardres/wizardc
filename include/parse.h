#ifndef PARSE_H_
#define PARSE_H_

#include "visitor.h"
#include "lexer.h"
#include "scope.h"
#include "type.h"
#include "ast.h"

#include <iostream>
#include <string>
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
    using prefixcall = std::function<std::shared_ptr<Node>()>;
    using infixcall = std::function<std::shared_ptr<Node>(std::shared_ptr<Node>& )>;
private:
    void setup();
private:
    std::shared_ptr<Node> parse_numeric();
    std::shared_ptr<Node> parse_ident();
    std::shared_ptr<Node> parse_string();
    std::shared_ptr<Node> parse_prefix();
    std::shared_ptr<Node> parse_binary_expr(std::shared_ptr<Node> lhs);
    std::shared_ptr<Node> ptr_add(token& op,std::shared_ptr<Node> lhs,std::shared_ptr<Node> rhs);
    std::shared_ptr<Node> ptr_sub(token& op,std::shared_ptr<Node> lhs,std::shared_ptr<Node> rhs);
    std::shared_ptr<Node> parse_group_expr();
    std::shared_ptr<Node> parse_expr(precType);

    std::shared_ptr<Stmt> parse_stmt();
    std::shared_ptr<Stmt> ret_stmt();
    std::shared_ptr<Stmt> block_stmt();
    std::shared_ptr<Stmt> for_stmt();
    std::shared_ptr<Stmt> init_stmt();
    std::shared_ptr<Stmt> if_stmt();
    std::shared_ptr<Stmt> while_stmt();
    std::shared_ptr<Stmt> expr_stmt();
    
    std::shared_ptr<Stmt> local_vars();
    std::shared_ptr<Stmt> global_vars(std::shared_ptr<Type> type);
    std::shared_ptr<Stmt> decl_func(std::shared_ptr<Type> retType);
    std::vector<std::shared_ptr<Node>> funcParams(const token& tok,std::shared_ptr<Type> retType);
    std::shared_ptr<Node> funcall();
    std::shared_ptr<Node> identifier();
    std::shared_ptr<Node> arrayvisit();

    std::shared_ptr<Node> var_init(std::shared_ptr<Obj> obj);
    std::shared_ptr<Node> array_init(std::shared_ptr<Obj> obj);
private:

    std::shared_ptr<Type> declType();
    std::shared_ptr<Type> declspec();
    std::shared_ptr<Type> pointerPrefix(std::shared_ptr<Type> base);
    std::shared_ptr<Obj> varTypeSuffix(std::shared_ptr<Type> type);
    
    void error(const token& t,std::string_view msg);
    void error(int start,int hint_len,std::string_view msg);
    void keywordCheck(const token& tok,const std::string& name);

    void tkskip(tokenType expected,const std::string& msg);
    bool tkequal(tokenType expect);
    void tkexpect(tokenType expect,const std::string& msg);
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

    std::map<tokenType, std::function<std::shared_ptr<Node>()>> prefixcalls;
    std::map<tokenType, std::function<std::shared_ptr<Node>(std::shared_ptr<Node>& )>> infixcalls;
    infixcall get_infix_call(tokenType t);
    prefixcall get_prefix_call(tokenType t);
};
#endif
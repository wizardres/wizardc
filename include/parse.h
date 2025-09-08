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


enum class precedence_t {
    P_none,
    P_atom,   /* numeric,identifier */
    P_assign, // variable assignment
    P_comparison,  /* '>' '<' '!=' '==' '<=' '>=' */
    P_bit,         /* '&' '|' '^' */
    P_factor, /* '+'  '-'  */
    P_term,   /* '*' '/' */
    P_prefix, /* -1,-2,'-' as prefix*/
};


std::unique_ptr<Expr> parse_numeric();
std::unique_ptr<Expr> parse_ident();
std::unique_ptr<Expr> parse_prefix();
std::unique_ptr<Expr> parse_binary_expr(std::unique_ptr<Expr>& lhs);
std::unique_ptr<Expr> parse_group_expr();
std::unique_ptr<Expr> parse_expr(precedence_t );

std::unique_ptr<Stmt> parse_stmt();
std::unique_ptr<Stmt> if_stmt();
std::unique_ptr<Stmt> decl_stmt();

using  prefix_call = std::function<std::unique_ptr<Expr>()>;
using  infix_call = std::function<std::unique_ptr<Expr>(std::unique_ptr<Expr>& )>;

Prog parse(const char *str);
#endif
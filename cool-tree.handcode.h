//
// The following include files must come first.

#ifndef COOL_TREE_HANDCODE_H
#define COOL_TREE_HANDCODE_H

#include <iostream>
#include "tree.h"
#include "cool.h"
#include "stringtab.h"
#define yylineno curr_lineno;
extern int yylineno;

inline Boolean copy_Boolean(Boolean b) {return b; }
inline void assert_Boolean(Boolean) {}
inline void dump_Boolean(ostream& stream, int padding, Boolean b)
	{ stream << pad(padding) << (int) b << "\n"; }

void dump_Symbol(ostream& stream, int padding, Symbol b);
void assert_Symbol(Symbol b);
Symbol copy_Symbol(Symbol b);

class Program_class;
typedef Program_class *Program;
class Class__class;
typedef Class__class *Class_;
class Feature_class;
typedef Feature_class *Feature;
class Formal_class;
typedef Formal_class *Formal;
class Expression_class;
typedef Expression_class *Expression;
class Case_class;
typedef Case_class *Case;

typedef list_node<Class_> Classes_class;
typedef Classes_class *Classes;
typedef list_node<Feature> Features_class;
typedef Features_class *Features;
typedef list_node<Formal> Formals_class;
typedef Formals_class *Formals;
typedef list_node<Expression> Expressions_class;
typedef Expressions_class *Expressions;
typedef list_node<Case> Cases_class;
typedef Cases_class *Cases;

enum class ExpressionType: uint8_t {
	Assign,
	StaticDispatch,
	Dispatch,
	Conditional,
	Loop,
	TypeCase,
	Block,
	Let,
	Plus,
	Sub,
	Mul,
	Divide,
	Neg,
    Lt,
    Eq,
    Leq,
    Comp,
    IntConst,
    BoolConst,
    StringConst,
    New,
    IsVoid,
    NoExpr,
    Object
};

#define Program_EXTRAS                          \
virtual void semant() = 0;			\
virtual void dump_with_types(ostream&, int) = 0; 



#define program_EXTRAS                          \
void semant();     				\
void dump_with_types(ostream&, int);            

#define Class__EXTRAS                   \
virtual Symbol get_filename() = 0;      \
virtual void dump_with_types(ostream&,int) = 0; \
virtual Symbol get_parent() = 0;                 \
virtual Symbol get_name() = 0;					\
virtual Features get_features() = 0;


#define class__EXTRAS                                 \
Symbol get_filename() { return filename; }             \
void dump_with_types(ostream&,int);                     \
Symbol get_parent() { return parent; }                   \
Symbol get_name() { return name; }						\
Features get_features() { return features; }			

#define Feature_EXTRAS                                        \
virtual void dump_with_types(ostream&,int) = 0; \
virtual bool is_attr() = 0;	\
virtual Symbol get_name() = 0;	\
virtual Symbol get_type() = 0; \
virtual Expression get_expression() = 0;

#define attr_EXTRAS \
bool is_attr() { return true; }	\
Symbol get_name() { return name; } \
Symbol get_type() { return type_decl; } \
Expression get_expression() { return init; }

#define method_EXTRAS \
bool is_attr() { return false; }	\
Symbol get_name() { return name; }	\
Symbol get_type() { return return_type; } \
Expression get_expression() { return expr; }  \
Formals get_formals() { return formals; };

#define Feature_SHARED_EXTRAS                                       \
void dump_with_types(ostream&,int);    

#define Formal_EXTRAS                              \
virtual void dump_with_types(ostream&,int) = 0; \
virtual Symbol get_name() = 0;	\
virtual Symbol get_type() = 0;

#define formal_EXTRAS                           \
void dump_with_types(ostream&,int);	\
Symbol get_name() { return name; } \
Symbol get_type() { return type_decl; } 


#define Case_EXTRAS                             \
virtual void dump_with_types(ostream& ,int) = 0;


#define branch_EXTRAS                                   \
void dump_with_types(ostream& ,int);	\
Symbol get_name() { return name; };	\
Symbol get_type() { return type_decl; }	\
Expression get_expr() { return expr; }

#define Expression_EXTRAS                    \
Symbol type;                                 \
Symbol get_type() { return type; }           \
Expression set_type(Symbol s) { type = s; return this; } \
virtual void dump_with_types(ostream&,int) = 0;  \
void dump_type(ostream&, int);               \
Expression_class() { type = (Symbol) NULL; } \
virtual ExpressionType get_expr_type() = 0;	\
/* For binary operators */virtual Expression get_lhs() { return nullptr; } \
/* For binary operators */virtual Expression get_rhs() { return nullptr; }	\
/* For dispatch */virtual Expression get_dispatch_id_expr() { return nullptr; }	\
/* For dispatch */virtual Symbol get_dispatch_method_name() { return nullptr; }	\
/* For dispatch */virtual Expressions get_dispatch_param_expressions() { return nullptr; }	\
/* For dispatch */virtual Symbol get_dispatch_subclass_type() { return nullptr; }

#define assign_EXTRAS	\
ExpressionType get_expr_type() { return ExpressionType::Assign; }	\
Symbol get_symbol_name() { return name; }	\
Expression get_expr() { return expr; }

#define static_dispatch_EXTRAS	\
ExpressionType get_expr_type() { return ExpressionType::StaticDispatch; }	\
Expression get_dispatch_id_expr() { return expr; }	\
Symbol get_dispatch_method_name() { return name; }	\
Expressions get_dispatch_param_expressions() { return actual; }	\
Symbol get_dispatch_subclass_type() { return type_name; }

#define dispatch_EXTRAS	\
ExpressionType get_expr_type() { return ExpressionType::Dispatch; }	\
Expression get_dispatch_id_expr() { return expr; }	\
Symbol get_dispatch_method_name() { return name; }	\
Expressions get_dispatch_param_expressions() { return actual; }

#define cond_EXTRAS	\
ExpressionType get_expr_type() { return ExpressionType::Conditional; }	\
Expression get_pred() { return pred; }	\
Expression get_then() { return then_exp; }	\
Expression get_else() { return else_exp; }

#define loop_EXTRAS	\
ExpressionType get_expr_type() { return ExpressionType::Loop; } \
Expression get_pred() { return pred; }	\
Expression get_body() { return body; }

#define typcase_EXTRAS	\
ExpressionType get_expr_type() { return ExpressionType::TypeCase; }	\
Expression get_case_expr() { return expr; }	\
Cases get_cases() { return cases; }

#define block_EXTRAS	\
ExpressionType get_expr_type() { return ExpressionType::Block; }	\
Expressions get_body() { return body; };

#define let_EXTRAS	\
ExpressionType get_expr_type() { return ExpressionType::Let; }	\
Symbol get_let_id() { return identifier; }	\
Symbol get_let_type_decl() { return type_decl; }	\
Expression get_let_init() { return init; }	\
Expression get_let_body() { return body; }	

#define plus_EXTRAS	\
ExpressionType get_expr_type() { return ExpressionType::Plus; }	\
Expression get_lhs() { return e1; } \
Expression get_rhs() { return e2; }

#define sub_EXTRAS	\
ExpressionType get_expr_type() { return ExpressionType::Sub; }	\
Expression get_lhs() { return e1; } \
Expression get_rhs() { return e2; }

#define mul_EXTRAS	\
ExpressionType get_expr_type() { return ExpressionType::Mul; }	\
Expression get_lhs() { return e1; } \
Expression get_rhs() { return e2; }

#define divide_EXTRAS	\
ExpressionType get_expr_type() { return ExpressionType::Divide; }	\
Expression get_lhs() { return e1; } \
Expression get_rhs() { return e2; }

#define neg_EXTRAS	\
ExpressionType get_expr_type() { return ExpressionType::Neg; }	\
Expression get_lhs() { return e1; }

#define lt_EXTRAS	\
ExpressionType get_expr_type() { return ExpressionType::Lt; }	\
Expression get_lhs() { return e1; } \
Expression get_rhs() { return e2; }

#define eq_EXTRAS	\
ExpressionType get_expr_type() { return ExpressionType::Eq; }	\
Expression get_lhs() { return e1; } \
Expression get_rhs() { return e2; }

#define leq_EXTRAS	\
ExpressionType get_expr_type() { return ExpressionType::Leq; }	\
Expression get_lhs() { return e1; } \
Expression get_rhs() { return e2; }

#define comp_EXTRAS	\
ExpressionType get_expr_type() { return ExpressionType::Comp; } \
Expression get_rhs() { return e1; } \

#define int_const_EXTRAS	\
ExpressionType get_expr_type() { return ExpressionType::IntConst; }

#define bool_const_EXTRAS	\
ExpressionType get_expr_type() { return ExpressionType::BoolConst; }

#define string_const_EXTRAS	\
ExpressionType get_expr_type() { return ExpressionType::StringConst; }

#define new__EXTRAS	\
ExpressionType get_expr_type() { return ExpressionType::New; } \
Symbol get_type_name() { return type_name; }

#define isvoid_EXTRAS	\
ExpressionType get_expr_type() { return ExpressionType::IsVoid; }	\
Expression get_rhs() { return e1; }

#define no_expr_EXTRAS	\
ExpressionType get_expr_type() { return ExpressionType::NoExpr; }

#define object_EXTRAS	\
ExpressionType get_expr_type() { return ExpressionType::Object; }	\
Symbol get_name() { return name; }

#define Expression_SHARED_EXTRAS           \
void dump_with_types(ostream&,int); 

#endif

/****************************************************/
/* File: tiny.y                                     */
/* The TINY Yacc/Bison specification file           */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/
%{
#define YYPARSER /* distinguishes Yacc output from other code files */

#include "globals.h"
#include "util.h"
#include "scan.h"
#include "parse.h"

#define YYSTYPE TreeNode *
static char * savedName; /* for use in assignments */
static int savedLineNo;  /* ditto */
static TreeNode * savedTree; /* stores syntax tree for later return */
static int yylex(void); // added 11/2/11 to ensure no conflict with lex

%}

%token IF ELSE INT RETURN VOID WHILE
%token ID NUM 
%token ASSIGN EQ LT PLUS MINUS TIMES OVER LPAREN RPAREN SEMI NE LE GT GE LBRACE RBRACE LCURLY RCURLY COMMA
%token ERROR 

%% /* Grammar for TINY */

program     : decl_list
				{ savedTree = $1;}
            ;
decl_list	: decl_list decl
				{   d = $1;
					$$=$1;
					if(d==NULL){d = $2; $$->sibling = d;}
					else{d->sibling = $2; d = d->sibling;}
				}
			| decl
				{
					$$=$1;
				}
			;
decl		: var_decl
				{
					$$=$1;
				}
			| func_decl
				{
					$$=$1;
				}
			;
var_decl	: type_spec ID SEMI
				{
					$$=$1;
					$$->attr.name = copyString(tokenString);
				}
			| type_spec ID LBRACE NUM RBRACE SEMI
				{
					$$=$1;
					$$->attr.name = copyString(tokenString);
					$$->type = Array;
					$$->size = atoi(numString);
				}
			;
type_spec	: INT
				{
					$$ = newDeclNode(VarK);
					$$->type = Integer;
				}
			| VOID
				{
					$$ = newDeclNode(VarK);
					$$->type = Void;
				}
			;
func_decl	: type_spec ID LPAREN params RPAREN comp_stmt
				{
					$$ = $1;
					$$->attr.name = copyString(tempString);
					$$->kind.decl = FuncK;
					$$->child[0] = $4;
					$$->child[1] = $6;
				}
			;

params		: param_list
				{
					$$ = $1;
				}
			| VOID
				{
					$$ = newDeclNode(ParamK);
					$$->type = Void;
					r = NULL;
				}
			;
param_list	: param_list COMMA param
				{
					$$ = $1;
					if(r==NULL) {r = $3; $$->sibling = r;}
					else{r->sibling = $3; r = r->sibling;}
				}
			| param
				{
					$$ = $1;
				}
			;
param		: type_spec ID
				{
					$$ = $1;
					$$->kind.decl = ParamK;
					$$->attr.name = copyString(tokenString);
				}
			| type_spec ID LBRACE RBRACE
				{
					$$ = $1;
					$$->kind.decl = ParamK;
					$$->attr.name = copyString(tokenString);
					$$->type = Array;
					$$->size = -1;
				}
			;
comp_stmt	: LCURLY local_decl stmt_list RCURLY
				{
					$$ = newStmtNode(CompK);
					l = NULL;
					t = NULL;
					$$->child[0] = $2;
					$$->child[1] = $3;
				}
			;
local_decl 	: local_decl var_decl
				{
					$$ = $1;
					if($$==NULL)
					{$$=$2;}
					else
					{
						if(l==NULL)
						{
							l=$2;
							$$->sibling = l;
						}
						else
						{
							l->sibling = $2;
							l = l->sibling;
						}
					}
				}
			|
				{
					$$ = NULL;
				}
			;
stmt_list	: stmt_list stmt
				{
					$$ = $1;
					if($$=NULL) {$$=$2;}
					else
					{
						if(t==NULL)
						{
							t = $2;
							$$->sibling = t;
						}
						else
						{
							t->sibling = $2;
							t = t->sibling;
						}
					}
				}
			|
				{
					$$=NULL;
				}
			;
stmt        : exp_stmt { $$ = $1; }
            | comp_stmt { $$ = $1; }
            | select_stmt { $$ = $1; }
            | iter_stmt { $$ = $1; }
            | ret_stmt { $$ = $1; }
            ;
exp_stmt	: exp SEMI
				{
					$$ = $1;
				}
			| SEMI
				{}
			;
select_stmt	: IF LPAREN exp RPAREN stmt
                 { $$ = newStmtNode(IfK);
                   $$->child[0] = $3;
                   $$->child[1] = $5;
                 }
            | IF LPAREN exp RPAREN stmt ELSE stmt
                 { $$ = newStmtNode(IfK);
                   $$->child[0] = $3;
                   $$->child[1] = $5;
                   $$->child[2] = $7;
                 }
            ;
iter_stmt	: WHILE LPAREN exp RPAREN stmt
				{
					$$ = newStmtNode(IterK);
					$$->child[0] = $3;
					$$->child[1] = $5;
				}
			;
ret_stmt	: RETURN SEMI
				{
					$$ = newStmtNode(RetK);
				}
			| RETURN exp SEMI
				{
					$$ = newStmtNode(RetK);
					$$->child[0] = $2;
				}
			;
exp         : var ASSIGN exp
				{
					$$ = newExpNode(AssignK);
					$$->attr.op = copyString("=");
					$$->child[0] = $1;
					$$->child[1] = $3;
				}
            | simple_exp { $$ = $1; }
            ;
var			: ID
				{
					$$ = newExpNode(IdK);
					$$->attr.name = copyString(tokenString);
				}
			| ID LBRACE exp RBRACE
				{
					$$ = newExpNode(IdK);
					$$->attr.name = copyString(tokenString);
					$$->child[0] = $3;
					$$->child[0]->kind.exp = ArrK;
				}
			;
simple_exp	: addi_exp relop addi_exp
				{
					$$ = $2;
					$$->child[0] = $1;
					$$->child[1] = $3;
				}
			| addi_exp
				{
					$$ = $1;
				}
			;
relop		: LE {$$ = newExpNode(OpK); $$->attr.op = copyString("<=");}
			| EQ {$$ = newExpNode(OpK); $$->attr.op = copyString("==");}
			| NE {$$ = newExpNode(OpK); $$->attr.op = copyString("!=");}
			| LT {$$ = newExpNode(OpK); $$->attr.op = copyString("<");}
			| GT {$$ = newExpNode(OpK); $$->attr.op = copyString(">");}
			| GE {$$ = newExpNode(OpK); $$->attr.op = copyString(">=");}
			;
addi_exp	: addi_exp addop term
				{
					$$ = $2;
					$$->child[0] = $1;
					$$->child[1] = $3;
				}
			| term
				{
					$$ = $1;
				}
			;
addop		: PLUS {$$ = newExpNode(OpK); $$->attr.op = copyString("+");}
			| MINUS{$$ = newExpNode(OpK); $$->attr.op = copyString("-");}
			;
term		: term mulop factor
				{
					$$ = $2;
					$$->child[0] = $1;
					$$->child[1] = $3;
				}
			| factor
				{
					$$ = $1;
				}
			;
mulop		: TIMES {$$ = newExpNode(OpK); $$->attr.op = copyString("*");}
			| OVER  {$$ = newExpNode(OpK); $$->attr.op = copyString("/");}
			;
factor		: LPAREN exp RPAREN
				{
					$$ = $2;
				}
			| var
				{
					$$ = $1;
				}
			| call
				{
					$$ = $1;
				}
			| NUM
				{
					$$ = newExpNode(ConstK);
					$$->attr.val = atoi(numString);
				}
			;
call		: ID LPAREN args RPAREN
				{
					$$ = newExpNode(CalK);
					$$->attr.name = copyString(tempString);
					$$->child[0] = $3;
				}
			;
args		: args_list
				{
					$$ = $1;
				}
			|
				{}
			;
args_list	: args_list COMMA exp
				
				{
					$$ = $1;
					if(g==NULL) {g=$3; $$->sibling = g;}
					else {g->sibling = $3; g=g->sibling;}
				}
			| exp
				{ $$ = $1; }
			;
%%

int yyerror(char * message)
{ fprintf(listing,"Syntax error at line %d: %s\n",lineno,message);
  fprintf(listing,"Current token: ");
  printToken(yychar,tokenString);
  Error = TRUE;
  return 0;
}

/* yylex calls getToken to make Yacc/Bison output
 * compatible with ealier versions of the TINY scanner
 */
static int yylex(void)
{ return getToken(); }

TreeNode * parse(void)
{ yyparse();
  return savedTree;
}


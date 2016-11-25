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
static int savedNumber;
static int saveType;
%}

%token IF ELSE INT RETURN VOID WHILE
%token ID NUM 
%token ASSIGN EQ LT PLUS MINUS TIMES OVER LPAREN RPAREN SEMI NE LE GT GE LBRACE RBRACE LCURLY RCURLY COMMA
%token ERROR 

%% /* Grammar for TINY */
program     : decl_list
			{ savedTree = $1;}
			;
decl_list   : decl_list decl
			{ YYSTYPE t = $1;
		  	  if (t != NULL)
			  { while (t->sibling != NULL)
				t = t->sibling;
				t->sibling = $2;
				$$ = $1; }
			  else $$ = $2;
			}
			| decl  { $$ = $1; }
			;
decl        : var_decl  { $$ = $1; }
			| fun_decl  { $$ = $1; }
			;
saveName    : ID
			{ savedName = copyString(tokenString);
			  savedLineNo = lineno;
			}
			;
saveNumber  : NUM
			{ savedNumber = atoi(tokenString);
			  savedLineNo = lineno;
			}
			;
var_decl    : type_spec saveName SEMI
			{ $$ = newDeclNode(VarK);
			  $$->child[0] = $1; /* type */
			  $$->lineno = lineno;
			  $$->attr.name = savedName;
	 		  }
			| type_spec saveName LBRACE saveNumber RBRACE SEMI
			{ $$ = newDeclNode(ArrVarK);
			  $$->child[0] = $1; /* type */
			  $$->lineno = lineno;
			  $$->attr.arr.name = savedName;
			  $$->attr.arr.size = savedNumber;
			}
			;
type_spec   : INT
			{ $$ = newTypeNode(TypeNameK);
			  $$->attr.type = INT;
			}
			| VOID
			{ $$ = newTypeNode(TypeNameK);
			  $$->attr.type = VOID;
			}
			;
fun_decl    : type_spec saveName 
			{
			  $$ = newDeclNode(FuncK);
			  $$->lineno = lineno;
			  $$->attr.name = savedName;
			}
			LPAREN params RPAREN comp_stmt
			{
			  $$ = $3;
			  $$->child[0] = $1; 	/* type print*/
	  		  $$->child[1] = $5;    /* param print*/
			  $$->child[2] = $7; 	/* body print*/
			}
			;
params      : param_list  { $$ = $1; }
			| VOID
			{ $$ = newTypeNode(TypeNameK);
			  $$->attr.type = VOID;
			}
param_list  : param_list COMMA param
			{ YYSTYPE t = $1;
			  if (t != NULL)
			  { while (t->sibling != NULL)
				t = t->sibling;
				t->sibling = $3;
				$$ = $1; }
			  else $$ = $3; 
			}	
			| param { $$ = $1; };
param       : type_spec saveName
			{ $$ = newParamNode(NonArrParamK);
			  $$->child[0] = $1;
			  $$->attr.name = savedName;
			}
			| type_spec saveName LBRACE RBRACE
			{ $$ = newParamNode(ArrParamK);
			  $$->child[0] = $1;
			  $$->attr.name = savedName;
			}
			;
comp_stmt   : LCURLY local_decls stmt_list RCURLY
			{ $$ = newStmtNode(CompK);
			  $$->child[0] = $2;
			  $$->child[1] = $3;
			}
			;
local_decls : local_decls var_decl
			{ YYSTYPE t = $1;
			  if (t != NULL)
			  { while (t->sibling != NULL)
				t = t->sibling;
				t->sibling = $2;
				$$ = $1; }
			  else $$ = $2;
			}
			| { $$ = NULL; }
			;
stmt_list   : stmt_list stmt
			{ YYSTYPE t = $1;
			  if (t != NULL)
			{ while (t->sibling != NULL)
				t = t->sibling;
				t->sibling = $2;
				$$ = $1; }
			else $$ = $2;
			}
			| { $$ = NULL; }
			;
stmt        : exp_stmt { $$ = $1; }
			| comp_stmt { $$ = $1; }
			| sel_stmt { $$ = $1; }
			| iter_stmt { $$ = $1; }
			| ret_stmt { $$ = $1; }
			;
exp_stmt    : exp SEMI { $$ = $1; }
			| SEMI { $$ = NULL; }
			;
sel_stmt    : IF LPAREN exp RPAREN stmt
			{ $$ = newStmtNode(IfK);
			  $$->child[0] = $3;
			  $$->child[1] = $5;
			  $$->child[2] = NULL;
			}
			| IF LPAREN exp RPAREN stmt ELSE stmt
			{ $$ = newStmtNode(IfK);
			  $$->child[0] = $3;
			  $$->child[1] = $5;
			  $$->child[2] = $7;
			}
			;
iter_stmt   : WHILE LPAREN exp RPAREN stmt
			{ $$ = newStmtNode(IterK);
			  $$->child[0] = $3;
			  $$->child[1] = $5;
			}
			;
ret_stmt    : RETURN SEMI
			{ $$ = newStmtNode(RetK);
			  $$->child[0] = NULL;
			}
			| RETURN exp SEMI
			{ $$ = newStmtNode(RetK);
			  $$->child[0] = $2;
			}
			;
exp         : var ASSIGN exp
			{ $$ = newExpNode(AssignK);
			  $$->child[0] = $1;
			  $$->child[1] = $3;
			}
			| simple_exp { $$ = $1; }
			;
var         : saveName
			{ $$ = newExpNode(IdK);
			  $$->attr.name = savedName;
			}
			| saveName
			{ $$ = newExpNode(ArrIdK);
			  $$->attr.name = savedName;
			}
			LBRACE exp RBRACE
			{ $$ = $2;
			$$->child[0] = $4;}
			;
simple_exp  : add_exp relop add_exp
			{
				$$ = $2;
				$$->child[0] = $1;
				$$->child[1] = $3;
			}
			| add_exp { $$ = $1; }
			;
relop		: LE {$$ = newExpNode(OpK); $$->attr.op = LE;}
			| EQ {$$ = newExpNode(OpK); $$->attr.op = EQ;}
			| NE {$$ = newExpNode(OpK); $$->attr.op = NE;}
			| LT {$$ = newExpNode(OpK); $$->attr.op = LT;}
			| GT {$$ = newExpNode(OpK); $$->attr.op = GT;}
			| GE {$$ = newExpNode(OpK); $$->attr.op = GE;}
			;
add_exp     : add_exp addop term
			{ $$ = $2;
			  $$->child[0] = $1;
			  $$->child[1] = $3;
			}
			| term { $$ = $1; }
			;
addop		: PLUS {$$ = newExpNode(OpK); $$->attr.op = PLUS;}
			| MINUS{$$ = newExpNode(OpK); $$->attr.op = MINUS;}
			;
term        : term mulop factor
			{ $$ = $2;
			  $$->child[0] = $1;
			  $$->child[1] = $3;
			}
			| factor { $$ = $1; }
			;
mulop		: TIMES {$$ = newExpNode(OpK); $$->attr.op = TIMES;}
			| OVER  {$$ = newExpNode(OpK); $$->attr.op = OVER;}
			;
factor      : LPAREN exp RPAREN { $$ = $2; }
			| var { $$ = $1; }
			| call { $$ = $1; }
			| NUM
			{ $$ = newExpNode(ConstK);
			  $$->attr.val = atoi(tokenString);
			}
			;
call        : saveName 
			{
			  $$ = newExpNode(CallK);
			  $$->attr.name = savedName;
			}
			LPAREN args RPAREN
			{ $$ = $2;
			  $$->child[0] = $4;
			}
			;
args        : arg_list { $$ = $1; }
			| /* empty */ { $$ = NULL; }
			;
arg_list    : arg_list COMMA exp
			{ YYSTYPE t = $1;
			  if (t != NULL)
			  { while (t->sibling != NULL)
				t = t->sibling;
				t->sibling = $3;
				$$ = $1; }
			  else $$ = $3;
			}
			| exp { $$ = $1; }
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


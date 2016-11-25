/****************************************************/
/* File: analyze.c                                  */
/* Semantic analyzer implementation                 */
/* for the TINY compiler                            */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "globals.h"
#include "symtab.h"
#include "analyze.h"
#include "util.h"

/* counter for variable memory locations */
static int location = 0;
static char * scope;
static int isFuncC = FALSE;

/* Procedure traverse is a generic recursive 
 * syntax tree traversal routine:
 * it applies preProc in preorder and postProc 
 * in postorder to tree pointed to by t
 */
static void traverse( TreeNode * t,
               void (* preProc) (TreeNode *),
               void (* postProc) (TreeNode *) )
{ if (t != NULL)
  { preProc(t);
    { int i;
      for (i=0; i < MAXCHILDREN; i++)
        traverse(t->child[i],preProc,postProc);
    }
    postProc(t);
    traverse(t->sibling,preProc,postProc);
  }
}

/* nullProc is a do-nothing procedure to 
 * generate preorder-only or postorder-only
 * traversals from traverse
 */
static void nullProc(TreeNode * t)
{ if (t==NULL) return;
  else return;
}

static void symbolError(TreeNode * t, char * err)
{
  char * type;
  switch(t->child[0]->attr.type){
    case INT:
      type = "int";
      break;
    case VOID:
      type = "void";
      break;
    default:
      type = "???";
      break;
  }
  fprintf(listing,"undeclared %s %s at line %d : %s \n", type, t->attr.name, t->lineno, err);
}

static void afterInsert( TreeNode * t)
{
  switch (t->nodekind)
  {
    case StmtK:
      switch (t->kind.stmt)
      {
        case CompK:
          printf("pop\n");
          pop_scope();
          location--;
          break;
        default:
          break;
      }
  }
}
/* Procedure insertNode inserts 
 * identifiers stored in t into 
 * the symbol table 
 */
static void insertNode( TreeNode * t)
{ 
  switch (t->nodekind)
  { case StmtK:
      switch (t->kind.stmt)
      { 
        case CompK:
            printf("CompK\n");
            if(isFuncC)
              isFuncC = FALSE;
            else{
              ScopeList newScope = createscope(scope);
              push_scope(newScope); location++;
            }
          break;
        default:
          break;
      }
      break;
    case ExpK:
      switch (t->kind.exp)
      { case IdK:
        case ArrIdK:
        case CallK:
          printf("ExpK\n");
          printf("%s \n",t->attr.name);
          if (st_lookup(scope, t->attr.name) == -1)
            symbolError(t, "undeclared symbol");
          break;
        default:
          break;
      }
      break;
    case DeclK:
      switch (t->kind.exp)
      { case FuncK:
            printf("FuncK %s\n", t->attr.name);
            isFuncC = TRUE;
            if ( st_lookup(scope, t->attr.name) >= 0 )
              symbolError(t, "Declared symbol");
            else{
              st_insert(t->attr.name,t->attr.name,t->child[0]->attr.type,t->lineno, location);
              scope = t->attr.name;
              push_scope(createscope(scope)); location++;
            }
            printf("end\n");
          break;
        case VarK:
            printf("VarK : %s\n", t->attr.name);
            if ( st_lookup(scope,t->attr.name) >= 0 )
              symbolError(t, "Declared symbol");
            else
              st_insert(scope, t->attr.name, t->child[0]->attr.type, t->lineno, location);
          break;
        case ArrVarK:
            printf("VarK : %s\n", t->attr.arr.name);
            if ( st_lookup(scope,t->attr.arr.name) >= 0 )
              symbolError(t, "Declared symbol");
            else
              st_insert(scope, t->attr.arr.name, t->child[0]->attr.type, t->lineno, location);
          break;
        default:
          break;
      }
      break;
    case ParamK:
          printf("ParamK\n");
          if ( st_lookup(scope, t->attr.name) >= 0 )
            symbolError(t, "Declared symbol");
          else
            st_insert(scope, t->attr.name, t->child[0]->attr.type, t->lineno, location);
      break;
    default:
      break;
  }
}

void insertGeneralFunc()
{
  TreeNode * func;
  TreeNode * type;
  TreeNode * param;
  TreeNode * comp_stmt;

  func = newDeclNode(FuncK);
  type = newTypeNode(TypeNameK);
  type->attr.type = INT;
  func->type = Integer;

  comp_stmt = newStmtNode(CompK);
  comp_stmt->child[0] = NULL;
  comp_stmt->child[1] = NULL;
  comp_stmt->child[2] = NULL;

  func->lineno = 0;
  func->attr.name = "input";
  func->child[0] = type;
  func->child[1] = NULL;
  func->child[2] = comp_stmt;

  st_insert(NULL, "input", type->attr.type, -1, location);

  func = newDeclNode(FuncK);
  type = newTypeNode(TypeNameK);
  type->attr.type = VOID;
  func->type = Void;

  comp_stmt = newStmtNode(CompK);
  comp_stmt->child[0] = NULL;
  comp_stmt->child[1] = NULL;
  comp_stmt->child[2] = NULL;

  param = newParamNode(NonArrParamK);
  param->attr.name = "arg";
  param->child[0] = newTypeNode(TypeNameK);
  param->child[0]->attr.type = INT;

  func->lineno = 0;
  func->attr.name = "output";
  func->child[0] = type;
  func->child[1] = param;
  func->child[2] = comp_stmt;

  st_insert(NULL, "output", type->attr.type, -1, location);
}
/* Function buildSymtab constructs the symbol 
 * table by preorder traversal of the syntax tree
 */
void buildSymtab(TreeNode * syntaxTree)
{ printf("buildSymTab\n");
  g_scope = createscope("Global");
  push_scope(g_scope);
  insertGeneralFunc();
  traverse(syntaxTree,insertNode,afterInsert);
  if (TraceAnalyze)
  { printf("printsymtab\n");
    fprintf(listing,"\nSymbol table:\n\n");
    printSymTab(listing);
  }
}

static void typeError(TreeNode * t, char * message)
{ fprintf(listing,"Type error at line %d: %s\n",t->lineno,message);
  Error = TRUE;
}

/* Procedure checkNode performs
 * type checking at a single tree node
 */
static void checkNode(TreeNode * t)
{ /*switch (t->nodekind)
  { case ExpK:
      switch (t->kind.exp)
      { case OpK:
          if ((t->child[0]->type != Integer) ||
              (t->child[1]->type != Integer))
            typeError(t,"Op applied to non-integer");
          if ((t->attr.op == EQ) || (t->attr.op == LT))
            t->type = Boolean;
          else
            t->type = Integer;
          break;
        case ConstK:
        case IdK:
          t->type = Integer;
          break;
        default:
          break;
      }
      break;
    case StmtK:
      switch (t->kind.stmt)
      { case IfK:
          if (t->child[0]->type == Integer)
            typeError(t->child[0],"if test is not Boolean");
          break;
        case AssignK:
          if (t->child[0]->type != Integer)
            typeError(t->child[0],"assignment of non-integer value");
          break;
          break;
        default:
          break;
      }
      break;
    default:
      break;

  }*/
}

/* Procedure typeCheck performs type checking 
 * by a postorder syntax tree traversal
 */
void typeCheck(TreeNode * syntaxTree)
{ traverse(syntaxTree,nullProc,checkNode);
}

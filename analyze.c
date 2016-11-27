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
  switch(t->type){
    case Integer:
      type = "int";
      break;
    case Void:
      type = "void";
      break;
    case IntegerArray:
      type = "IntegerArray";
      break;
    default:
      type = "void";
      break;
  }
  fprintf(listing,"Err %s %s at line %d : %s \n", type, t->attr.name, t->lineno, err);
}

static void afterInsert( TreeNode * t)
{
  switch (t->nodekind)
  {
    case StmtK:
      switch (t->kind.stmt)
      {
        case CompK:
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
          if (st_lookup(scope, t->attr.name) == -1)
            symbolError(t, "undeclared symbol");
          else
            add_line(t->attr.name, t->lineno);
          break;
        case AssignK:{
            ExpType ty;
            ty = type_lookup(scope,t->child[0]->attr.name);
            if( ty == IntegerArray && t->child[0]->kind.exp == IdK )
              symbolError(t, "cannot assign array to");
          }
          break;
        case ConstK:
            t->type = Integer;
            break;
        default:
          break;
      }
      break;
    case DeclK:
      switch (t->kind.exp)
      { case FuncK:
            if ( st_lookup(scope, t->attr.name) >= 0 )
              symbolError(t, "Declared symbol");
            else
              isFuncC = TRUE;
              switch(t->child[0]->attr.type){
                case INT:
                  t->type = Integer;
                  break;
                case VOID:
                default:
                  t->type = Void;
                  break;
                }
              st_insert("Func",t->attr.name,t->type,t->lineno, location);
              scope = t->attr.name;
              push_scope(createscope(scope)); location++;
          break;
        case VarK:
            if ( st_lookup_cur(scope,t->attr.name) >= 0 )
              symbolError(t, "Declared symbol");
            else{
              if (t->child[0]->attr.type == VOID){
                symbolError(t,"variable cannot have void type");
                break;
              }
              t->type = Integer;
              st_insert("Var", t->attr.name, t->type, t->lineno, location);
            }
          break;
        case ArrVarK:
            if ( st_lookup_cur(scope,t->attr.arr.name) >= 0 )
              symbolError(t, "Declared symbol");
            else{
              if (t->child[0]->attr.type == VOID){
                symbolError(t,"variable cannot have void type");
                break;
              }
              t->type = IntegerArray;
              st_insert("Var", t->attr.arr.name, t->type, t->lineno, location);
            }
          break;
        default:
          break;
      }
      break;
    case ParamK:
          if ( st_lookup("Var", t->attr.name) >= 0 )
            symbolError(t, "Declared symbol");
          else{
            switch(t->kind.param){
              case ArrParamK:
                  t->type = IntegerArray;
                  st_insert("Var", t->attr.name, t->type, t->lineno, location);
                  break;
              default:
                  t->type = Integer;
                  st_insert("Var", t->attr.name, t->type, t->lineno, location);
                  break;
            }
          }
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

  st_insert("Func", "input", func->type, -1, location);
  push_pl(createpl("input",func));

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

  st_insert("Func", "output", func->type, -1, location);
  push_pl(createpl("output",func));

}
/* Function buildSymtab constructs the symbol 
 * table by preorder traversal of the syntax tree
 */
void buildSymtab(TreeNode * syntaxTree)
{ 
  g_scope = createscope("Global");
  push_scope(g_scope);
  insertGeneralFunc();
  traverse(syntaxTree,insertNode,afterInsert);
  if (TraceAnalyze)
  { 
    fprintf(listing,"\nSymbol table:\n\n");
    printSymTab(listing);
  }
}

static void typeError(TreeNode * t, char * message)
{ fprintf(listing,"Type error at line %d: %s\n",t->lineno,message);
  Error = TRUE;
}

static void beforeCheck(TreeNode * t)
{ switch (t->nodekind)
  { case DeclK:
      switch (t->kind.decl)
      { case FuncK:
          scope = t->attr.name;
          push_pl(createpl(t->attr.name, t));
          break;
        default:
          break;
      }
      break;
    case ExpK:
      switch (t->kind.exp)
      {
        case IdK:
          t->type = type_lookup(scope, t->attr.name);
          break;
        case ArrIdK:
          t->type = type_lookup(scope, t->attr.name);
          break;
        case CallK:
          t->type = sc_lookup(t->attr.name);
          break;
        default:
         break;
      }
    default:
      break;
  }
}

/* Procedure checkNode performs
 * type checking at a single tree node
 */
static void checkNode(TreeNode * t)
{ //printf("  %d \n", t->lineno);
  switch (t->nodekind)
  { case StmtK:
      switch (t->kind.stmt)
      {
        case CompK:
          break;
        case IfK:{
          if( t->child[0]->child[0]->type == Void)
            typeError(t->child[0], "no void");
        }
          break;
        case IterK:{
          if( t->child[0]->child[0]->type == Void)
            typeError(t->child[0], "no void");
        }
          break;
        case RetK:
          {
            const TreeNode * expr = t->child[0];
            if((sc_lookup(scope)==Void)&&(expr != NULL && expr->type != Void)){
              typeError(t,"Expected to return void");
            }
            else if((sc_lookup(scope)==Integer) && (expr == NULL || expr->type != Integer)){
              typeError(t,"Expected to return integer");
            }
          }
          break;
      }
      break;
    case ExpK:
      switch (t->kind.exp)
      {
        case AssignK:{
            //printf("asign1  %s   %d \n",t->child[0]->attr.name, t->child[0]->type);
            //printf("right        %d  \n",t->child[1]->type);
            const ExpType type_l = t->child[0]->type;
            const ExpType type_r = t->child[1]->type;
            if(type_l==Void || type_r == Void){
              typeError(t,"Void type connot assigned");
            }
            else
              t->type = type_l;
          }
          break;
        case OpK:{
            ExpType type_l = t->child[0]->type;
            ExpType type_r = t->child[1]->type;

            if(type_l == Void || type_r == Void)
              typeError(t, "void type is not allowd");
            else
              t->type = Integer;
          }
          break;
        case ConstK:
            t->type = Integer;
          break;
        case IdK:
          break;
        case ArrIdK:
          break;
        case CallK:{
            TreeNode * func = getpl(t->attr.name)->treenode;
            TreeNode * arg = t->child[0];
            TreeNode * param = func->child[1];
            printf("%s\n",t->attr.name);
            while(arg != NULL)
            { //printf("arg: %d,  param: %d,\n ",arg->child[0]->kind.exp, param->kind.param);
              if (param == NULL)
                typeError(arg,"the number of parameters wrong");
              else if(arg->type == Void)
                typeError(arg,"void is not allowd for parameters");
              else if( (arg->type ==Integer)&& (param->type==IntegerArray))
                typeError(arg,"different type parameters");
              else{
                arg = arg->sibling;
                param = param->sibling;
                continue;
              }
              break;
            }

            if ( arg == NULL && param != NULL )
              typeError(t,"the number of parameters wrong");

            t->type = func->type;
          }
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

/* Procedure typeCheck performs type checking 
 * by a postorder syntax tree traversal
 */
void typeCheck(TreeNode * syntaxTree)
{ 
  traverse(syntaxTree,beforeCheck,checkNode);
}

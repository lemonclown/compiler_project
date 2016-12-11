/****************************************************/
/* File: cgen.c                                     */
/* The code generator implementation                */
/* for the TINY compiler                            */
/* (generates code for the TM machine)              */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "globals.h"
#include "symtab.h"
#include "code.h"
#include "cgen.h"

/* tmpOffset is the memory offset for temps
   It is decremented each time a temp is
   stored, and incremeted when loaded again
*/
static int paramOffset = 0;
static int argOffset = -3;
static int isinFunc = FALSE;
static int isReturned = FALSE;

static char * prevScope;
/* prototype for internal recursive code generator */
static void cGen (TreeNode * tree);
static char * scope;

static void genExp( TreeNode * tree, int lhs);

/* Procedure genStmt generates code at a statement node */
static void genStmt( TreeNode * tree)
{ TreeNode * p1, * p2, * p3;
  int savedLoc1,savedLoc2,currentLoc;
  int paramnum, varnum;
  ScopeList Scope;
  switch (tree->kind.stmt) {
      case CompK:
        if(TraceCode) emitComment("-> Compound Stmt");
        p1 = tree->child[0];
        p2 = tree->child[1];
        if(isinFunc){  
          isinFunc = FALSE;
          push_scope(scope_lookup(scope));
          cGen(p1);
          cGen(p2);
          pop_scope(scope_lookup(scope));
          prevScope = scope;
        }
        else{
          cGen(p1);
          cGen(p2);
        }
        
        if(TraceCode) emitComment("<- Compound Stmt");
        break;
      case IfK :
         if (TraceCode) emitComment("-> if") ;
         p1 = tree->child[0] ;
         p2 = tree->child[1] ;
         p3 = tree->child[2] ;
         /* generate code for test expression */
         cGen(p1);
         savedLoc1 = emitSkip(1) ;
         emitComment("if: jump to else belongs here");
         /* recurse on then part */
         cGen(p2);
         savedLoc2 = emitSkip(1) ;
         emitComment("if: jump to end belongs here");
         currentLoc = emitSkip(0) ;
         emitBackup(savedLoc1) ;
         emitRM_Abs("JEQ",ac,currentLoc,"if: jmp to else");
         emitRestore() ;
         /* recurse on else part */
         cGen(p3);
         currentLoc = emitSkip(0) ;
         emitBackup(savedLoc2) ;
         emitRM_Abs("LDA",pc,currentLoc,"jmp to end") ;
         emitRestore() ;
         if (TraceCode)  emitComment("<- if") ;
         break; /* if_k */

      case RetK:
         if (TraceCode) emitComment("-> return");
         p1 = tree->child[0];
         cGen(p1);
         Scope = scope_lookup(scope);
         paramnum = Scope->paramNum;
         varnum = Scope->varNum;
         paramnum += varnum;
         //param
         while(paramnum!=0){
          emitRM("LDA", sp, 1, sp, "move stack pointer +1 : return");
          paramnum--;
        }
          //fp
         emitRM("LD", fp, 1, sp,"restore old fp");
         emitRM("LDA", sp, 1, sp, "move stack pointer +1");
         //sp
         emitRM("LDA", sp, 1, sp, "move stack pointer +1");
         //pc
         emitRM("LDA", sp, 1, sp, "move stack pointer +1");
         emitRM("LD", pc, 0, sp, "restore pc");
         
         isReturned = TRUE;
         if (TraceCode) emitComment("<- return");
         break;

      case IterK:
        if (TraceCode) emitComment("-> while") ;

        p1 = tree->child[0];
        p2 = tree->child[1];

        savedLoc1 = emitSkip(0);
        emitComment("while: jump after body comes back here");

        /* generate code for test expression */
        cGen(p1);

        savedLoc2 = emitSkip(1);
        emitComment("while: jump to end belongs here");

        /* generate code for body */
        cGen(p2);
        emitRM_Abs("LDA",pc,savedLoc1,"while: jmp back to test");
        /* backpatch */
        currentLoc = emitSkip(0);
        emitBackup(savedLoc2);
        emitRM_Abs("JEQ",ac,currentLoc,"while: jmp to end");
        emitRestore();
        if (TraceCode)  emitComment("<- while") ;
        break; /* repeat */
      default:
         break;
    }
} /* genStmt */

static void reverseTraverse(TreeNode * tree)
{
  TreeNode * t;
  t= tree;
  if (t == NULL) return;
  reverseTraverse(t->sibling);
  genExp(t,FALSE);
  
  emitRM("ST", ac, argOffset++, sp, "store arg in sp");
  //emitRM("LDA", sp, -1, sp, "move stack pointer -1");
}

/* Procedure genExp generates code at an expression node */
static void genExp( TreeNode * tree, int lhs)
{ int loc, savedLoc, paramnum, currentLoc;
  char buffer[256];
  TreeNode * p1, * p2;
  ScopeList Scope;
  ExpType type;
  switch (tree->kind.exp) {
    case AssignK:
      if(TraceCode) emitComment("-> Assign");
      p1 = tree->child[0];
      p2 = tree->child[1];
      genExp(p1, TRUE);
      emitRM("ST", ac, 0, sp, "store ac in stack pointer");
      emitRM("LDA", sp, -1, sp, "move stack pointer -1");
      cGen(p2);
      emitRM("LD", ac1, 1, sp, "load left");
      emitRM("ST", ac, 0, ac1, "store value");
      emitRM("LDA", sp, 1, sp, "move stack pointer +1");

      if(TraceCode) emitComment("<- Assign");
      break;
    case CallK:
      if(TraceCode) {
        sprintf(buffer,"-> Call : %s", tree->attr.name);
        emitComment(buffer);
      }
      loc = st_lookup("scope", tree->attr.name);
      Scope = scope_lookup(tree->attr.name);
      if(strcmp(tree->attr.name,"input")==0){
          emitRO("IN", ac, 0, 0, "input value");
        }
      else{
        p1 = tree->child[0];
        if((strcmp(tree->attr.name,"output")==0)) {
          cGen(p1);
          emitRO("OUT", ac, 0,0, "output value");
        }
        else {
          argOffset = argOffset - Scope->paramNum + 1;
          reverseTraverse(p1);
          argOffset=-3;
          //return address
          savedLoc = emitSkip(0);
          emitRM("LDC", ac1, savedLoc+10, 0,"return range");
          emitRM("ST", ac1, 0, sp, "store return addr");
          emitRM("LDA", sp, -1, sp, "move stack pointer -1");
          emitRM("LDA", ac1, 0, sp, "current loc");
          emitRM("ST", ac1, 0, sp, "store sp");
          emitRM("LDA", sp, -1, sp, "move stack pointer -1");
          emitRM("ST", fp, 0, sp, "store old fp");
          emitRM("LDA", sp, -1, sp, "move stack pointer -1");
          emitRM("LDA", fp, 0, sp, "move sp -> fp");
          //emitRM("LDA", sp, -1, sp, "move stack pointer -1");
          emitRM("LD", pc, loc, gp, "load func loc");
          //emitRM("LD", pc, 0, ac, "store memloc in pc");
        }
      }
      if(TraceCode) {
        sprintf(buffer,"<- Call : %s", tree->attr.name);
        emitComment(buffer);
      }
      break;
    case ConstK :
      if (TraceCode) emitComment("-> Const") ;
      /* gen code to load integer constant using LDC */
      emitRM("LDC",ac,tree->attr.val,0,"load const");
      if (TraceCode)  emitComment("<- Const") ;
      break; /* ConstK */
    
    case IdK :
      if (TraceCode) emitComment("-> Id") ;
      Scope = scope_lookup(scope);
      paramnum = Scope->paramNum;
      loc = -st_lookup("temp",tree->attr.name);
      type = type_lookup(scope, tree->attr.name);

      if(lhs || type == IntegerArray){
        if(strcmp(scope_name,"Global")==0){
          emitRM("LDA", ac, -loc, gp, "store memloc in ac :Global");
        }
        else{
          if(-loc<paramnum){
            emitRM("LD", ac, loc, fp, "store arr addr in ac : Local param");
          }
          else
            emitRM("LDA", ac, loc, fp, "store memloc in ac :Local");
        }
      }
      else{
        if(strcmp(scope_name,"Global")==0){
          emitRM("LD", ac, -loc, gp, "store memloc in ac :Global");
        }
        else{
          emitRM("LD", ac, loc, fp, "store memloc in ac :Local");
        }
      }
      if (TraceCode)  emitComment("<- Id") ;
      break; /* IdK */

    case ArrIdK :
      if (TraceCode) emitComment("-> ArrId");
      p1 = tree->child[0];

      cGen(p1);
      Scope = scope_lookup(scope);
      paramnum = Scope->paramNum;
      loc = st_lookup("temp", tree->attr.name);
      currentLoc = emitSkip(0);
      
      emitRM("LDC", ac1, loc, 0,"load loc");
      if(lhs){
        if(strcmp(scope_name,"Global")==0){
          emitRO("SUB", ac, ac1, ac, "sub array offset");
          emitRO("ADD", ac, ac, gp, "add arr loc gp, store");
        }
        else{
          if(loc<paramnum){
            emitRM("LD", ac1, -loc, fp, "load base addr of param arr");
            emitRO("SUB", ac, ac1, ac, "sub array offset");
          }
          else{
            emitRO("ADD", ac, ac1, ac, "add array offset");
            emitRO("SUB", ac, fp, ac,"sub arr loc fp, store");
          }
        }
      }
      else{
        if(strcmp(scope_name,"Global")==0){
          emitRO("SUB", ac1, ac1, ac, "add offset loc");
          emitRO("ADD", ac1, ac1, gp, "add arr loc gp");
          emitRM("LD", ac, 0, ac1, "store memloc in ac :Global");
        }
        else{
          if(loc<paramnum){
            emitRM("LD", ac1, -loc, fp, "load base addr of param arr");
            emitRO("SUB", ac, ac1, ac, "Sub array offset");
            emitRM("LD", ac, 0, ac, "load value array offset");
          }
          else{
            //emitRM("LD", ac, fp, ac1, "store memloc in ac :Local");
            emitRO("ADD", ac, ac1, ac, "add array offset");
            emitRO("SUB", ac, fp, ac,"sub arr loc fp, store");
            emitRM("LD", ac, 0, ac, "load value");
          }
        }
      }
      
      if (TraceCode) emitComment("<- ArrId");
      break;
    case OpK :
         if (TraceCode) emitComment("-> Op") ;
         p1 = tree->child[0];
         p2 = tree->child[1];
         /* gen code for ac = left arg */
         cGen(p1);
         /* gen code to push left operand */
         emitRM("ST", ac, 0, sp,"op: push left");
         emitRM("LDA", sp, -1, sp, "move stack pointer -1");
         /* gen code for ac = right operand */
         cGen(p2);
         /* now load left operand */
         emitRM("LD",ac1, 1, sp,"op: load left");
         emitRM("LDA", sp, 1, sp, "move stack pointer +1");
         switch (tree->attr.op) {
            case PLUS :
               emitRO("ADD",ac,ac1,ac,"op +");
               break;
            case MINUS :
               emitRO("SUB",ac,ac1,ac,"op -");
               break;
            case TIMES :
               emitRO("MUL",ac,ac1,ac,"op *");
               break;
            case OVER :
               emitRO("DIV",ac,ac1,ac,"op /");
               break;
            case LT :
               emitRO("SUB",ac,ac1,ac,"op <") ;
               emitRM("JLT",ac,2,pc,"br if true") ;
               emitRM("LDC",ac,0,ac,"false case") ;
               emitRM("LDA",pc,1,pc,"unconditional jmp") ;
               emitRM("LDC",ac,1,ac,"true case") ;
               break;
            case LE :
               emitRO("SUB",ac,ac1,ac,"op <=") ;
               emitRM("JLE",ac,2,pc,"br if true") ;
               emitRM("LDC",ac,0,ac,"false case") ;
               emitRM("LDA",pc,1,pc,"unconditional jmp") ;
               emitRM("LDC",ac,1,ac,"true case") ;
               break;
            case GT :
               emitRO("SUB",ac,ac1,ac,"op >") ;
               emitRM("JGT",ac,2,pc,"br if true") ;
               emitRM("LDC",ac,0,ac,"false case") ;
               emitRM("LDA",pc,1,pc,"unconditional jmp") ;
               emitRM("LDC",ac,1,ac,"true case") ;
               break;
            case GE :
               emitRO("SUB",ac,ac1,ac,"op >=") ;
               emitRM("JGE",ac,2,pc,"br if true") ;
               emitRM("LDC",ac,0,ac,"false case") ;
               emitRM("LDA",pc,1,pc,"unconditional jmp") ;
               emitRM("LDC",ac,1,ac,"true case") ;
               break;
            case EQ :
               emitRO("SUB",ac,ac1,ac,"op ==") ;
               emitRM("JEQ",ac,2,pc,"br if true");
               emitRM("LDC",ac,0,ac,"false case") ;
               emitRM("LDA",pc,1,pc,"unconditional jmp") ;
               emitRM("LDC",ac,1,ac,"true case") ;
               break;
            case NE :
               emitRO("SUB",ac,ac1,ac,"op !=") ;
               emitRM("JNE",ac,2,pc,"br if true") ;
               emitRM("LDC",ac,0,ac,"false case") ;
               emitRM("LDA",pc,1,pc,"unconditional jmp") ;
               emitRM("LDC",ac,1,ac,"true case") ;
               break;
            default:
               emitComment("BUG: Unknown operator");
               break;
         } /* case op */
         if (TraceCode)  emitComment("<- Op") ;
         break; /* OpK */

    default:
      break;
  }
} /* genExp */

static void genDecl(TreeNode * tree)
{
  TreeNode * p1, * p2;
  ScopeList Scope;
  int paramnum, varnum;
  int loc;
  int currentLoc, savedLoc1;
  char buffer[100];
  switch(tree->kind.decl)
  {
    case FuncK:
      if(TraceCode) {
        sprintf(buffer,"-> Func Decl : %s", tree->attr.name);
        emitComment(buffer);
      }
      isinFunc = TRUE;
      isReturned = FALSE;
      scope = tree->attr.name;
      Scope = scope_lookup(scope);
      varnum = Scope->varNum;
      loc = st_lookup(scope, tree->attr.name);
      emitRM("LDA", ac, 2, pc, "pc+2");
      emitRM("ST", ac, loc, gp, "load function");
      if(strcmp(tree->attr.name, "main")!=0)
        savedLoc1 = emitSkip(1);
      
      p1 = tree->child[1]; 
      //body
      while(varnum!=0){
        emitRM("LDA", sp, -1, sp, "move stack pointer -1 : var");
        varnum--;
      }
      p2 = tree->child[2];

      cGen(p1);
      cGen(p2);
      if(TraceCode) {
        paramOffset = 0;
        sprintf(buffer,"<- Func Decl : %s", tree->attr.name);
        emitComment(buffer);
      }
      if(strcmp(tree->attr.name, "main")==0)
        break;
      if (!isReturned){
          if(TraceCode) emitComment("-> Void return");
            Scope = scope_lookup(prevScope);
           paramnum = Scope->paramNum;
           varnum = Scope->varNum;
           paramnum += varnum;
           //param
           while(paramnum!=0){
            emitRM("LDA", sp, 1, sp, "move stack pointer +1 : return");
            paramnum--;
          }
           //param
            //fp
           emitRM("LD", fp, 1, sp,"restore old fp");
           emitRM("LDA", sp, 1, sp, "move stack pointer +1");
           //sp
           emitRM("LDA", sp, 1, sp, "move stack pointer +1");
           //pc
           emitRM("LDA", sp, 1, sp, "move stack pointer +1");
           emitRM("LD", pc, 0, sp, "restore pc");
           if(TraceCode) emitComment("<- Void return");
           isReturned=TRUE;
        }
      currentLoc = emitSkip(0);
      emitBackup(savedLoc1);
      // write
      emitRM("LDC", pc, currentLoc, 0,"jump to function end");
      emitRestore();
      break;
    default:
      break;
  }
}

static void genParam(TreeNode * tree)
{
  switch(tree->kind.param)
  {
    case NonArrParamK:
    case ArrParamK:
      if(TraceCode) emitComment("-> param");
      emitRM("LDA", sp, -1, sp, "move stack pointer -1 : param");
      paramOffset++;
      emitComment(tree->attr.name);
      if(TraceCode) emitComment("<- param");
      return;
      break;
    default:
      break;
  }
}

/* Procedure cGen recursively generates code by
 * tree traversal
 */
static void cGen( TreeNode * tree)
{ if (tree != NULL)
  { switch (tree->nodekind) {
      case StmtK:
        genStmt(tree);
        break;
      case ExpK:
        genExp(tree, FALSE);
        break;
      case DeclK:
        genDecl(tree);
        break;
      case ParamK:
        genParam(tree);
        break;
      default:
        break;
    }
    cGen(tree->sibling);
  }
}

/**********************************************/
/* the primary function of the code generator */
/**********************************************/
/* Procedure codeGen generates code to a code
 * file by traversal of the syntax tree. The
 * second parameter (codefile) is the file name
 * of the code file, and is used to print the
 * file name as a comment in the code file
 */
void codeGen(TreeNode * syntaxTree, char * codefile)
{  char * s = malloc(strlen(codefile)+7);
   strcpy(s,"File: ");
   strcat(s,codefile);
   emitComment("TINY Compilation to TM Code");
   emitComment(s);
   /* generate standard prelude */
   emitComment("Standard prelude:");
   emitRM("LD",sp,0,ac,"load maxaddress from location 0");
   emitRM("ST",ac,0,ac,"clear location 0");
   emitRM("LDA",fp,0,sp,"sp->fp");
   emitComment("End of standard prelude.");
   /* generate code for TINY program */
   scope="Global";
   cGen(syntaxTree);
   /* finish */
   emitComment("End of execution.");
   emitRO("HALT",0,0,0,"");
}

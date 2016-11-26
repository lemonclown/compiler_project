/****************************************************/
/* File: symtab.c                                   */
/* Symbol table implementation for the TINY compiler*/
/* (allows only one symbol table)                   */
/* Symbol table is implemented as a chained         */
/* hash table                                       */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"
#include "globals.h"

/* the hash function */
static int hash ( char * key )
{ int temp = 0;
  int i = 0;
  while (key[i] != '\0')
  { temp = ((temp << SHIFT) + key[i]) % SIZE;
    ++i;
  }
  return temp;
}

/* the hash table */
static ScopeList scopeStack[STACK];
static ScopeList scopelist[SIZE];
static FuncParam funclist[SIZE];
int scopeindex = 0;
int scopestack_i = 0;
int plindex = 0;
FuncParam createpl(char * name, TreeNode * tree)
{
  FuncParam newpl;

  newpl = (FuncParam) malloc(sizeof(struct FuncParamRec));
  newpl->name = name;
  newpl->treenode = tree;

  return newpl;
}

void push_pl(FuncParam pl)
{
  funclist[plindex++] = pl;
}

FuncParam getpl(char *name)
{
  int i;
  
  for(i=0;i<plindex;i++)
  { 
    if((strcmp(name,funclist[i]->name)==0))
      return funclist[i];
  }
  return NULL;
}

ScopeList topscope()
{ if(scopestack_i == 0)
    return NULL;
  return scopeStack[scopestack_i-1];
}

ScopeList createscope(char * name)
{
  ScopeList newScope;

  newScope = (ScopeList) malloc(sizeof(struct ScopeListRec));
  newScope->name = name;
  newScope->parent = topscope();
  scopelist[scopeindex++] = newScope;

  return newScope;
}

void push_scope(ScopeList scope)
{
  scopeStack[scopestack_i++] = scope;
}

void pop_scope()
{
  scopestack_i--;
}
/* Procedure st_insert inserts line numbers and
 * memory locations into the symbol table
 * loc = memory location is inserted only the
 * first time, otherwise ignored
 */
void st_insert( char * scope, char * name, ExpType type, int lineno, int loc )
{ int h = hash(name);
  ScopeList topsc = topscope();
  BucketList l =  topsc->bucket[h];
  while ((l != NULL) && (strcmp(name,l->name) != 0))
    l = l->next;
  if (l == NULL) /* variable not yet in table */
  { l = (BucketList) malloc(sizeof(struct BucketListRec));
    l->scope = scope;
    l->name = name;
    l->type = type;
    l->lines = (LineList) malloc(sizeof(struct LineListRec));
    l->lines->lineno = lineno;
    l->memloc = loc;
    l->lines->next = NULL;
    l->next = topsc->bucket[h];
    topsc->bucket[h] = l; }
  else /* found in table, so just add line number */
  { LineList t = l->lines;
    while(topsc != NULL)
    { printf("%s\n",l->name);
      l = topsc->bucket[h];
      while((l != NULL) && (strcmp(name,l->name) !=0))
        l=l->next;
      if( l != NULL){
        while (t->next != NULL) t = t->next;
        t->next = (LineList) malloc(sizeof(struct LineListRec));
        t->next->lineno = lineno;
        t->next->next = NULL;
        printf("tt\n");
        return;
      }
      else if ( topsc->parent == NULL) break;
      topsc = topsc->parent;
    }
    
  }
} /* st_insert */

void add_line(char * name, int lineno)
{
  int h = hash(name);
  ScopeList topsc = topscope();
  BucketList l =  topsc->bucket[h];
  while(topsc != NULL)
  { 
    l = topsc->bucket[h];
    while((l != NULL) && (strcmp(name,l->name) !=0))
      l=l->next;
    if( l != NULL){
      LineList t = l->lines;
      while (t->next != NULL) {
        if (t->next->lineno == lineno)
          return;
        t = t->next;
      }
      t->next = (LineList) malloc(sizeof(struct LineListRec));
      t->next->lineno = lineno;
      t->next->next = NULL;
      return;
    }
    else if ( topsc->parent == NULL) break;
    topsc = topsc->parent;
  }
}

/* Function st_lookup returns the memory 
 * location of a variable or -1 if not found
 */
int st_lookup ( char * scope, char * name )
{ int h = hash(name);
  ScopeList tscope = topscope();
  while(tscope!=NULL)
  {
    BucketList l =  tscope->bucket[h];
    while((l != NULL) && (strcmp(name,l->name) != 0))
      l = l->next;
    if (l != NULL) return l->memloc; 
    else if ( tscope->parent != NULL) tscope = tscope->parent;
    else return -1;
  }
}

ExpType type_lookup ( char * scope, char * name)
{ int h = hash(name);
  ScopeList tscope;
  int i;
  for(i=scopeindex-1;i>=0;i--){
    if((strcmp(scopelist[i]->name, scope) == 0))
    {
      tscope = scopelist[i];
      break;
    }
  }
  while(tscope!=NULL)
  {
    BucketList l =  tscope->bucket[h];
    while((l != NULL) && (strcmp(name,l->name) != 0))
      l = l->next;
    if (l != NULL) return l->type; 
    else if ( tscope->parent != NULL) tscope = tscope->parent;
    else {return -1;} 
  }
}


int st_lookup_cur ( char * scope, char * name)
{
  int h = hash(name);
  ScopeList tscope = topscope();
  BucketList l = tscope->bucket[h];
  while((l != NULL) && (strcmp(name,l->name) != 0))
    l = l->next;
  if (l != NULL) return l->memloc;
  else return -1;
}

ExpType sc_lookup ( char * name )
{ int h = hash(name);
  ScopeList scope = scopelist[0];
  BucketList bucket = scope->bucket[h];
  while((bucket!=NULL) && (strcmp(bucket->name,name) != 0))
  {
    printf("sc_lll : %s\n",bucket->name);
    bucket = bucket->next;
    if(strcmp(bucket->name,name)==0)
      return bucket->type;
  }
  if(bucket!=NULL) return bucket->type;
  return -1;
}

/* Procedure printSymTab prints a formatted 
 * listing of the symbol table contents 
 * to the listing file
 */
void printSymTab(FILE * listing)
{ printf("print\n\n");
  int i,j;
  char * type;
  for(i=0;i<scopeindex;i++)
  {
    ScopeList scope = scopelist[i];
    fprintf(listing, "%s \n",scope->name);
    fprintf(listing, "======================================================\n");
    fprintf(listing, "name    type          IDtype   loc     lines\n");
    fprintf(listing, "------------------------------------------------------\n");
    for(j=0;j<SIZE;j++)
    {
      BucketList bucket = scope->bucket[j];
      while(bucket!=NULL)
      {
        switch(bucket->type){
          case Integer:
            type = "Int";
            break;
          case Void:
            type = "Void";
            break;
          case IntegerArray:
            type = "IntegerArray";
            break;
          default:
            type = "???";
            break;
        }
        fprintf(listing,"%-8s",bucket->name);
        fprintf(listing,"%-15s",type);
        fprintf(listing,"%-8s", bucket->scope);
        fprintf(listing, "%-6d", bucket->memloc);
        LineList line = bucket->lines;
        while(line!=NULL){
          fprintf(listing,"%d ",line->lineno);
          line= line->next;
        }
        
        fprintf(listing, "\n");
    //    fprintf(listing, "%s      %-4s     %-8d     %-12d\n", bucket->name,type,bucket->lines->lineno,bucket->memloc);
        bucket = bucket->next;
      }
    }
    fprintf(listing, "\n\n");
  }
} /* printSymTab */

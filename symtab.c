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
static BucketList hashTable[SIZE];
static ScopeList scopeStack[STACK];
static ScopeList scopelist[SIZE];
int scopeindex = 0;
int scopestack_i = 0;

ScopeList topscope()
{
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
  printf("pushpush \n");
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
    while (t->next != NULL) t = t->next;
    t->next = (LineList) malloc(sizeof(struct LineListRec));
    t->next->lineno = lineno;
    t->next->next = NULL;
  }
} /* st_insert */

/* Function st_lookup returns the memory 
 * location of a variable or -1 if not found
 */
int st_lookup ( char * scope, char * name )
{ int h = hash(name);
  ScopeList tscope = topscope();
  while(tscope)
  {
    BucketList l =  tscope->bucket[h];
    while((l != NULL) && (strcmp(name,l->name) != 0))
      l = l->next;
    if (l != NULL) return l->memloc; 
    else if ( tscope->parent != NULL) tscope = tscope->parent;
    else return -1;
  }
}
/*
int sc_lookup ( char * name )
{
  int i;
  for(i=scopestack_i-1; i>=0; i--)
  {
    if(scopelist[i]->name == name)
      return i;
  }
  return -1;
  int h = hash(name);
  ScopeList l = scopeTable[h];
  while ((l != NULL) && (strcmp(name,l->name) != 0))
    l = l->parent;
  if (l == NULL) return -1;
  else return l->bucket;
}
*/
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
    fprintf(listing, "=============================\n");
    for(j=0;j<SIZE;j++)
    {
      BucketList bucket = scope->bucket[j];
      while(bucket!=NULL)
      {
        switch(bucket->type){
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
        fprintf(listing, "name   type   lines   loc   \n");
        fprintf(listing, "------------------------------\n");
        fprintf(listing, "%s      %s     %d     %d\n", bucket->name,type,bucket->lines->lineno,bucket->memloc);
        bucket = bucket->next;
      }
    }
    fprintf(listing, "\n\n");
  }
} /* printSymTab */

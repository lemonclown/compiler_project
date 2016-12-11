/****************************************************/
/* File: symtab.h                                   */
/* Symbol table interface for the TINY compiler     */
/* (allows only one symbol table)                   */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#ifndef _SYMTAB_H_
#define _SYMTAB_H_
#include "globals.h"

/* SIZE is the size of the hash table */
#define SIZE 211
#define STACK 211

char * scope_name;

/* SHIFT is the power of two used as multiplier
   in hash function  */
#define SHIFT 4

/* the list of line numbers of the source 
 * code in which a variable is referenced
 */
typedef struct LineListRec
   { int lineno;
     struct LineListRec * next;
   } * LineList;

/* The record in the bucket lists for
 * each variable, including name, 
 * assigned memory location, and
 * the list of line numbers in which
 * it appears in the source code
 */
typedef struct BucketListRec
   { char * scope;
     char * name;
     ExpType type;
     LineList lines;
     int memloc ; /* memory location for variable */
     int mloc;
     struct BucketListRec * next;
   } * BucketList;

/* ScopeListStructure */
typedef struct ScopeListRec
{ char *name;
  BucketList bucket[SIZE];
  struct ScopeListRec * parent;
  int paramNum;
  int varNum;
} * ScopeList;

typedef struct FuncParamRec
{
	char * name;
	TreeNode * treenode;
  int paramNum;
} * FuncParam;

ScopeList g_scope;
ScopeList scope_lookup( char * name );
/* Procedure st_insert inserts line numbers and
 * memory locations into the symbol table
 * loc = memory location is inserted only the
 * first time, otherwise ignored
 */
void st_insert( char * scope, char * name, ExpType type, int lineno, int loc, int mloc );

/* Function st_lookup returns the memory 
 * location of a variable or -1 if not found
 */
int st_lookup ( char * scope, char * name );

/* Procedure printSymTab prints a formatted 
 * listing of the symbol table contents 
 * to the listing file
 */
void printSymTab(FILE * listing);
FuncParam getpl(char *name);
FuncParam createpl(char * name, TreeNode * tree);
void push_pl();
ScopeList createscope(char * name);
void push_scope();
void pop_scope();
ExpType type_lookup ( char * scope, char * name);
int st_lookup_cur ( char * scope, char * name);
ExpType sc_lookup ( char * name );
void add_line(char * name, int lineno);

#endif

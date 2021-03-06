/****************************************************/
/* File: tiny.l                                     */
/* Lex specification for TINY                       */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

%{
#include "globals.h"
#include "util.h"
#include "scan.h"
/* lexeme of identifier or reserved word */
char tokenString[MAXTOKENLEN+1];
char numString[MAXTOKENLEN+1];
char tempString[MAXTOKENLEN+1];
char prevToken[MAXTOKENLEN+1]="\0";
%}

digit       [0-9]
number      {digit}+
letter      [a-zA-Z]
identifier  {letter}+
newline     \n
whitespace  [ \t]+

%%

"if"            {return IF;}
"else"          {return ELSE;}
"while"		{return WHILE;}
"int"		{return INT;}
"void"		{return VOID;}
"return"	{return RETURN;}
"="            {return ASSIGN;}
"=="             {return EQ;}
"<"             {return LT;}
"<="		{return LE;}
">"		{return GT;}
">="		{return GE;}
"!="		{return NE;}
","		{return COMMA;}
"["		{return LBRACE;}
"]"		{return RBRACE;}
"{"		{return LCURLY;}
"}"		{return RCURLY;}
"+"             {return PLUS;}
"-"             {return MINUS;}
"*"             {return TIMES;}
"/"             {return OVER;}
"("             {return LPAREN;}
")"             {return RPAREN;}
";"             {return SEMI;}
{number}        {return NUM;}
{identifier}    {return ID;}
{newline}       {lineno++;}
{whitespace}    {/* skip whitespace */}
"/*"            { char c;
    		  char l = '\0';
                  do { c = input();
                    if (c == EOF) break;
                    if (c == '\n') lineno++;
		    if (l == '*' && c == '/')break;
		    l=c;
                  } while (1);
                }
.               {return ERROR;}

%%

TokenType getToken(void)
{ static int firstTime = TRUE;
  TokenType currentToken;
  if (firstTime)
  { firstTime = FALSE;
    lineno++;
    yyin = source;
    yyout = listing;
  }
  currentToken = yylex();
  strncpy(tokenString,yytext,MAXTOKENLEN);
  if (TraceScan) {
    fprintf(listing,"\t%d: ",lineno);
    printToken(currentToken,tokenString);
  }
  return currentToken;
}


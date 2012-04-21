/**************************************
 * filename isql.l
 * scanner for SQL lex file
 * Copyright (c) 2011, RL. All rights reserved.
 * flex -o isql.lex.c isql.l
 **************************************/

%option noyywrap nodefault yylineno case-insensitive

%{
#include "isql.tab.h"
#include "isql_main.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
%}

%X COMMENT

%%

 /*KEYWORDS*/

CREATE { return CREATE; }
DATABASE { return DATABASE; }
DATABASES { return DATABASES; }
SHOW { return SHOW; }
USE { return USE; }
TABLE { return TABLE; }
TABLES { return TABLES; }
DROP { return DROP; }
INSERT { return INSERT; }
INTO { return INTO; }
VALUES { return VALUES; }
SELECT { return SELECT; }
FROM { return FROM; }
WHERE { return WHERE; }
AND { return AND; }
OR { return OR; }
DELETE { return DELETE; }
UPDATE { return UPDATE; }
SET { return SET; }
IF[ \t\n]+EXISTS {return IF_EXISTS;}
SCHEMA {return SCHEMA;}
INT { return INT; }
CHAR { return CHAR; }
ROLE { return ROLE; }
GRANT { return GRANT; }
TO { return TO; }
ON { return ON; }
REVOKE { return REVOKE; }
CASCADE { return CASCADE; }
WITH { return WITH; }
OPTION { return OPTION; }
EXIT { yyterminate(); }

 /*NUMBERS*/

-?[0-9]+ { yylval.intval = atoi(yytext); return INTNUM; } /*int*/
-?[0-9]+"."[0-9]* |
-?"."[0-9]+ |
-?[0-9]+E[-+]?[0-9]+ |
-?[0-9]+"."[0-9]*E[-+]?[0-9]+ |
-?"."[0-9]*E[-+]?[0-9]+ { yylval.floatval = atof(yytext); return APPROXNUM; } /*float*/

 /*BOOLEANS*/

TRUE    { yylval.intval = 1; return BOOL; }
FALSE   { yylval.intval = 1; return BOOL; }
UNKNOWN { yylval.intval = 1; return BOOL; }

 /*strings*/

'(\\.|''|[^'\n])*'  {
				yylval.strval = strdup(yytext);
				yylval.strval=(char *)(yylval.strval+1);			//去掉单引号
				yylval.strval[strlen(yylval.strval)-1]='\0';
			   	return STRING; }
[A-Za-z][A-Za-z0-9_]* { yylval.strval = strdup(yytext); return NAME; }

 /*OPERATORS*/

[-+*/(),.;] {return yytext[0]; }
"="     { yylval.subtok = 1; return CMP; }
"!="    { yylval.subtok = 2; return CMP; }
"<"     { yylval.subtok = 3; return CMP; }
">"     { yylval.subtok = 4; return CMP; }
"<="    { yylval.subtok = 5; return CMP; }
">="    { yylval.subtok = 6; return CMP; }

 /*COMMENT*/

"/*"    { BEGIN COMMENT; }
<COMMENT>"*/" ;
<COMMENT>.|\n ;
<COMMENT><<EOF>> { yyerror("unclosed comment"); }

[ \t\n]  /*white space*/
.       { yyerror("UNKNOW WORDS '%c'",*yytext); }

%%

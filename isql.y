/**************************************
 * filename isql.y
 * Parser for SQL yacc file
 * Copyright (c) 2011, RL. All rights reserved.
 * bison -d isql.y
 **************************************/

%{
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "isql_main.h"

extern int DT_HANDLE;
extern int pFList;
extern struct _fieldList FList[];

%}

%union {
	int intval;
	double floatval;
	char *strval;
	int subtok;
	struct _expr *Expr;
	struct ast_createD *createD; 
	struct ast_createDefinition *createDefinition;
	struct ast_createCol_list *createCol_list;
	struct ast_createTable *createTable;
	struct ast_wordList *wordList;
	struct ast_tableInto *tableInto;
	struct ast_valList *valList;
	struct ast_insertRecord *insertRecord;
	struct ast_selectRecord *selectRecord;
	struct ast_permission *permission;
}

/*names and literal values*/

%token <strval> NAME
%token <strval> STRING
%token <intval> INTNUM
%token <intval> BOOL
%token <floatval> APPROXNUM

/*operators and precedence levels*/

%left OR
%left AND
%left <subtok> CMP /* = != < > <= >= */
%left '+' '-'
%left '*' '/'
%nonassoc UMINUS

%token CREATE
%token DATABASE
%token DATABASES
%token SHOW
%token USE
%token TABLE
%token TABLES
%token DROP
%token INSERT
%token INTO
%token VALUES
%token SELECT
%token FROM
%token WHERE
%token ADD
%token OR
%token DELETE
%token UPDATE
%token SET
%token IF_EXISTS
%token SCHEMA
%token CMP
%token INT
%token CHAR
%token ROLE
%token GRANT
%token TO
%token ON
%token REVOKE
%token CASCADE
%token WITH
%token OPTION 


%type <intval> drop_obj show_obj data_type opt_length power power_lists
%type <createDefinition> create_definition
%type <createCol_list> create_col_list
%type <createTable> create_table_stmt
%type <wordList> insert_expr_list select_expr_list table_references users
%type <tableInto> table_into
%type <valList> insert_vals_list insert_vals
%type <insertRecord> insert_stmt
%type <Expr> expr opt_where
%type <strval> table_reference passwd
%type <selectRecord> select_stmt
%type <permission> usermanage_stmt

%start stmt_list

%%

/*expressions*/
expr: NAME				{ $$=malloc(sizeof(struct _expr));$$->type=0; $$->string=$1; FList[pFList].name=$1; pFList++; }
 /*	| NAME '.' NAME */
	| STRING			{ $$=malloc(sizeof(struct _expr));$$->type=1; $$->string=$1; }
	| INTNUM			{ $$=malloc(sizeof(struct _expr));$$->type=2; $$->intval=$1; }
 /*	| APPROXNUM 
	| BOOL */
	;

expr: '(' expr ')'		{ $$=$2; }
	| expr '+' expr		{ $1->intval=$1->intval+$3->intval; $$=$1; }
	| expr '-' expr		{ $1->intval=$1->intval-$3->intval; $$=$1; }
	| expr '*' expr		{ $1->intval=$1->intval*$3->intval; $$=$1; }
	| expr '/' expr
 /*	| '-' expr %prec UMINUS */
	| expr CMP expr		{ $$=malloc(sizeof(struct _expr)); $$->type=$2+2; $$->left=$1; $$->right=$3; }
	| expr AND expr		{ $$=malloc(sizeof(struct _expr)); $$->type=9; $$->left=$1; $$->right=$3; }
	| expr OR  expr		{ $$=malloc(sizeof(struct _expr)); $$->type=10; $$->left=$1; $$->right=$3; }
	;

/*start*/
stmt_list:
	| stmt_list ';' { printf("> "); }
	| stmt_list stmt ';' { printf("> "); }
	| stmt_list error { yyclearin; yyerrok; printf("> "); }
	;

/*stmt:select statement*/
stmt: select_stmt {
			//	if (DT_HANDLE==-1) {yyerror("Please choose a DATABASE !");}
			//	else {
			//		select_Record($1);
			//	}
			}
	;

select_stmt: SELECT select_expr_list FROM table_references opt_where {
					if (DT_HANDLE==-1) {yyerror("Please choose a DATABASE !");}
					else {
						$$=malloc(sizeof(struct ast_selectRecord));
						$$->wordlist=$2;
						$$->where_tree=$5;
						while ($4!=NULL){
							$$->name=$4->name;
							select_Record($$);
							$4=$4->next;
						}
					}
					pFList=0;
				}
	;

select_expr_list: NAME {
					$$=malloc(sizeof(struct ast_wordList));
					$$->name=$1;
					$$->next=NULL;
				}
	| select_expr_list ',' NAME { $$=add_wordList($1,$3); }
	| '*' { $$=NULL; }
	;

table_references: table_reference {
					$$=malloc(sizeof(struct ast_wordList));
					$$->name=$1;
					$$->next=NULL;
				}
	| table_references ',' table_reference { $$=add_wordList($1,$3); }
	;

table_reference: NAME { $$=$1; }
	;

opt_where:		 { $$=NULL; }
	| WHERE expr { $$=$2;   }
	;

/*stmt:delete statement*/

stmt: delete_stmt
	;

delete_stmt: DELETE FROM table_references opt_where
	;

/*stmt:insert statement*/

stmt: insert_stmt {
				if (DT_HANDLE==-1) {yyerror("Please choose a DATABASE !");}
				else {
					insert_Record($1);
				}
			}
	;

insert_stmt: INSERT INTO table_into VALUES insert_vals_list {
				$$=malloc(sizeof(struct ast_insertRecord));
				$$->tableinto=$3;
				$$->vallist=$5;
		   }
	;

table_into: NAME { $$=malloc(sizeof(struct ast_tableInto)); $$->name=$1; $$->wordlist=NULL; }
	| NAME '(' insert_expr_list ')' {
			$$=malloc(sizeof(struct ast_tableInto));
			$$->name=$1;
			$$->wordlist=$3;
		}
	;

insert_expr_list: NAME {
					$$=malloc(sizeof(struct ast_wordList));
					$$->name=$1;
					$$->next=NULL;
				}
	| insert_expr_list ',' NAME { $$=add_wordList($1,$3); }
	;

insert_vals_list: '(' insert_vals ')' { $$=$2; }
	;

insert_vals: expr {
				$$=malloc(sizeof(struct ast_valList));
				$$->expr=$1;
				$$->next=NULL;
		   }
	| insert_vals ',' expr { $$=add_Vals($1,$3); }
	;

/*stmt:update statement*/

stmt: update_stmt
	;

update_stmt: UPDATE table_references 
		     SET update_asgn_list
		     opt_where
	;

update_asgn_list: NAME CMP expr
 /*	| NAME '.' NAME CMP expr */
	| update_asgn_list ',' NAME CMP expr
 /*	| update_asgn_list ',' NAME '.' NAME CMP expr */
	;

/*stmt:create database*/

stmt: create_database_stmt
	;

create_database_stmt: CREATE DATABASE opt_if_not_exists NAME
					{
						FILE *fin;
						if(!(fin=fopen("isql.db","r+"))){
							printf("cannot open file isql.db\n");
						}
						int num=atoi(read_D(fin,0,8));
						int i;
						int flag=0;
						char *tmp;
						for (i=0;i<num;i++){
							tmp=read_D(fin,D_PAGE*PAGE_SIZE+i*D_HEAD_SIZE,32);
							if (!strcmp(tmp,$4)) {flag=1;yyerror("Database:%s alreadly exist !",$4);}
						}
						if (!flag){
							fill_D(fin,D_PAGE*PAGE_SIZE+num*D_HEAD_SIZE,D_HEAD_SIZE,null);
							write_D(fin,D_PAGE*PAGE_SIZE+num*D_HEAD_SIZE,$4);
							fseek(fin,D_PAGE*PAGE_SIZE+num*D_HEAD_SIZE+D_NAME_SIZE,SEEK_SET);
							fprintf(fin,"%d",0);
							num=num+1;
							fseek(fin,0,SEEK_SET);
							fprintf(fin,"%d",num);
							fclose(fin);
						}
					}
	| CREATE SCHEMA opt_if_not_exists NAME
	;

opt_if_not_exists:
	| IF_EXISTS
	;
/*stmt:create table*/

stmt: create_table_stmt {
			if (DT_HANDLE==-1) {yyerror("Please choose a DATABASE !");}
			else {
				create_Table($1);
			}
		}
	;

create_table_stmt: CREATE TABLE opt_if_not_exists NAME '(' create_col_list ')' {
					$$=malloc(sizeof(struct ast_createTable));
					$$->name=$4;
					$$->col_list=$6;
				 }
/*	| CREATE TABLE opt_if_not_exists NAME '.' NAME '(' create_col_list ')'*/
	;

create_col_list: create_definition { $$=malloc(sizeof(struct ast_createCol_list));
									 $$->createDefinition=$1;
									 $$->next=NULL;
								   }
	| create_col_list ',' create_definition { $$=add_createCol_list($1,$3); }
	;

create_definition: NAME data_type opt_length {
					$$=new_CDefinition($1,$2,$3);
				}
	;

data_type: INT { $$=0; }
	| CHAR	   { $$=1; }
	;

opt_length:				{ $$=8;  }
	| '(' INTNUM ')'	{ $$=$2; }
	;

/*stmt: show statement*/

stmt: show_stmt
	;

show_stmt: SHOW show_obj {
				if (!$2){
					FILE *fin;
					if(!(fin=fopen("isql.db","r+"))){
						printf("cannot open file isql.db\n");
					}
					int num=atoi(read_D(fin,0,8));
					int i;
					char *tmp;
					for (i=0;i<num;i++){
						tmp=read_D(fin,D_PAGE*PAGE_SIZE+i*D_HEAD_SIZE,32);
						if (tmp[0]!=0x00) printf("%s\t",tmp);
					}
					printf("\n");
					fclose(fin);
				}
				else {
					if (DT_HANDLE==-1) yyerror("Please choose a database !\n");
					else {
						FILE *fin;
						if(!(fin=fopen("isql.db","r+"))){
							printf("cannot open file isql.db\n");
						}
						char *tmp;
						int i;
						int offset=D_PAGE*PAGE_SIZE+DT_HANDLE*D_HEAD_SIZE;
						int table_num=atoi(read_D(fin,offset+D_NAME_SIZE,32));
						offset=T_LIST_PAGE*PAGE_SIZE+DT_HANDLE*T_LIST_SIZE;
						for (i=0;i<table_num;i++){
							tmp=read_D(fin,offset+i*T_HEAD_SIZE,T_NAME_SIZE);
							if (tmp[0]!=0x00) printf("%s\t",tmp);
						}
						printf("\n");
						fclose(fin);
					}
				}
			}
	;

show_obj: DATABASES { $$=0; }
	| TABLES		{ $$=1; }
	;

/*stmt: drop statement*/

stmt: DROP drop_obj NAME {
			if (!$2){  /* drop database */
				FILE *fin;
				if(!(fin=fopen("isql.db","r+"))){
					printf("cannot open file isql.db\n");
				}
				int num=atoi(read_D(fin,0,8));
				int i;
				int flag=0;
				char *tmp;
				for (i=0;i<num;i++){
					tmp=read_D(fin,D_PAGE*PAGE_SIZE+i*D_HEAD_SIZE,32);
					if (!strcmp(tmp,$3)) {flag=1;fill_D(fin,D_PAGE*PAGE_SIZE+i*D_HEAD_SIZE,D_HEAD_SIZE,null);break;}
				}
				if (!flag) yyerror("Database:%s didn't exist!\n",$3);
				fclose(fin);
			}
			else {
				FILE *fin;
				if(!(fin=fopen("isql.db","r+"))){
					printf("cannot open file isql.db\n");
				}
				if (DT_HANDLE==-1) yyerror("Please choose a database !\n");
				else{
					char *tmp;
					int i;
					int flag=0;
					int offset=D_PAGE*PAGE_SIZE+DT_HANDLE*D_HEAD_SIZE;
					int table_num=atoi(read_D(fin,offset+D_NAME_SIZE,D_TABLE_NUM));
					offset=T_LIST_PAGE*PAGE_SIZE+DT_HANDLE*T_LIST_SIZE;
					for (i=0;i<table_num;i++){
						tmp=read_D(fin,offset+i*T_HEAD_SIZE,T_NAME_SIZE);
						if (!strcmp(tmp,$3)) {flag=1;fill_D(fin,offset+i*T_HEAD_SIZE,T_HEAD_SIZE,null);break;}
					}
					if (!flag) yyerror("Table:%s didn't exist!\n",$3);
				}
				
				fclose(fin);
			}
		}
	;

drop_obj: DATABASE  { $$=0; }
	| TABLE			{ $$=1; }
	;

/*stmt: use statement*/

stmt: USE NAME {
			FILE *fin;
			if(!(fin=fopen("isql.db","r+"))){
				printf("cannot open file isql.db\n");
			}
			int num=atoi(read_D(fin,0,8));
			int i;
			int flag=0;
			char *tmp;
			for (i=0;i<num;i++){
				tmp=read_D(fin,D_PAGE*PAGE_SIZE+i*D_HEAD_SIZE,32);
				if (!strcmp(tmp,$2)) {flag=1;DT_HANDLE=i;break;}
			}
			if (!flag) {
				printf("Database:%s didn't exist!\n",$2);
				DT_HANDLE=-1;
			}
		}
	;

/*stmt: usermanage statement*/

stmt: usermanage_stmt {
			if ($1!=NULL) user_Manage($1);
		}
	;

usermanage_stmt: CREATE ROLE NAME passwd { $$=NULL; create_Role($3,$4); }
	| GRANT power_lists ON TABLE table_references TO users {
			$$=malloc(sizeof(struct ast_permission));
			$$->flag=0;
			$$->permission=$2;
			$$->tablelist=$5;
			$$->userlist=$7;
		}
 /*	| GRANT users TO users */
	| REVOKE power_lists ON TABLE table_references FROM users {
			$$=malloc(sizeof(struct ast_permission));
			$$->flag=1;
			$$->permission=$2;
			$$->tablelist=$5;
			$$->userlist=$7;
		}
	;

passwd: NAME	{ $$=$1; }
	;

power_lists: power { $$=0|$1; }
	| power_lists ',' power { $$=$1|$3; }
	;

power: SELECT	{ $$=1; }		/*0001*/
	|  INSERT	{ $$=2; }		/*0010*/
	|  UPDATE	{ $$=4; }		/*0100*/
	|  DELETE	{ $$=8; }		/*1000*/
	;

users: NAME {
			$$=malloc(sizeof(struct ast_wordList));
			$$->name=$1;
			$$->next=NULL;
		}
	| users ',' NAME { $$=add_wordList($1,$3); }
	;

%%



/**************************************
 * filename isql.h
 * header for SQL head file
 * Copyright (c) 2011, RL. All rights reserved.
 * CFLAGS = isql_main.h
 * ************************************/

#ifndef _ISQL_H
#define _ISQL_H

#define PAGE_SIZE 2048

//db :0页：8位当前数据库数+12位当前字段表地址+8位用户数
//dat:0页：16位字段数据地址

#define D_PAGE 1					//起始页，最多32个数据库
#define D_HEAD_SIZE  64				//
#define D_NAME_SIZE  32				//数据库名
#define D_TABLE_NUM  32				//>=数据库table数(最多16个)

#define T_LIST_PAGE  2				//起始页，2-17页
#define T_LIST_SIZE  1024
#define T_HEAD_SIZE  64
#define T_NAME_SIZE  32
#define T_PAGE		 12				//>=18  $$-18就是用户表的偏移量
#define T_USERNUM	 4				//一个table最多16个用户
#define T_COUNT		 11				//最多1<<12条记录
#define T_WORDS      5				//最多1<<6个字段

#define WORD_SIZE    64				//最多32个字段
#define W_NAME_SIZE  32				//字段名
#define W_TYPE		 8				//字段类型 0:int 1:char 其他未编码
#define W_LENGTH	 8				//字段长度 最长1<<8位
#define W_PAGE		 16				//字段存储位置偏移 2字节和dat连接表对应

#define U_PAGE		 1				//dat文件中
#define U_HEAD_SIZE  64				//最多32个用户
#define U_NAME		 24
#define U_NUM		 8				//用户编号				
#define U_PASSWD	 32				//md5

#define P_SELECT	 1
#define P_INSERT	 2
#define P_UPDATE	 4
#define P_DELETE	 8

#define U_LISTPAGE   2				//2-65,64页 64*8条记录 对应table数32*16
#define U_LISTSIZE	 256			//最多16个用户 对应T_USERNUM
#define U_WORDSIZE   16				//8位用户编号+4位权限+4位可扩展
#define U_POWER		 4


static char null=(char)0x00;

void yyerror(char *s, ...);
void emit(char *s, ...);
//void shell_exec(char *cmd); popen(cmd,"r"); echo fuckyou | md5sum
char *get_MD5(char *str);
char *read_D(FILE *fin, int offset, int length);
void write_D(FILE *fin, int offset, char *str);
void fill_D(FILE *fin, int offset, int length, char t);

struct ast_createD{
	char *D_name;
};

struct ast_createDefinition{
	char *name;
	int data_type;
	int opt_length;
};

struct ast_createCol_list{
	struct ast_createDefinition *createDefinition;
	struct ast_createCol_list *next;
};

struct ast_createTable{
	char *name;
	struct ast_createCol_list *col_list;
};

struct ast_wordList{
	char *name;
	struct ast_wordList *next;
};

struct ast_valList{
	struct _expr *expr;
	struct ast_valList *next;
};

struct ast_tableInto{
	char *name;
	struct ast_wordList *wordlist;
};

struct ast_insertRecord{
	struct ast_tableInto *tableinto;
	struct ast_valList *vallist;
};

struct ast_selectRecord{
	char *name;
	struct ast_wordList *wordlist;
	struct _expr *where_tree;
};

struct ast_permission{
	int flag; //0:grant 1:revoke
	int permission;
	struct ast_wordList *tablelist;
	struct ast_wordList *userlist;
};

struct _expr{
	int type;					//0:name 1:string 2:intnum 'a':and 'o':or 3-8 = != < > <= >=
	char *string;
	int intval;
	int judge;					//0 false 1 true -1 null
	struct _expr *left,*right;
};

struct _table{
	char *name;
	char *database;
	int D_HANDLE;
	int T_HANDLE;
	int count;
	int words;
	struct _fieldList **list;
};

struct _fieldList{ 
	char *name;
	int type;
	int length;
	int page;
	int intval;
	char *charval;
};

struct ast_createDefinition *new_CDefinition(char *name, int data_type, int opt_length);
struct ast_createCol_list *add_createCol_list(struct ast_createCol_list *list,struct ast_createDefinition *nextDefinition);
void create_Table(struct ast_createTable *table);
struct ast_valList *add_Vals(struct ast_valList *list,struct _expr *nextExpr);

struct ast_wordList *add_wordList(struct ast_wordList *list,char *nextname);
void insert_Record(struct ast_insertRecord *record);
void select_Record(struct ast_selectRecord *record);

void create_Role(char *username,char *passwd);
void user_Manage(struct ast_permission *pchange);

#endif

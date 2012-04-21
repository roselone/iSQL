/**************************************
 * filename isql_main.c
 * c file for SQL main func
 * Copyright (c) 2011, RL. All rights reserved.
 * cc -c -o isql_main.o isql_main.c
 * ************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include "isql_main.h"

int DT_HANDLE;
int USER_NUM;
struct _fieldList FList[50];
int pFList;

char* read_D(FILE *fin,int offset,int length){
	char *tmp=(char*)malloc(sizeof(char)*length);
	fseek(fin,offset,SEEK_SET);
	fread(tmp,sizeof(char),length,fin);
	return tmp;
}

void write_D(FILE *fin,int offset,char *str){
	fseek(fin,offset,SEEK_SET);
	int num=fwrite(str,sizeof(char),strlen(str),fin);
	return ;
}

void fill_D(FILE *fin, int offset, int length, char t){
	int i;
	fseek(fin,offset,SEEK_SET);
	for(i=0;i<length;i++)
		fprintf(fin,"%c",t);
	return;
}

void emit(char *s, ...){
	extern yylineno;
	va_list ap;
	va_start(ap,s);

	printf("rpn: ");
	vfprintf(stdout,s,ap);
	printf("\n");
}

void yyerror (char *s, ...){
	extern yylineno;
	va_list ap;
	va_start(ap,s);
	fprintf(stderr,"%d:error: ",yylineno);
	vfprintf(stderr,s,ap);
	fprintf(stderr,"\n");
}

int judge(FILE *fin,FILE *fout,char *tname,int power){ //还未加入使用
	int offset=D_PAGE*PAGE_SIZE+DT_HANDLE*D_HEAD_SIZE;
	int table_num=atoi(read_D(fin,offset+D_NAME_SIZE,32)); //D中现有table数
	offset=T_LIST_PAGE*PAGE_SIZE+DT_HANDLE*T_LIST_SIZE;
	int i,u_page,u_num;
	for (i=0;i<table_num;i++){
		if (!strcmp(tname,read_D(fin,offset+i+T_HEAD_SIZE,T_NAME_SIZE))){
			u_page=atoi(read_D(fin,offset+i*T_HEAD_SIZE+T_NAME_SIZE,T_PAGE))-18;
			u_num=atoi(read_D(fin,offset+i*T_HEAD_SIZE+T_NAME_SIZE+T_PAGE,T_USERNUM));
			break;
		}
	}
	offset=U_LISTPAGE*PAGE_SIZE+u_page*U_LISTSIZE;
	for (i=0;i<u_num;i++){
		if (USER_NUM==atoi(read_D(fout,offset,U_NUM))){
			printf("user_num:%d\n",USER_NUM);
			int org=atoi(read_D(fout,offset+U_NUM,U_POWER));
			printf("org:%d\n",org);
			if (org & power !=0) return 1;
			break;
		}
	}
	return 0;
}

struct ast_createDefinition *new_CDefinition(char *name, int data_type, int opt_length){
	struct ast_createDefinition *tmp = malloc(sizeof(struct ast_createDefinition));
	tmp->name=name;
	tmp->data_type=data_type;
	tmp->opt_length=opt_length;
	return tmp;
}

struct ast_createCol_list *add_createCol_list(struct ast_createCol_list *list,struct ast_createDefinition *nextDefinition){
	struct ast_createCol_list *tmp=list;
	while (tmp->next!=NULL) tmp=tmp->next;
	struct ast_createCol_list *added=malloc(sizeof(struct ast_createCol_list));
	added->createDefinition=nextDefinition;
	added->next=NULL;
	tmp->next=added;
	return list;
}

void create_Table(struct ast_createTable *table){
	FILE *fin,*fout;
	if (!(fin=fopen("isql.db","r+"))){
		printf("cannot open file isql.db\n");
	}
	if (!(fout=fopen("isql.dat","r+"))){
		printf("cannot open file isql.dat!\n");
	}
	int offset=D_PAGE*PAGE_SIZE+DT_HANDLE*D_HEAD_SIZE;
	int table_num=atoi(read_D(fin,offset+D_NAME_SIZE,32)); //D中现有table数
	fseek(fin,offset+D_NAME_SIZE,SEEK_SET);
	fprintf(fin,"%d",table_num+1);			//下一个table

	offset=T_LIST_PAGE*PAGE_SIZE+DT_HANDLE*T_LIST_SIZE+table_num*T_HEAD_SIZE;
	int tmpoff=offset;
	fill_D(fin,offset,T_HEAD_SIZE,null);    //初始化为空
	write_D(fin,offset,table->name);
	int num=atoi(read_D(fin,8,12));			//字段表地址页
	fseek(fin,offset+T_NAME_SIZE,SEEK_SET);
	fprintf(fin,"%d",num);
	fseek(fin,offset+T_NAME_SIZE+T_PAGE+T_USERNUM,SEEK_SET);
	fprintf(fin,"%d",0);							//初始表中没有记录
	fseek(fin,8,SEEK_SET);
	fprintf(fin,"%d",num+1);				//下一个字段表

	/*建立用户表*/
	if (USER_NUM==0) { fseek(fin,offset+T_NAME_SIZE+T_PAGE,SEEK_SET);fprintf(fin,"%d",1); }
	else {fseek(fin,offset+T_NAME_SIZE+T_PAGE,SEEK_SET);fprintf(fin,"%d",2); } 
	offset=U_LISTPAGE*PAGE_SIZE+(num-18)*U_LISTSIZE;
	fseek(fout,offset,SEEK_SET);			//admin用户
	fprintf(fout,"%d",0);
	fseek(fout,offset+U_NUM,SEEK_SET);
	fprintf(fout,"%d",15);
	if (USER_NUM!=0){
		offset=offset+U_WORDSIZE;
		fseek(fout,offset,SEEK_SET);
		fprintf(fout,"%d",USER_NUM);
		fseek(fout,offset+8,SEEK_SET);
		fprintf(fout,"%d",15);
	}

	struct ast_createCol_list *tmplist=table->col_list;
	offset=num*PAGE_SIZE;
	int word=0;
	write_D(fin,offset+word*WORD_SIZE,tmplist->createDefinition->name);
	fseek(fin,offset+word*WORD_SIZE+W_NAME_SIZE,SEEK_SET);
	fprintf(fin,"%d",tmplist->createDefinition->data_type);
	fseek(fin,offset+word*WORD_SIZE+W_NAME_SIZE+W_TYPE,SEEK_SET);
	fprintf(fin,"%d",tmplist->createDefinition->opt_length);
	fseek(fin,offset+word*WORD_SIZE+W_NAME_SIZE+W_TYPE+W_LENGTH,SEEK_SET);
	num=atoi(read_D(fout,0,8));  //从16页开始
	fprintf(fin,"%d",num);
	while (tmplist->next!=NULL){
		tmplist=tmplist->next;
		word++;num++;
		write_D(fin,offset+word*WORD_SIZE,tmplist->createDefinition->name);
		fseek(fin,offset+word*WORD_SIZE+W_NAME_SIZE,SEEK_SET);
		fprintf(fin,"%d",tmplist->createDefinition->data_type);
		fseek(fin,offset+word*WORD_SIZE+W_NAME_SIZE+W_TYPE,SEEK_SET);
		fprintf(fin,"%d",tmplist->createDefinition->opt_length);
		fseek(fin,offset+word*WORD_SIZE+W_NAME_SIZE+W_TYPE+W_LENGTH,SEEK_SET);
	
		fprintf(fin,"%d",num);
	}
	fseek(fin,tmpoff+T_NAME_SIZE+T_PAGE+T_USERNUM+T_COUNT,SEEK_SET);
	fprintf(fin,"%d",word+1);			//字段数
	fseek(fout,0,SEEK_SET);
	fprintf(fout,"%d",num+1);
	fclose(fin);
	fclose(fout);
	return ;
}

struct ast_valList *add_Vals(struct ast_valList *list,struct _expr *nextExpr){
	struct ast_valList *tmp=list;
	while (tmp->next!=NULL) tmp=tmp->next;
	struct ast_valList *added=malloc(sizeof(struct ast_valList));
	added->expr=nextExpr;
	added->next=NULL;
	tmp->next=added;
	return list;
}

struct ast_wordList *add_wordList(struct ast_wordList *list, char *nextname){
	struct ast_wordList *tmp=list;
	while (tmp->next!=NULL) tmp=tmp->next;
	struct ast_wordList *added=malloc(sizeof(struct ast_wordList));
	added->name=nextname;
	added->next=NULL;
	tmp->next=added;
	return list;
}

void insert_Record(struct ast_insertRecord *record){
	struct ast_tableInto *tableinto=record->tableinto;
	struct ast_valList *vallist=record->vallist;

	FILE *fin,*fout;
	if (!(fin=fopen("isql.db","r+"))){
		printf("cannot open file isql.db\n");
	}
	if (!(fout=fopen("isql.dat","r+"))){
		printf("cannot open file isql.dat!\n");
	}

//	if (!judge(fin,fout,tableinto->name,P_INSERT)) goto end_insert;

	struct _table table;
	table.name=tableinto->name;
	table.D_HANDLE=DT_HANDLE;

	char *tmp;
	int i;
	int flag=0;
	int offset=D_PAGE*PAGE_SIZE+DT_HANDLE*D_HEAD_SIZE;
	int table_num=atoi(read_D(fin,offset+D_NAME_SIZE,D_TABLE_NUM));
	offset=T_LIST_PAGE*PAGE_SIZE+DT_HANDLE*T_LIST_SIZE;
	for (i=0;i<table_num;i++){
		tmp=read_D(fin,offset+i*T_HEAD_SIZE,T_NAME_SIZE);
		if (!strcmp(tmp,tableinto->name)){
			flag=1;
			fseek(fin,offset+i*T_HEAD_SIZE+T_NAME_SIZE,SEEK_SET);
			table.T_HANDLE=atoi(read_D(fin,offset+i*T_HEAD_SIZE+T_NAME_SIZE,T_PAGE));
			table.count=atoi(read_D(fin,offset+i*T_HEAD_SIZE+T_NAME_SIZE+T_PAGE,T_COUNT));
			fill_D(fin,offset+i*T_HEAD_SIZE+T_NAME_SIZE+T_PAGE,T_COUNT,null);
			fseek(fin,offset+i*T_HEAD_SIZE+T_NAME_SIZE+T_PAGE,SEEK_SET);
			fprintf(fin,"%d",table.count+1);
			table.words=atoi(read_D(fin,offset+(i+1)*T_HEAD_SIZE-T_WORDS,T_WORDS));
			break;
		}
	}
	if (!flag) { yyerror("Table:%s didn't exist!\n",tableinto->name); return ;}

	table.list=(struct _fieldList **)malloc(sizeof(struct _fieldList *)*table.words);
	offset=table.T_HANDLE*PAGE_SIZE;
	for (i=0;i<table.words;i++){
		table.list[i]=(struct _fieldList *)malloc(sizeof(struct _fieldList));
		table.list[i]->name=read_D(fin,offset+i*WORD_SIZE,W_NAME_SIZE);
		table.list[i]->type=atoi(read_D(fin,offset+i*WORD_SIZE+W_NAME_SIZE,W_TYPE));
		table.list[i]->length=atoi(read_D(fin,offset+i*WORD_SIZE+W_NAME_SIZE+W_TYPE,W_LENGTH));
		table.list[i]->page=atoi(read_D(fin,offset+i*WORD_SIZE+W_NAME_SIZE+W_TYPE+W_LENGTH,W_PAGE));
	}

	struct ast_wordList *wordlist=tableinto->wordlist;
	if (wordlist==NULL){  //按顺序插入
		for (i=0;i<table.words;i++){
			fseek(fout,table.list[i]->page*PAGE_SIZE+table.count*table.list[i]->length,SEEK_SET);
			if (!table.list[i]->type)
				fprintf(fout,"%d",vallist->expr->intval);
			else fprintf(fout,"%s",vallist->expr->string);
			vallist=vallist->next;
		}
	}
	else{
		int **a=malloc(sizeof(int*)*table.words);
		for (i=0;i<table.words;i++){a[i]=malloc(sizeof(int));(*a[i])=0;}
		while (wordlist!=NULL){
			for (i=0;i<table.words;i++){
				if (!strcmp(table.list[i]->name,wordlist->name)){
					(*a[i])=1;
					fseek(fout,table.list[i]->page*PAGE_SIZE+table.count*table.list[i]->length,SEEK_SET);
					//printf("%s:",table.list[i]->name);
					if (!table.list[i]->type){
						fprintf(fout,"%d",vallist->expr->intval);
					//	printf("%d\n",vallist->expr->intval);
					}
					else {
						fprintf(fout,"%s",vallist->expr->string);
					//	printf("%s\n",vallist->expr->string);
					}
					break;
				}
			}
			wordlist=wordlist->next;
			vallist=vallist->next;
		}
		for (i=0;i<table.words;i++){
			if (!(*a[i])){
				fseek(fout,table.list[i]->page*PAGE_SIZE+table.count*table.list[i]->length,SEEK_SET);
				fprintf(fout,"%s","NULL");
			}
		}
	}
	end_insert:
	fclose(fin);
	fclose(fout);	
}

int judge_Where(struct _expr *tree){
	if (tree==NULL || tree->type<3) return 1;
	int i1,i2;
	char *s1,*s2;
	int type;
	if (tree->type>2 && tree->type<9) {
		switch (tree->left->type){ 
			case 0:if(FList[pFList].type==0) {
					   type=0;i1=FList[pFList].intval;
				   }
				   else { type=1; s1=FList[pFList].charval; }
				   pFList++;
				   break;
			case 1:type=1; s1=tree->left->string; break;
			case 2:type=0; i1=tree->left->intval; break;
		}
		switch (tree->right->type){ 
			case 0:if(FList[pFList].type==0) i2=FList[pFList].intval;
					   else s2=FList[pFList].charval;
				   pFList++;
				   break;
			case 1:s2=tree->right->string; break;
			case 2:i2=tree->right->intval; break;
		}
	}
	if (tree->type==9 || tree->type==10){
		i1=judge_Where(tree->left);
		i2=judge_Where(tree->right);
	}
	switch (tree->type){
		case 3:if (!type) tree->judge=(i1==i2?1:0);
				   else tree->judge=(strcmp(s1,s2)==0?1:0);
			   break;
		case 4:if (!type) tree->judge=(i1!=i2?1:0);
				   else tree->judge=(strcmp(s1,s2)!=0?1:0);
			   break;
		case 5:tree->judge=(i1<i2)?1:0;break;
		case 6:tree->judge=(i1>i2)?1:0;break;
		case 7:tree->judge=(i1<=i2)?1:0;break;
		case 8:tree->judge=(i1>=i2)?1:0;break;
		case 9:tree->judge=i1 & i2; break;
		case 10:tree->judge=i1 | i2; break;
	}
	return tree->judge;
}

void select_Record(struct ast_selectRecord *record){
	FILE *fin,*fout;
	if (!(fin=fopen("isql.db","r+"))){
		printf("cannot open file isql.db\n");
	}
	if (!(fout=fopen("isql.dat","r+"))){
		printf("cannot open file isql.dat!\n");
	}
//	if (!judge(fin,fout,record->name,P_SELECT)) goto end_select;

	struct _table table;
	table.name=record->name;
	table.D_HANDLE=DT_HANDLE;


	char *tmp;
	int i;
	int flag=0;
	int offset=D_PAGE*PAGE_SIZE+DT_HANDLE*D_HEAD_SIZE;
	int table_num=atoi(read_D(fin,offset+D_NAME_SIZE,D_TABLE_NUM));
	offset=T_LIST_PAGE*PAGE_SIZE+DT_HANDLE*T_LIST_SIZE;
	for (i=0;i<table_num;i++){
		tmp=read_D(fin,offset+i*T_HEAD_SIZE,T_NAME_SIZE);
		if (!strcmp(tmp,table.name)){
			flag=1;
			fseek(fin,offset+i*T_HEAD_SIZE+T_NAME_SIZE,SEEK_SET);
			table.T_HANDLE=atoi(read_D(fin,offset+i*T_HEAD_SIZE+T_NAME_SIZE,T_PAGE));
			table.count=atoi(read_D(fin,offset+i*T_HEAD_SIZE+T_NAME_SIZE+T_PAGE,T_COUNT));
			table.words=atoi(read_D(fin,offset+(i+1)*T_HEAD_SIZE-T_WORDS,T_WORDS));
			break;
		}
	}
	if (!flag) { yyerror("Table:%s didn't exist!\n",table.name); return ;}

	table.list=(struct _fieldList **)malloc(sizeof(struct _fieldList *)*table.words);
	offset=table.T_HANDLE*PAGE_SIZE;
	for (i=0;i<table.words;i++){
		table.list[i]=(struct _fieldList *)malloc(sizeof(struct _fieldList));
		table.list[i]->name=read_D(fin,offset+i*WORD_SIZE,W_NAME_SIZE);
		table.list[i]->type=atoi(read_D(fin,offset+i*WORD_SIZE+W_NAME_SIZE,W_TYPE));
		table.list[i]->length=atoi(read_D(fin,offset+i*WORD_SIZE+W_NAME_SIZE+W_TYPE,W_LENGTH));
		table.list[i]->page=atoi(read_D(fin,offset+i*WORD_SIZE+W_NAME_SIZE+W_TYPE+W_LENGTH,W_PAGE));
	}
	
	struct ast_wordList *wordlist=record->wordlist;
	int j;
	if (wordlist==NULL){
		for (i=0;i<table.words;i++) printf("%-16s",table.list[i]->name);
		printf("\n---------------------------------------------------\n");
		for (j=0;j<table.count;j++){
			flag=0;
			int flag2=0;
			int k;
			for (k=0;k<pFList;k++){
				for (i=0;i<table.words;i++){
					if(!strcmp(FList[k].name,table.list[i]->name)){
						tmp=read_D(fout,table.list[i]->page*PAGE_SIZE+j*table.list[i]->length,table.list[i]->length);  //null没有处理
						FList[k].type=table.list[i]->type;
						if (tmp[0]!=0x00) {
							flag2=1;
							if (!FList[k].type) FList[k].intval=atoi(tmp);
							else FList[k].charval=tmp;
						}
						break;
					}
				}
			}
			if (pFList==0) flag2=1;
			pFList=0;
			if (flag2 && judge_Where(record->where_tree)){
				for (i=0;i<table.words;i++){
					tmp=read_D(fout,table.list[i]->page*PAGE_SIZE+j*table.list[i]->length,table.list[i]->length);
					if (tmp[0]!=0x00){
						printf("%-16s",tmp);
						flag=1;
					}
				}
				if (flag) printf("\n");
			}
		}
	}
	else{
		struct ast_wordList *tmplist=wordlist;
		while (tmplist!=NULL) { printf("%-16s",tmplist->name);tmplist=tmplist->next; }
		printf("\n---------------------------------------------------\n");
		for (j=0;j<table.count;j++){
			tmplist=wordlist;
			flag=0;
			while (tmplist!=NULL){
				for (i=0;i<table.words;i++){
					if (!strcmp(tmplist->name,table.list[i]->name)){
						tmp=read_D(fout,table.list[i]->page*PAGE_SIZE+j*table.list[i]->length,table.list[i]->length);
						if (tmp[0]!=0x00){
							printf("%-16s",tmp);
							flag=1;
						}
						break;
					}
				}
				tmplist=tmplist->next;
			}
			if (flag) printf("\n");
		}
	}
	end_select:
	fclose(fin);
	fclose(fout);
}

char *get_MD5(char *str){
	FILE *stream;
	char *buf=malloc(sizeof(char)*33);
	char cmd[100];
	memset(buf,'\0',sizeof(buf));
	memset(cmd,'\0',sizeof(cmd));
	strcpy(cmd,"echo "); //5
	strcpy((char *)cmd+5,str);
	strcpy((char *)cmd+5+strlen(str)," | md5sum");
	stream=popen(cmd,"r");
	fread(buf,sizeof(char),32,stream);
//	printf("%s\n",buf);
	return buf;
}

void create_Role(char *username,char *passwd){
	if (USER_NUM!=0) {
		yyerror("You don't have the right to create role !\n");
	}
	else {
		FILE *fin,*fout;
		if (!(fin=fopen("isql.db","r+"))){
			printf("cannot open file isql.db\n");
		}
		if (!(fout=fopen("isql.dat","r+"))){
			printf("cannot open file isql.dat!\n");
		}
		int num=atoi(read_D(fin,20,8));
		int offset=U_PAGE*PAGE_SIZE+num*U_HEAD_SIZE;
		write_D(fout,offset,username);
		fseek(fout,offset+U_NAME,SEEK_SET);
		fprintf(fout,"%d",num);
		write_D(fout,offset+U_NAME+U_NUM,get_MD5(passwd));
		fseek(fin,20,SEEK_SET);
		fprintf(fin,"%d",num+1);
	}
	return;	
}

int find_UNUM(char *name){
	FILE *fin,*fout;
	if (!(fin=fopen("isql.db","r+"))){
		printf("cannot open file isql.db\n");
	}
	if (!(fout=fopen("isql.dat","r+"))){
		printf("cannot open file isql.dat!\n");
	}
	int num=atoi(read_D(fin,20,8));
	int offset=U_PAGE*PAGE_SIZE;
	int i;
	for (i=0;i<num;i++)
		if (!strcmp(name,read_D(fout,offset,24))) {
			fclose(fin);fclose(fout);
			return atoi(read_D(fout,offset+U_NAME,U_NUM));
		}
		else offset=offset+U_HEAD_SIZE;
}

void user_Manage(struct ast_permission *pchange){
	if (DT_HANDLE==-1) {yyerror("Please choose a DATABASE !"); return ;}
	struct ast_wordList *tablelist=pchange->tablelist;
	struct ast_wordList *userlist =pchange->userlist;
	FILE *fin,*fout;
	if (!(fin=fopen("isql.db","r+"))){
		printf("cannot open file isql.db\n");
	}
	if (!(fout=fopen("isql.dat","r+"))){
		printf("cannot open file isql.dat!\n");
	}

	char *tmp;
	int i;
	int flag;
	int offset=D_PAGE*PAGE_SIZE+DT_HANDLE*D_HEAD_SIZE;
	int table_num=atoi(read_D(fin,offset+D_NAME_SIZE,D_TABLE_NUM));
	int users,usernum,k;
	struct ast_wordList *tmplist;

	while (tablelist!=NULL){
		flag=0;
		offset=T_LIST_PAGE*PAGE_SIZE+DT_HANDLE*T_LIST_SIZE;
		for (i=0;i<table_num;i++){
			tmp=read_D(fin,offset+i*T_HEAD_SIZE,T_NAME_SIZE);
			if (!strcmp(tmp,tablelist->name)){
				flag=1;
				users=atoi(read_D(fin,offset+i*T_HEAD_SIZE+T_NAME_SIZE,T_PAGE))-18;
				usernum=atoi(read_D(fin,offset+i*T_HEAD_SIZE+T_NAME_SIZE+T_PAGE,T_USERNUM));
				break;
			}
		}
		if (!flag) { yyerror("Table:%s didn't exist!\n",tablelist->name); return ;}
		offset=U_LISTPAGE*PAGE_SIZE+users*U_LISTSIZE+usernum*U_WORDSIZE;
		k=0;
		tmplist=userlist;
		while (tmplist!=NULL){
			fseek(fout,offset,SEEK_SET);
			fprintf(fout,"%d",find_UNUM(tmplist->name));
			int org=atoi(read_D(fout,offset+U_NUM,U_POWER));
			if (!pchange->flag)
				pchange->permission=pchange->permission | org;
			else 
				pchange->permission=~pchange->permission & org;
			fseek(fout,offset+U_NUM,SEEK_SET);
			fprintf(fout,"%d",pchange->permission);
			offset=offset+U_WORDSIZE;
			tmplist=tmplist->next;
		}
		tablelist=tablelist->next;
	}
	fclose(fin);
	fclose(fout);
}

void init_File(void){
	//db :0页：8位当前数据库数+12位当前字段表地址+8位用户数
	//dat:0页：16位字段数据地址
	FILE *fin,*fout;
	if (!(fin=fopen("isql.db","r+"))){
		printf("cannot open file isql.db\n");
	}
	if (!(fout=fopen("isql.dat","r+"))){
		printf("cannot open file isql.dat!\n");
	}
	
	int num;
	num=atoi(read_D(fin,8,12));
	if (num==0) { fseek(fin,8,SEEK_SET); fprintf(fin,"%d",18); }
	num=atoi(read_D(fin,20,8));
	if (num==0) {
		fseek(fin,20,SEEK_SET); 
		fprintf(fin,"%d",1);
		//sudo admin admin
		int offset=U_PAGE*PAGE_SIZE;
		write_D(fout,offset,"admin");
		fseek(fout,offset+U_NAME,SEEK_SET);
		fprintf(fout,"%d",0);
		write_D(fout,offset+U_NAME+U_NUM,get_MD5("admin"));
	}
	num=atoi(read_D(fout,0,16));
	if (num==0) { fseek(fout,0,SEEK_SET); fprintf(fout,"%d",66); }

	fclose(fin);
	fclose(fout);	

	DT_HANDLE=-1;
	USER_NUM=-1;
	pFList=0;
	return ;
}

void verify(void){
	printf("\n*************** welcome to ISQL ****************\n");
	printf("* version 1.0                                  *\n");
	printf("* Designed by RL                               *\n");
	printf("* Copyright (c) 2011, RL. All rights reserved  *\n");
	printf("************************************************\n\n");
	char username[32];
	char passwd[32];
	char *md5,*vmd5;
	FILE *fin,*fout;
	int usernum,i,offset;
	if (!(fin=fopen("isql.db","r+"))){
		printf("cannot open file isql.db\n");
	}
	if (!(fout=fopen("isql.dat","r+"))){
		printf("cannot open file isql.dat!\n");
	}	
	usernum=atoi(read_D(fin,20,8));
	while (USER_NUM==-1){
		printf(">username:");
		scanf("%s",username);
		printf(">password:");
		scanf("%s",passwd);
		md5=get_MD5(passwd);
		for (i=0;i<usernum;i++){
			offset=U_PAGE*PAGE_SIZE+i*U_HEAD_SIZE;
			if (!strcmp(username,read_D(fout,offset,24)) && !strcmp(md5,read_D(fout,offset+32,32))){
				USER_NUM=i;
			}
		}
		if (USER_NUM==-1) printf("verify failure , try again !\n");
	}
	printf("\n hi,%s\n",username);
	fclose(fin);
	fclose(fout);
	return ;
}

main (int ac,char **avv){
//	get_MD5("fuckyou");
	init_File();
	verify();
	printf("> ");
	return yyparse();
}

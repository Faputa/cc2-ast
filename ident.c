//符号表、参数类型表

#include "cc.h"
#include <stdio.h>
#include <malloc.h>
#include <string.h>

Id *gid, *lid;

static Id *gids, *lids;

void ident_init(void) {
	gids = gid = (Id*)malloc(MAXSIZE * sizeof(Id));
	lids = lid = (Id*)malloc(MAXSIZE * sizeof(Id));
	gid++ -> kind = GLO;
}

void print_ids(void) {
	printf("--- GLO ---\n");
	for(Id *i = gids; i < gid; i++) {
		if(i -> kind == GLO) printf("GLO");
		else if(i -> kind == STR) printf("STR : %s", i -> name);
		else if(i -> kind == ID) {
			printf("%s ", i -> name);
			printf("%d ", i -> offset);
			if(i -> class == GLO) printf("GLO ");
			print_type(i);
		}
		printf("\n");
	}
	printf("--- LOC ---\n");
	for(Id *i = lids; i < lid; i++) {
		if(i -> kind == FUN) printf("FUN");
		else if(i -> kind == LOC) printf("LOC");
		else if(i -> kind == ID) {
			printf("%s ", i -> name);
			printf("%d ", i -> offset);
			if(i -> class == ARG) printf("ARG ");
			if(i -> class == LOC) printf("LOC ");
			print_type(i);
		}
		printf("\n");
	}
	printf("\n");
}

Id* sgetstr(char *tks) {
	for(Id *i = gid - 1; i > gids; i--) {
		if(i->kind == STR && !strcmp(tks, i->name)) return i;
	}
	
	Id *this_id = gid++;
	Id *last_id = this_id - 1;
	
	this_id->name = tks;
	this_id->type = type_derive(ARR, typechar, strlen(tks) + 1);
	this_id->kind = STR;
	
	while(last_id->kind == ID && (last_id->type->base == FUN || last_id->type->base == API)) last_id--;
	if(last_id == gids) this_id->offset = MAXSIZE - type_size(this_id->type);
	else this_id->offset = last_id->offset - type_size(this_id->type);
	
	for(int i = 0; i < strlen(tks) + 1; i++) {
		(data + this_id->offset)[i] = tks[i];
	}
	
	return this_id;
}

Id* setid(Type* type, Id *id) {
	for(Id *i = id - 1; i->kind == ID || i->kind == STR; i--) {
		if(!strcmp(id->name, i->name) && i->kind != STR) error("line %d: error!\n", line);
	}
	
	id->type = type;
	id->kind = ID;
	
	Id *last_id = id - 1;
	if(id->class == GLO) {
		gid = id + 1;
		while(last_id->kind == ID && (last_id->type->base == FUN || last_id->type->base == API)) last_id--;
		if(last_id->kind == GLO) id->offset = MAXSIZE - type_size(type);
		else id->offset = last_id->offset - type_size(type);
	} else {
		lid = id + 1;
		while(last_id->kind == LOC) last_id--;
		if(last_id->kind == FUN || last_id->class == ARG) id->offset = 0;
		else id->offset = last_id->offset + type_size(last_id->type);
	}
	return id;
}

Id* getid(char *tks) {
	for(Id *i = lid - 1; i > lids; i--) {
		if(i -> kind == ID && !strcmp(tks, i -> name)) return i;
	}
	for(Id *i = gid - 1; i > gids; i--) {
		if(i -> kind == ID && !strcmp(tks, i -> name)) return i;
	}
	return NULL;
}

void inblock(void) {
	(lid++) -> kind = LOC;
}

void outblock(void) {
	do {
		lid--;
	} while(lid -> kind != LOC);
}

void inparam(void) {
	(lid++) -> kind = FUN;
}

void infunc(void) {
	Id *i = lid - 1;
	int argc = 0;
	while(i -> kind != FUN) {
		(i--) -> offset -= (argc++) + 3;
	}
}

void outfunc(void) {
	lid = lids;
}

//assert——处理程序员的错误
//所有参数类型为Node*的函数皆以gen_作为前缀

#include "cc.h"
#include <stdio.h>
#include <assert.h>
#include <malloc.h>
#include <string.h>

int *emit, *e;

static Type* gen_specifier(Node *n);
static void gen_declare(Node *n, int scope);
static Id* gen_decl_expr(Node *n, Type *t, int scope);
static int gen_const_expr(Node *n);
static Er gen_expr(Node *n);
static int gen_const_ptr(Node *n, Type *t);
static void gen_arr_init(Node *n, int scope, Type *t, int offset);
static void gen_stmt_expr(Node *n);
static void gen_stmt(Node *n);

void gen_init(void) {
	emit = e = (int*)malloc(MAXSIZE * sizeof(int));
}

static Type* gen_specifier(Node *n) {
	assert(n && n->kind == SPECIFIER);
	return type_derive(n->tki, NULL, 0);
}

static Id* gen_decl_expr(Node *n, Type *t, int scope) {
	assert(n);
	Id *id = (scope == GLO)? gid: lid;
	id->class = scope;
	for(; n->kind != EXPR_ATOM; n = n->child[0]) {
		assert(n);
		if(n->kind == EXPR_UNARY) {
			assert(n->tki == DEREF);
			if(!(t = type_derive(PTR, t, 0))) error("line %d: error1!\n", line);
		} else if(n->kind == EXPR_BINARY) {
			if(n->tki == CALL) {
				int count = 0;
				inparam();
				for(Node *i = n->child[1]; i; i = i->next) {
					count++;
					assert(i->kind == PARAMETER);
					gen_decl_expr(i->child[1], gen_specifier(i->child[0]), ARG);
				}
				if(!(t = type_derive(FUN, t, count))) error("line %d: error2!\n", line);
			} else if(n->tki == INDEX) {
				int count = n->child[1]? gen_const_expr(n->child[1]): 0;
				if(!(t = type_derive(ARR, t, count))) error("line %d: error3!\n", line);
			} else assert(0);
		} else assert(0);
	}
	assert(n->tki == ID);
	id->name = n->tks;
	if(t->base == FUN && scope == ARG) {
		if(!(t = type_derive(PTR, t, 0))) error("line %d: error4!\n", line);
	} else if(t->base == ARR && scope == ARG) {
		if(!(t = type_derive(PTR, t->rely, 0))) error("line %d: error5!\n", line);
	}
	if(!setid(t, id)) error("line %d: error6!\n", line);
	return id;
}

static int gen_const_expr(Node *n) {
	assert(n);
	if(n->kind == EXPR_ATOM) {
		if(n->tki == INT) return atoi(n->tks);
		else if(n->tki == CHAR) return n->tks[0];
	} else if(n->kind == EXPR_UNARY) {
		assert(n->tki == NOT);
		return !gen_const_expr(n->child[0]);
	} else if(n->kind == EXPR_BINARY) {
		int lopr = gen_const_expr(n->child[0]);
		int ropr = gen_const_expr(n->child[1]);
		if(n->tki == ADD) return lopr + ropr;
		else if(n->tki == SUB) return lopr - ropr;
		else if(n->tki == MUL) return lopr * ropr;
		else if(n->tki == DIV) return lopr / ropr;
		else if(n->tki == MOD) return lopr % ropr;
		else if(n->tki == EQ) return lopr == ropr;
		else if(n->tki == GT) return lopr > ropr;
		else if(n->tki == LT) return lopr < ropr;
		else if(n->tki == AND) return lopr && ropr;
		else if(n->tki == OR) return lopr || ropr;
	}
	assert(0);
}

static Er gen_expr(Node *n) {
	assert(n);
	Er er = {NULL, 0, 1};
	if(n->kind == EXPR_ATOM) {
		if(n->tki == INT) {
			er.type = typeint;
			*e++ = SET; *e++ = AX; *e++ = atoi(n->tks);
		} else if(n->tki == CHAR) {
			er.type = typechar;
			*e++ = SET; *e++ = AX; *e++ = n->tks[0];
		} else if(n->tki == STR) {
			Id *id = sgetstr(n->tks);
			*e++ = AG; *e++ = id->offset;
			er.type = id->type;
			er.is_lvalue = 1;
		} else if(n->tki == ID) {
			Id *id = getid(n->tks);
			if(!id) error("line %d: error7!\n", line);
			er.type = id->type;
			*e++ = id->class == GLO? AG: AL; *e++ = id->offset;
			if(er.type->base == INT || er.type->base == CHAR || er.type->base == PTR) {
				*e++ = VAL;
				er.is_const = 0;
			}
			er.is_lvalue = 1;
		} else if(n->tki == NUL) {
			er.type = typenull;
			*e++ = SET; *e++ = AX; *e++ = 0;
		} else assert(0);
	} else if(n->kind == EXPR_UNARY) {
		if(n->tki == DEREF) {
			er.type = gen_expr(n->child[0]).type;
			if(er.type->base == ARR) {
				if(!(er.type = type_derive(PTR, er.type->rely, 0))) error("line %d: error8!\n", line);
			}
			if(er.type->base != PTR) error("line %d: error9!\n", line);
			er.type = er.type->rely;
			if(er.type->base == INT || er.type->base == CHAR || er.type->base == PTR) {
				*e++ = VAL;
				er.is_const = 0;
			}
			er.is_lvalue = 1;
		} else if(n->tki == REF) {
			Er _er = gen_expr(n->child[0]);
			if(!_er.is_lvalue) error("line %d: error10!\n", line);
			if(_er.type->base == INT || _er.type->base == PTR) e--;
			if(!(er.type = type_derive(PTR, _er.type, 0))) error("line %d: error11!\n", line);
		} else if(n->tki == NOT) {
			er.type = gen_expr(n->child[0]).type;
			if(er.type->base != INT) er.type = typeint;
			*e++ = NOT;
		} else assert(0);
	} else if(n->kind == EXPR_BINARY) {
		er = gen_expr(n->child[0]);
		if(n->tki == ASS) e--;
		*e++ = PUSH; *e++ = AX;
		if(n->tki == CALL) {
			if(er.type->base == PTR) er.type = er.type->rely;
			if(er.type->base != FUN && er.type->base != API) error("line %d: error12!\n", line);
			int argc = 0;
			for(Node *i = n->child[1]; i; i = i->next) {
				if(!type_check((er.type->argtyls)[argc], gen_expr(i).type, ASS)) error("line %d: error13!\n", line);
				*e++ = PUSH; *e++ = AX;
				argc++;
			}
			if(argc != er.type->count) error("line %d: error14!\n", line);
			*e++ = (er.type->base == FUN)? CALL: CAPI; *e++ = argc;
			*e++ = DEC; *e++ = SP; *e++ = argc + 1;
			er.type = er.type->rely;
		} else {
			if(!type_check(er.type, gen_expr(n->child[1]).type, n->tki)) error("line %d: error15!\n", line);
			if(n->tki == ADD || n->tki == SUB) {
				if(er.type->base == INT) {
					*e++ = n->tki;
				} else if(er.type->base == PTR || er.type->base == ARR) {
					*e++ = PUSH; *e++ = AX;
					*e++ = SET; *e++ = AX; *e++ = type_size(er.type->rely);
					*e++ = MUL;
					*e++ = n->tki;
					if(er.type->base == ARR) {
						if(!(er.type = type_derive(PTR, er.type->rely, 0))) error("line %d: error16!\n", line);
					}
				} else assert(0);
			} else {
				*e++ = n->tki;
			}
		}
	} else assert(0);
	return er;
}

static int gen_const_ptr(Node *n, Type *t) {
	assert(n && n->kind == EXPR_ATOM);
	if(n->tki == NUL) return 0;
	else if(n->tki == STR && t->rely->base == CHAR) return sgetstr(tks)->offset;
	assert(0);
}

static void gen_arr_init(Node *n, int scope, Type *t, int offset) {
	assert(n);
	if(scope == GLO) {
		if(n->kind == INIT_LIST) {
			memset(data + offset, 0, t->count);
			int count = 0;
			for(Node *i = n->child[0]; i; i = i->next) {
				count++;
				if(t->rely->base == INT) data[offset] = gen_const_expr(i);
				else if(t->rely->base == CHAR) data[offset] = gen_const_expr(i);
				else if(t->rely->base == PTR) data[offset] = gen_const_ptr(i, t->rely);
				else if(t->rely->base == ARR) gen_arr_init(i, GLO, t->rely, offset);
				else assert(0);
				offset += type_size(t->rely);
			}
			assert(t->count >= count);
		} else if(n->kind == EXPR_ATOM) {
			assert(n->tki == STR);
			assert(t->rely->base == CHAR);
			Id *s = sgetstr(n->tks);
			assert(t->count >= s->type->count);
			for(int i = 0; i < s->type->count; i++) {
				data[offset + i] = data[s->offset + i];
			}
		} else assert(0);
	} else if(scope == LOC) {
		if(n->kind == INIT_LIST) {
			int count = 0;
			for(Node *i = n->child[0]; i; i = i->next) {
				count++;
				switch(t->rely->base) {
				case INT: case CHAR: case PTR:
					*e++ = AL; *e++ = offset;
					*e++ = PUSH; *e++ = AX;
					if(!type_check(t->rely, gen_expr(i).type, ASS)) error("line %d: error17!\n", line);
					*e++ = ASS;
					break;
				case ARR: gen_arr_init(i, LOC, t->rely, offset); break;
				default: assert(0);
				}
				offset += type_size(t->rely);
			}
			if(t->count < count) error("line %d: error18!\n", line);
		} else if(n->kind == EXPR_ATOM) {
			assert(n->tki == STR);
			assert(t->rely->base == CHAR);
			Id *s = sgetstr(n->tks);
			assert(t->count >= s->type->count);
			for(int i = 0; i < s->type->count; i++) {
				*e++ = AL; *e++ = offset + i;
				*e++ = PUSH; *e++ = AX;
				*e++ = AG; *e++ = s->offset + i;
				*e++ = VAL;
				*e++ = ASS;
			}
		} else assert(0);
	} else assert(0);
}

static void gen_stmt_expr(Node *n) {
	if(n->kind == DECLARE) gen_declare(n, LOC);
	else gen_expr(n);
}

static void gen_stmt(Node *n) {
	assert(n);
	if(n->kind == STMT_BLOCK) {
		inblock();
		for(Node *i = n->child[0]; i; i = i->next) {
			gen_stmt(i);
		}
		outblock();
	} else if(n->kind == STMT_IF) {
		gen_expr(n->child[0]);
		*e++ = JZ; int *_e1 = e++;
		gen_stmt(n->child[1]);
		if(n->child[2]) {
			*e++ = JMP; int *_e2 = e++;
			*_e1 = e - emit;
			gen_stmt(n->child[2]);
			*_e2 = e - emit;
		} else {
			*_e1 = e - emit;
		}
	} else if(n->kind == STMT_WHILE) {
		int *_e1 = e;
		gen_expr(n->child[0]);
		*e++ = JZ; int *_e2 = e++;
		gen_stmt(n->child[1]);
		*e++ = JMP; *e++ = _e1 - emit;
		*_e2 = e - emit;
	} else if(n->kind == STMT_DO_WHILE) {
		int *_e1 = e;
		gen_stmt(n->child[0]);
		gen_expr(n->child[1]);
		*e++ = JZ; int *_e2 = e++;
		*e++ = JMP; *e++ = _e1 - emit;
		*_e2 = e - emit;
	} else if(n->kind == STMT_FOR) {
		inblock();
		if(n->child[0]) {
			gen_stmt_expr(n->child[0]);
		}
		int *_e1 = e;
		int *_e2;
		if(n->child[1]) {
			gen_expr(n->child[1]);
			*e++ = JZ; _e2 = e++;
		}
		gen_stmt(n->child[3]);
		if(n->child[2]) {
			gen_expr(n->child[2]);
		}
		*e++ = JMP; *e++ = _e1 - emit;
		*_e2 = e - emit;
		outblock();
	} else if(n->kind == STMT_RETURN) {
		gen_expr(n->child[0]);
		*e++ = MOV; *e++ = SP; *e++ = BP;
		*e++ = POP; *e++ = BP;
		*e++ = POP; *e++ = IP;
	} else if(n->kind != STMT_EMPTY) {
		gen_stmt_expr(n);
	}
}

static void gen_declare(Node *n, int scope) {
	static int varc;
	assert(n && n->kind == DECLARE);
	Type *t = gen_specifier(n->child[0]);
	for(Node *i = n->child[1]; i; i = i->next) {
		assert(i->kind == DECLARATOR);
		Id *id = gen_decl_expr(i->child[0], t, scope);
		if(scope == GLO) {
			if(i->child[1]) {
				if(id->type->base == FUN) {
					infunc();
					varc = 0;
					id->offset = e - emit;
					*e++ = PUSH; *e++ = BP;
					*e++ = MOV; *e++ = BP; *e++ = SP;
					*e++ = INC; *e++ = SP; int *_e = e++;
					assert(i->child[1]->kind == STMT_BLOCK);
					for(Node *j = i->child[1]->child[0]; j; j = j->next) {
						gen_stmt(j);
					}
					*_e = varc;
					*e++ = MOV; *e++ = SP; *e++ = BP;
					*e++ = POP; *e++ = BP;
					*e++ = POP; *e++ = IP;
					outfunc();
				}
				else if(id->type->base == ARR) gen_arr_init(i->child[1], GLO, id->type, id->offset);
				else if(id->type->base == PTR) data[id->offset] = gen_const_ptr(i->child[1], id->type);
				else if(id->type->base == INT) data[id->offset] = gen_const_expr(i->child[1]);
				else if(id->type->base == CHAR) data[id->offset] = gen_const_expr(i->child[1]);
				else assert(0);
			} else {
				switch(id->type->base){
				case INT: case CHAR: case PTR:
					data[id->offset] = 0; break;
				case FUN: outfunc(); break;
				case ARR: memset(data + id->offset, 0, id->type->count); break;
				default: assert(0);
				}
			}
		} else if(scope == LOC) {
			if(i->child[1]) {
				switch(id->type->base){
				case INT: case CHAR: case PTR:
					*e++ = AL; *e++ = id->offset;
					*e++ = PUSH; *e++ = AX;
					if(!type_check(id->type, gen_expr(i->child[1]).type, ASS)) error("line %d: error19!\n", line);
					*e++ = ASS;
					break;
				case ARR: gen_arr_init(i->child[1], LOC, id->type, id->offset); break;
				default: assert(0);
				}
			}
			varc += type_size(id->type);
		} else assert(0);
	}
}

void gen_proto(Node *n) {
	gen_declare(n, GLO);
}

void gen(Node *n) {
	*e++ = AG; int *_main = e++;
	*e++ = PUSH; *e++ = AX;
	*e++ = CALL; *e++ = 0;
	*e++ = EXIT;
	for(; n; n = n->next) {
		gen_declare(n, GLO);
	}
	Id *idmain = getid("main");
	if(!idmain) error("line %d: error20!\n", line);
	*_main = idmain->offset;
}

//if-error6-print-exit处理用户的错误
//所有返回值类型为Node*的函数皆以parse_作为前缀

#include "cc.h"
#include <stdio.h>
#include <malloc.h>
#include <string.h>

static int expr_lev(char *opr);
static int decl_lev(char *opr);
static Node* parse_expr(char *last_opr);
static Node* parse_array(void);
static Node* parse_specifier(void);
static Node* parse_decl_expr(char *last_opr);
static Node* parse_declare(void);
static Node* parse_stmt_expr(void);
static Node* parse_stmt(void);

static int expr_lev(char *opr) { //优先级越高lev越大，其他符号lev为0
	char *oprs[] = {
		")", "]",
		"", "&&", "||", "!",
		"", "==", "!=",
		"", ">", "<", ">=", "<=",
		"", "+", "-",
		"", "*", "/", "%",
		"", "=",
		"", "*_", "&_",
		"", "(", "["
	};
	int lev = 1;
	for(int i = 0; i < sizeof(oprs) / sizeof(*oprs); i++) {
		if(!strcmp(oprs[i], opr)) return lev;
		else if(!strcmp(oprs[i], "")) lev++;
	}
	return 0; //其他符号
}

static int decl_lev(char *opr) {
	char *oprs[] = {
		")", "]",
		"", "*",
		"", "(", "["
	};
	int lev = 1;
	for(int i = 0; i < sizeof(oprs) / sizeof(*oprs); i++) {
		if(!strcmp(oprs[i], opr)) return lev;
		else if(!strcmp(oprs[i], "")) lev++;
	}
	return 0; //其他符号
}

static Node* parse_expr(char *last_opr) {
	Node *n;
	switch(tki) {
	case INT: case CHAR: case STR: case ID: case NUL:
		n = ast_new_node(EXPR_ATOM); n->tki = tki; n->tks = tks; next(); break;
	default:
		if(!strcmp(tks, "(")) {
			next();
			n = parse_expr(")");
			if(!strcmp(tks, ")")) next(); else error("line %d: error7!\n", line);
		} else if(!strcmp(tks, "-")) {
			next();
			n = ast_new_node(EXPR_BINARY);
			n->tki = SUB;
			n->child[0] = ast_new_node(EXPR_ATOM);
			n->child[0]->tki = INT;
			n->child[0]->tks = "0";
			n->child[1] = parse_expr("-");
		} else {
			n = ast_new_node(EXPR_UNARY);
			if(!strcmp(tks, "*")) {
				next();
				n->tki = DEREF;
				n->child[0] = parse_expr("*_");
			} else if(!strcmp(tks, "&")) {
				next();
				n->tki = REF;
				n->child[0] = parse_expr("&_");
			} else if(!strcmp(tks, "!")) {
				next();
				n->tki = NOT;
				n->child[0] = parse_expr("!");
			} else error("line %d: error8!\n", line);
		}
	}
	
	while(expr_lev(tks) > expr_lev(last_opr)) {
		char *opr = tks;
		next();
		Node *_n = n;
		n = ast_new_node(EXPR_BINARY);
		n->child[0] = _n;
		if(!strcmp(opr, "(")) {
			n->tki = CALL;
			if(strcmp(tks, ")")) {
				Node *i = NULL;
				while(1) {
					if(i == NULL) {
						n->child[1] = parse_expr("");
						i = n->child[1];
					} else {
						i->next = parse_expr("");
						i = i->next;
					}
					if(!strcmp(tks, ")")) break;
					else if(!strcmp(tks, ",")) next();
					else error("line %d: error9!\n", line);
				}
			}
			next();
		} else if(!strcmp(opr, "[")) {
			n->tki = ADD;
			n->child[1] = parse_expr("]");
			_n = n;
			n = ast_new_node(EXPR_UNARY);
			n->tki = DEREF;
			n->child[0] = _n;
			if(!strcmp(tks, "]")) next(); else error("line %d: error10!\n", line);
		} else if(!strcmp(opr, "=")) {
			n->tki = ASS;
			n->child[1] = parse_expr("");
		} else {
			n->child[1] = parse_expr(opr);
			if(!strcmp(opr, "+")) n->tki = ADD;
			else if(!strcmp(opr, "-")) n->tki = SUB;
			else if(!strcmp(opr, "*")) n->tki = MUL;
			else if(!strcmp(opr, "/")) n->tki = DIV;
			else if(!strcmp(opr, "%")) n->tki = MOD;
			else if(!strcmp(opr, "&&")) n->tki = AND;
			else if(!strcmp(opr, "||")) n->tki = OR;
			else if(!strcmp(opr, ">")) n->tki = GT;
			else if(!strcmp(opr, "<")) n->tki = LT;
			else if(!strcmp(opr, "==")) n->tki = EQ;
			else {
				_n = n;
				n = ast_new_node(EXPR_UNARY);
				n->tki = NOT;
				n->child[0] = _n;
				if(!strcmp(opr, ">=")) _n->tki = LT;
				else if(!strcmp(opr, "<=")) _n->tki = GT;
				else if(!strcmp(opr, "!=")) _n->tki = EQ;
				else error("line %d: error11!\n", line);
			}
		}
	}
	return n;
}

static Node* parse_array(void) {
	if(!strcmp(tks, "{")) next(); else error("line %d: error12!\n", line);
	Node *n = ast_new_node(INIT_LIST);
	if(strcmp(tks, "}")) {
		Node *i = n;
		while(1) {
			Node *t = !strcmp(tks, "{")? parse_array(): parse_expr("");
			if(i == n) {
				i->child[0] = t;
				i = i->child[0];
			} else {
				i->next = t;
				i = i->next;
			}
			if(!strcmp(tks, "}")) break;
			else if(!strcmp(tks, ",")) next();
			else error("line %d: error13!\n", line);
		}
	}
	next();
	return n;
}

static Node* parse_specifier(void) {
	Node *n = ast_new_node(SPECIFIER);
	if(tki == Int) n->tki = INT;
	else if(tki == Char) n->tki = CHAR;
	else if(tki == Void) n->tki = VOID;
	else error("line %d: error14!\n", line);
	next();
	return n;
}

static Node* parse_decl_expr(char *last_opr) {
	Node *n;
	if(!strcmp(tks, "*")) {
		next();
		n = ast_new_node(EXPR_UNARY);
		n->tki = DEREF;
		n->child[0] = parse_decl_expr("*");
	} else if(!strcmp(tks, "(")) {
		next();
		n = parse_decl_expr(")");
		if(!strcmp(tks, ")")) next(); else error("line %d: error15!\n", line);
	} else if(tki == ID) {
		n = ast_new_node(EXPR_ATOM);
		n->tki = ID;
		n->tks = tks;
		next();
	} else error("line %d: error16!\n", line);
	
	while(decl_lev(tks) > decl_lev(last_opr)) {
		Node *_n = n;
		n = ast_new_node(EXPR_BINARY);
		n->child[0] = _n;
		if(!strcmp(tks, "(")) {
			next();
			n->tki = CALL;
			if(strcmp(tks, ")")) {
				Node *i = n;
				while(1) {
					Node *p = ast_new_node(PARAMETER);
					p->child[0] = parse_specifier();
					p->child[1] = parse_decl_expr("");
					if(i == n) {
						i->child[1] = p;
						i = i->child[1];
					} else {
						i->next = p;
						i = i->next;
					}
					if(!strcmp(tks, ")")) break;
					else if(!strcmp(tks, ",")) next();
					else error("line %d: error17!\n", line);
				}
			}
		} else if(!strcmp(tks, "[")) {
			next();
			n->tki = INDEX;
			if(tki == INT) {
				n->child[1] = parse_expr("]");
			}
			if(strcmp(tks, "]")) error("line %d: error18!\n", line);
		} else error("line %d: error19!\n", line);
		next();
	}
	return n;
}

static Node* parse_declare(void) {
	Node *n = ast_new_node(DECLARE);
	n->child[0] = parse_specifier();
	Node *i = n;
	int j = 0;
	while(1) {
		Node *d = ast_new_node(DECLARATOR);
		d->child[0] = parse_decl_expr("");
		if(!strcmp(tks, "=")) {
			next();
			d->child[1] = !strcmp(tks, "{")? parse_array(): parse_expr("");
		} else if(!strcmp(tks, "{")) {
			d->child[1] = parse_stmt();
		}
		if(i == n) {
			i->child[1] = d;
			i = i->child[1];
		} else {
			i->next = d;
			i = i->next;
		}
		if(!strcmp(tks, ";")) break;
		else if(!strcmp(tks, ",")) next();
		else if(j == 0 && !strcmp(tks, "}")) break;
		else error("line %d: error20!\n", line);
	}
	return n;
}

static Node* parse_stmt_expr(void) { //局部语句
	Node *n;
	if(tki == Int || tki == Char || tki == Void) {
		n = parse_declare();
		if(n->child[1]->child[1] && n->child[1]->child[1]->kind == STMT_BLOCK) { //防止局部定义为函数定义
			error("line %d: error21!\n", line);
		}
	} else {
		n = parse_expr("");
		if(strcmp(tks, ";")) error("line %d: error22!\n", line);
	}
	return n;
}

static Node* parse_stmt(void) {
	Node *n;
	if(!strcmp(tks, "{")) {
		n = ast_new_node(STMT_BLOCK); next();
		Node *i = n;
		while(strcmp(tks, "}")) {
			if(i == n) {
				i->child[0] = parse_stmt();
				i = i->child[0];
			} else {
				i->next = parse_stmt();
				i = i->next;
			}
			next();
		}
	} else if(tki == If) {
		n = ast_new_node(STMT_IF); next();
		if(!strcmp(tks, "(")) next(); else error("line %d: error23!\n", line);
		n->child[0] = parse_expr(")");
		if(!strcmp(tks, ")")) next(); else error("line %d: error24!\n", line);
		n->child[1] = parse_stmt(); peek();
		if(tki == Else) {
			next(); next();
			n->child[2] = parse_stmt();
		}
	} else if(tki == Do) {
		n = ast_new_node(STMT_DO_WHILE); next();
		n->child[0] = parse_stmt(); next();
		if(tki == While) next(); else error("line %d: error25!\n", line);
		if(!strcmp(tks, "(")) next(); else error("line %d: error26!\n", line);
		n->child[1] = parse_expr(")");
		if(!strcmp(tks, ")")) next(); else error("line %d: error27!\n", line);
		if(strcmp(tks, ";")) error("line %d: error28!\n", line);
	} else if(tki == For) {
		n = ast_new_node(STMT_FOR); next();
		if(!strcmp(tks, "(")) next(); else error("line %d: error29!\n", line);
		if(strcmp(tks, ";")) {
			n->child[0] = parse_stmt_expr();
		}
		next();
		if(strcmp(tks, ";")) {
			n->child[1] = parse_expr("");
			if(strcmp(tks, ";")) error("line %d: error30!\n", line);
		}
		next();
		if(strcmp(tks, ")")) {
			n->child[2] = parse_expr(")");
			if(strcmp(tks, ")")) error("line %d: error31!\n", line);
		}
		next();
		n->child[3] = parse_stmt();
	} else if(tki == While) {
		n = ast_new_node(STMT_WHILE); next();
		if(!strcmp(tks, "(")) next(); else error("line %d: error32!\n", line);
		n->child[0] = parse_expr(")");
		if(!strcmp(tks, ")")) next(); else error("line %d: error33!\n", line);
		n->child[1] = parse_stmt();
	} else if(tki == Return) {
		n = ast_new_node(STMT_RETURN); next();
		n->child[0] = parse_expr("");
		if(strcmp(tks, ";")) error("line %d: error34!\n", line);
	} else {
		if(!strcmp(tks, ";")) n = ast_new_node(STMT_EMPTY);
		else n = parse_stmt_expr();
	}
	return n;
}

Node* parse_proto(char *str) {
	token_set(str);
	next();
	return parse_declare();
}

Node* parse(char *str) {
	Node *n, *i = NULL;
	token_set(str);
	next();
	while(strcmp(tks, "") || tki != -1) {
		if(i == NULL) {
			i = n = parse_declare();
		} else {
			i->next = parse_declare();
			i = i->next;
		}
		next();
	}
	return n;
}

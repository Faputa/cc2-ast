#include "cc.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

static void print_indent(int indent, char *fmt, ...);
static void print_node(Node *n, int indent);
static void print_stmt(Node *n, int indent);

static void print_indent(int indent, char *fmt, ...) {
	for(int i = 0; i < indent; i++) printf("\t");
	va_list args;
	va_start(args, fmt);
	printf(fmt, *args);
	va_end(args);
}

static void print_node(Node *n, int indent) {
	if(!n) return;
	if(n->kind == DECLARE) {
		print_node(n->child[0], 0);
		printf(" ");
		for(Node *i = n->child[1]; i; i = i->next) {
			print_node(i, indent);
			if(i->next) printf(", ");
		}
		if(!(n->child[1]->child[1] && n->child[1]->child[1]->kind == STMT_BLOCK)) printf(";");
	} else if(n->kind == SPECIFIER) {
		if(n->tki == INT) printf("int");
		else if(n->tki == CHAR) printf("char");
		else if(n->tki == VOID) printf("void");
	} else if(n->kind == DECLARATOR) {
		print_node(n->child[0], 0);
		if(n->child[1]) {
			if(n->child[1]->kind == STMT_BLOCK) {
				printf("\n");
				print_stmt(n->child[1], indent);
			} else {
				printf(" = ");
				print_node(n->child[1], 0);
			}
		}
	} else if(n->kind == PARAMETER) {
		print_node(n->child[0], 0);
		printf(" ");
		print_node(n->child[1], 0);
	} else if(n->kind == INIT_LIST) {
		printf("{");
		for(Node *i = n->child[0]; i; i = i->next) {
			print_node(i, 0);
			if(i->next) printf(", ");
		}
		printf("}");
	} else if(n->kind == EXPR_ATOM) {
		if(n->tki == ID) printf("%s", n->tks);
		else if(n->tki == INT) printf("%s", n->tks);
		else if(n->tki == CHAR) printf("%c", n->tks[0]);
		else if(n->tki == STR) printf("\"%s\"", n->tks);
		else if(n->tki == NUL) printf("NULL");
	} else if(n->kind == EXPR_UNARY) {
		if(n->tki == REF) printf("&");
		else if(n->tki == DEREF) printf("*");
		else if(n->tki == NOT) printf("!");
		print_node(n->child[0], 0);
	} else if(n->kind == EXPR_BINARY) {
		if(n->tki == CALL) {
			print_node(n->child[0], 0);
			printf("(");
			for(Node *i = n->child[1]; i; i = i->next) {
				print_node(i, 0);
				if(i->next) printf(", ");
			}
			printf(")");
		} else if(n->tki == INDEX) {
			print_node(n->child[0], 0);
			printf("[");
			print_node(n->child[1], 0);
			printf("]");
		} else {
			print_node(n->child[0], 0);
			if(n->tki == ADD) printf(" + ");
			else if(n->tki == SUB) printf(" - ");
			else if(n->tki == MUL) printf(" * ");
			else if(n->tki == DIV) printf(" / ");
			else if(n->tki == MOD) printf(" %% ");
			else if(n->tki == ASS) printf(" = ");
			else if(n->tki == EQ) printf(" == ");
			else if(n->tki == GT) printf(" > ");
			else if(n->tki == LT) printf(" < ");
			else if(n->tki == AND) printf(" && ");
			else if(n->tki == OR) printf(" || ");
			print_node(n->child[1], 0);
		}
	}
}

static void print_stmt(Node *n, int indent) {
	if(!n) return;
	if(n->kind == STMT_IF) {
		print_indent(indent, "if(");
		print_node(n->child[0], 0);
		printf(")\n");
		print_stmt(n->child[1], n->child[1] && n->child[1]->kind == STMT_BLOCK ? indent : indent + 1);
		if(n->child[2]) {
			print_indent(indent, "else\n");
			print_stmt(n->child[2], n->child[2] && n->child[2]->kind == STMT_BLOCK ? indent : indent + 1);
		}
	} else if(n->kind == STMT_WHILE) {
		print_indent(indent, "while(");
		print_node(n->child[0], 0);
		printf(")\n");
		print_stmt(n->child[1], n->child[1] && n->child[1]->kind == STMT_BLOCK ? indent : indent + 1);
	} else if(n->kind == STMT_DO_WHILE) {
		print_indent(indent, "do\n");
		print_stmt(n->child[0], n->child[0] && n->child[0]->kind == STMT_BLOCK ? indent : indent + 1);
		print_indent(indent, "while(");
		print_node(n->child[1], 0);
		printf(")\n");
	} else if(n->kind == STMT_FOR) {
		print_indent(indent, "for(");
		print_node(n->child[0], 0);
		printf(" ");
		print_node(n->child[1], 0);
		printf("; ");
		print_node(n->child[2], 0);
		printf(")\n");
		print_stmt(n->child[3], n->child[3] && n->child[3]->kind == STMT_BLOCK ? indent : indent + 1);
	} else if(n->kind == STMT_RETURN) {
		print_indent(indent, "return ");
		print_node(n->child[0], 0);
		printf(";\n");
	} else if(n->kind == STMT_BLOCK) {
		print_indent(indent, "{\n");
		for(Node *i = n->child[0]; i; i = i->next) {
			print_stmt(i, indent + 1);
		}
		print_indent(indent, "}\n");
	} else if(n->kind == STMT_EMPTY) {
		print_indent(indent, ";\n");
	} else {
		print_indent(indent, "");
		print_node(n, indent);
		if(n->kind != DECLARE) printf(";"); //±í´ïÊ½
		printf("\n");
	}
}

void print_program(Node *n) {
	while(n) {
		print_stmt(n, 0);
		n = n->next;
	}
}

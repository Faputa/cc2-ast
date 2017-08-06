#include "cc.h"
#include <stdio.h>
#include <malloc.h>

Node* ast_new_node(int kind) {
	Node *n = (Node*)malloc(sizeof(Node));
	n->kind = kind;
	n->line = line;
	n->next = NULL;
	for(int i = 0; i < 4; i++) n->child[i] = NULL;
	n->tks = "";
	return n;
}

void ast_del_node(Node *n) {
	if(n) {
		ast_del_node(n->next);
		for(int i = 0; i < 4; i++) ast_del_node(n->child[i]);
		free(n);
	}
}

#ifndef _CC_H_
#define _CC_H_

#define MAXSIZE 10000
#define BUFSIZE 100

typedef struct Type {
	int base;
	int count;
	struct Type *rely;
	struct Type **argtyls;
} Type;

typedef struct {
	int kind;
	char *name;
	Type *type;
	int offset;
	int class;
} Id;

typedef struct {
	Type *type;
	int is_lvalue;
	int is_const;
} Er;

typedef struct Node {
	int kind;
	int line;
	struct Node *next;
	struct Node *child[4];
	int tki;
	char *tks;
} Node;

typedef void (*Api)(void);

enum {
	//keyword
	Int, Char, Void, If, Else, While, Do, For, Return, Null,
	//type
	INT, CHAR, VOID, NUL, FUN, API, PTR, ARR,
	//ast kind
	DECLARE, SPECIFIER, DECLARATOR, PARAMETER,
	EXPR_ATOM, EXPR_UNARY, EXPR_BINARY, INIT_LIST, INDEX, REF, DEREF,
	STMT_BLOCK, STMT_IF, STMT_WHILE, STMT_DO_WHILE, STMT_FOR, STMT_RETURN, STMT_EMPTY,
	//other integer
	ID, GLO, LOC, ARG, STR,
	//opcode
	PUSH, POP, SET, INC, DEC, JMP, JZ, MOV, ADD, SUB, MUL, DIV, MOD, EQ, GT, LT, AND, OR, NOT, AG, AL, VAL, ASS, CALL, CAPI, EXIT,
	//reg
	IP = 0, BP, SP, AX
};

extern Id *gid, *lid;
extern char *tks;
extern int tki, line, *e, *emit, *data;
extern Type *typeint, *typechar, *typenull;

//ast.c
Node* ast_new_node(int kind);
void ast_del_node(Node *n);

//print_ast.c
void print_program(Node *n);

//parser.c
Node* parse_proto(char *str);
Node* parse(char *str);

//gen.c
void gen_init(void);
void gen_proto(Node *n);
void gen(Node *n);

//type.c
void type_init(void);
Type* type_derive(int base, Type *rely, int count);
void print_type(Id *id);
int type_size(Type *type);
int type_check(Type *t1, Type *t2, int opr);

//error.c
void error(char *fmt, ...);
void warning(char *fmt, ...);

//ident.c
void ident_init(void);
void print_ids(void);
Id* sgetstr(char *tks);
Id* setid(Type *type, Id *id);
Id* getid(char *tks);
void inblock(void);
void outblock(void);
void inparam(void);
void infunc(void);
void outfunc(void);

//token.c
void token_init(void);
void token_set(char *src);
void next(void);
void peek(void);

//vm.c
void vm_init(void);
void vm_run(int src, int debug);

//api.c
void api_init(void);
void api_register(Api fun, char *proto);
void api_call(int offset);
int api_getint(int index);
char api_getchar(int index);
char* api_getstr(int index);
void api_setint(int i);
void api_setchar(char c);
void api_setstr(char *s);

#endif

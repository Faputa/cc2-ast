all:
	@gcc api.c ast.c cc.c error.c gen.c ident.c parser.c print_ast.c token.c type.c vm.c -o cc -Wall
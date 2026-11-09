#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/*
For the language grammar, please refer to Grammar section on the github page:
  https://github.com/lightbulb12294/CSI2P-II-Mini1#grammar
*/

#define MAX_LENGTH 200
typedef enum {
	ASSIGN, ADD, SUB, MUL, DIV, REM, PREINC, PREDEC, POSTINC, POSTDEC, IDENTIFIER, CONSTANT, LPAR, RPAR, PLUS, MINUS, END
} Kind;
typedef enum {
	STMT, EXPR, ASSIGN_EXPR, ADD_EXPR, MUL_EXPR, UNARY_EXPR, POSTFIX_EXPR, PRI_EXPR
} GrammarState;
typedef struct TokenUnit {
	Kind kind;
	int val; // record the integer value or variable name
	struct TokenUnit *next;
} Token;
typedef struct ASTUnit {
	Kind kind;
	int val; // record the integer value or variable name
	struct ASTUnit *lhs, *mid, *rhs;
} AST;

/// utility interfaces

// err marco should be used when a expression error occurs.
#define err(x) {\
	puts("Compile Error!");\
	if(DEBUG) {\
		fprintf(stderr, "Error at line: %d\n", __LINE__);\
		fprintf(stderr, "Error message: %s\n", x);\
	}\
	exit(0);\
}
// You may set DEBUG=1 to debug. Remember setting back to 0 before submit.
#define DEBUG 0
// Split the input char array into token linked list.
Token *lexer(const char *in);
// Create a new token.
Token *new_token(Kind kind, int val);
// Translate a token linked list into array, return its length.
size_t token_list_to_arr(Token **head);
// Parse the token array. Return the constructed AST.
AST *parser(Token *arr, size_t len);
// Parse the token array. Return the constructed AST.
AST *parse(Token *arr, int l, int r, GrammarState S);
// Create a new AST node.
AST *new_AST(Kind kind, int val);
// Find the location of next token that fits the condition(cond). Return -1 if not found. Search direction from start to end.
int findNextSection(Token *arr, int start, int end, int (*cond)(Kind));
// Return 1 if kind is ASSIGN.
int condASSIGN(Kind kind);
// Return 1 if kind is ADD or SUB.
int condADD(Kind kind);
// Return 1 if kind is MUL, DIV, or REM.
int condMUL(Kind kind);
// Return 1 if kind is RPAR.
int condRPAR(Kind kind);
// Check if the AST is semantically right. This function will call err() automatically if check failed.
void semantic_check(AST *now);
// Check if token in lvalue.
int is_lvalue(AST *n);
// Returns a new register number.
int new_reg(void);
// Returns the memory address of a identifier.
int addr_of(AST*);
// Unwraps any parentheses
AST *unwrap_paren(AST*);
// Returns operator name for binary operators
char *opname(Kind);
// Generate ASM code.
int gen(AST *n, bool in_register[], bool changed[]);
void codegen(AST *root, bool in_register[], bool changed[]);
// Gets the register number
int get_register(AST *n, bool in_register[]);
// Free the whole AST.
void freeAST(AST *now);

/// debug interfaces

// Print token array.
void token_print(Token *in, size_t len);
// Print AST tree.
void AST_print(AST *head);

char input[MAX_LENGTH];

int main(void) {
	AST *ast_root;
	AST *temp_tree[16]; // Guaranteed that there are not over 15 testcases
	bool in_register[3] = {false};
	bool changed[3] = {false};
	int inp_count = 0;
	while (fgets(input, MAX_LENGTH, stdin) != NULL) {
		Token *content = lexer(input);
		size_t len = token_list_to_arr(&content);
		if (len == 0) continue;
		if (DEBUG) token_print(content, len);
		ast_root = parser(content, len);
		if (DEBUG) AST_print(ast_root);
		semantic_check(ast_root);
		temp_tree[inp_count] = ast_root;
		++inp_count;
		free(content);
	}
	for (int i = 0; i < inp_count; ++i) {
		codegen(temp_tree[i], in_register, changed);
	} 
	// Final stores go here
	for (int i = 0; i < 3; ++i) {
		if (changed[i] == true) printf("store [%d] r%d\n", i * 4, i);
	}

	for (int i = 0; i < inp_count; ++i) {
		freeAST(temp_tree[i]);
	}
	return 0;
}

Token *lexer(const char *in) {
	Token *head = NULL;
	Token **now = &head;
	for (int i = 0; in[i]; i++) {
		if (isspace(in[i])) // ignore space characters
			continue;
		else if (isdigit(in[i])) {
			(*now) = new_token(CONSTANT, atoi(in + i));
			while (in[i+1] && isdigit(in[i+1])) i++;
		}
		else if ('x' <= in[i] && in[i] <= 'z') // variable
			(*now) = new_token(IDENTIFIER, in[i]);
		else switch (in[i]) {
			case '=':
				(*now) = new_token(ASSIGN, 0);
				break;
			case '+':
				if (in[i+1] && in[i+1] == '+') {
					i++;
					// In lexer scope, all "++" will be labeled as PREINC.
					(*now) = new_token(PREINC, 0);
				}
				// In lexer scope, all single "+" will be labeled as PLUS.
				else (*now) = new_token(PLUS, 0);
				break;
			case '-':
				if (in[i+1] && in[i+1] == '-') {
					i++;
					// In lexer scope, all "--" will be labeled as PREDEC.
					(*now) = new_token(PREDEC, 0);
				}
				// In lexer scope, all single "-" will be labeled as MINUS.
				else (*now) = new_token(MINUS, 0);
				break;
			case '*':
				(*now) = new_token(MUL, 0);
				break;
			case '/':
				(*now) = new_token(DIV, 0);
				break;
			case '%':
				(*now) = new_token(REM, 0);
				break;
			case '(':
				(*now) = new_token(LPAR, 0);
				break;
			case ')':
				(*now) = new_token(RPAR, 0);
				break;
			case ';':
				(*now) = new_token(END, 0);
				break;
			default:
				err("Unexpected character.");
		}
		now = &((*now)->next);
	}
	return head;
}

Token *new_token(Kind kind, int val) {
	Token *res = (Token*)malloc(sizeof(Token));
	res->kind = kind;
	res->val = val;
	res->next = NULL;
	return res;
}

size_t token_list_to_arr(Token **head) {
	size_t res;
	Token *now = (*head), *del;
	for (res = 0; now != NULL; res++) now = now->next;
	now = (*head);
	if (res != 0) (*head) = (Token*)malloc(sizeof(Token) * res);
	for (int i = 0; i < res; i++) {
		(*head)[i] = (*now);
		del = now;
		now = now->next;
		free(del);
	}
	return res;
}

AST *parser(Token *arr, size_t len) {
	for (int i = 1; i < len; i++) {
		// correctly identify "ADD" and "SUB"
		if (arr[i].kind == PLUS || arr[i].kind == MINUS) {
			switch (arr[i - 1].kind) {
				case PREINC:
				case PREDEC:
				case IDENTIFIER:
				case CONSTANT:
				case RPAR:
					arr[i].kind = arr[i].kind - PLUS + ADD;
				default: break;
			}
		}
	}
	return parse(arr, 0, len - 1, STMT);
}

AST *parse(Token *arr, int l, int r, GrammarState S) {
	AST *now = NULL;
	if (l > r)
		err("Unexpected parsing range.");
	int nxt;
	switch (S) {
		case STMT:
			if (l == r && arr[l].kind == END)
				return NULL;
			else if (arr[r].kind == END)
				return parse(arr, l, r - 1, EXPR);
			else err("Expected \';\' at the end of line.");
		case EXPR:
			return parse(arr, l, r, ASSIGN_EXPR);
		case ASSIGN_EXPR:
			if ((nxt = findNextSection(arr, l, r, condASSIGN)) != -1) {
				now = new_AST(arr[nxt].kind, 0);
				now->lhs = parse(arr, l, nxt - 1, UNARY_EXPR);
				now->rhs = parse(arr, nxt + 1, r, ASSIGN_EXPR);
				return now;
			}
			return parse(arr, l, r, ADD_EXPR);
		case ADD_EXPR:
			if((nxt = findNextSection(arr, r, l, condADD)) != -1) {
				now = new_AST(arr[nxt].kind, 0);
				now->lhs = parse(arr, l, nxt - 1, ADD_EXPR);
				now->rhs = parse(arr, nxt + 1, r, MUL_EXPR);
				return now;
			}
			return parse(arr, l, r, MUL_EXPR);
		case MUL_EXPR:
			// COMPLETED
			if ((nxt = findNextSection(arr, r, l, condMUL)) != -1) {
				now = new_AST(arr[nxt].kind, 0);
				now->lhs = parse(arr, l, nxt - 1, MUL_EXPR);
				now->rhs = parse(arr, nxt + 1, r, UNARY_EXPR);
				return now;
			}
			return parse(arr, l, r, UNARY_EXPR);
		case UNARY_EXPR:
			if (arr[l].kind == PREINC || arr[l].kind == PREDEC || arr[l].kind == PLUS || arr[l].kind == MINUS) {
				now = new_AST(arr[l].kind, 0);
				now->mid = parse(arr, l + 1, r, UNARY_EXPR);
				return now;
			}
			return parse(arr, l, r, POSTFIX_EXPR);
		case POSTFIX_EXPR:
			// if expression ends with PREINC or PREDEC
			if (arr[r].kind == PREINC || arr[r].kind == PREDEC) {
				// translate "PREINC", "PREDEC" into "POSTINC", "POSTDEC"
				now = new_AST(arr[r].kind - PREINC + POSTINC, 0);
				now->mid = parse(arr, l, r - 1, POSTFIX_EXPR);
				return now;
			}
			return parse(arr, l, r, PRI_EXPR);
		case PRI_EXPR:
			if (findNextSection(arr, l, r, condRPAR) == r) {
				now = new_AST(LPAR, 0);
				now->mid = parse(arr, l + 1, r - 1, EXPR);
				return now;
			}
			if (l == r) {
				if (arr[l].kind == IDENTIFIER || arr[l].kind == CONSTANT)
					return new_AST(arr[l].kind, arr[l].val);
				err("Unexpected token during parsing.");
			}
			err("No token left for parsing.");
		default:
			err("Unexpected grammar state.");
	}
}

AST *new_AST(Kind kind, int val) {
	AST *res = (AST*)malloc(sizeof(AST));
	res->kind = kind;
	res->val = val;
	res->lhs = res->mid = res->rhs = NULL;
	return res;
}

int findNextSection(Token *arr, int start, int end, int (*cond)(Kind)) {
	int par = 0;
	int d = (start < end) ? 1 : -1;
	for (int i = start; (start < end) ? (i <= end) : (i >= end); i += d) {
		if (arr[i].kind == LPAR) par++;
		if (arr[i].kind == RPAR) par--;
		if (par == 0 && cond(arr[i].kind) == 1) return i;
	}
	return -1;
}

int condASSIGN(Kind kind) {
	return kind == ASSIGN;
}

int condADD(Kind kind) {
	return kind == ADD || kind == SUB;
}

int condMUL(Kind kind) {
	return kind == MUL || kind == DIV || kind == REM;
}

int condRPAR(Kind kind) {
	return kind == RPAR;
}

void semantic_check(AST *now) {
	
	if (now == NULL) return;
	// Left operand of '=' must be an identifier or identifier with one or more parentheses.

	AST *left = now->lhs;
	AST *right = now->rhs;
	AST *mid = now->mid;
	
	if (now->kind == ASSIGN) {

		if (!is_lvalue(left))
			err("Lvalue is required as left operand of assignment.");
	}

	if (now->kind == PREINC || now->kind == POSTINC || now->kind == PREDEC || now->kind == POSTDEC) {

		if (!is_lvalue(mid)) {
			err("Lvalue is required as operand of increment/decrement.");
		}	
	}
	semantic_check(left);
	semantic_check(right);
	semantic_check(mid);

}

int is_lvalue(AST *n) {
	while (n->kind == LPAR) {
		n = n->mid;
	}
	return n->kind == IDENTIFIER;
}

static int next_reg = 0;
int new_reg(void) {
	return next_reg++;
}

int addr_of(AST *var) {
	return (var->val - 'x') * 4;
}

AST *unwrap_paren(AST *var) {
	while (var->kind == LPAR) {
		var = var->mid;
	}
	return var;
}

char *opname(Kind k) {
	switch (k) {
		case ADD:
			return "add";
		case SUB:
			return "sub";
		case MUL:
			return "mul";
		case DIV:
			return "div";
		case REM:
			return "rem";
		default:
			return NULL;
	}
}

// returns the register number (int)
int gen(AST *n, bool in_register[], bool changed[]) {
	int r, r_new, left, right, mid;
	AST *left_node, *mid_node;
	switch (n->kind) {
		case CONSTANT:
			r = new_reg();
			printf("add r%d 0 %d\n", r, n->val);
			return r;
		case IDENTIFIER:
			r = get_register(n, in_register);
			return r;
		case ADD:
		case SUB:
		case MUL:
		case DIV:
		case REM:
			left = gen(n->lhs, in_register, changed);
			right = gen(n->rhs, in_register, changed);
			r = new_reg();
		
			// result = left OPERATOR right 
			// store the result to r
			printf("%s r%d r%d r%d\n", opname(n->kind), r, left, right);
			return r;
		case LPAR:
		case PLUS:
			return gen(n->mid, in_register, changed);
		case MINUS:
			// Store the result in a different register, since it is a different value
			mid = gen(n->mid, in_register, changed);
			r = new_reg();
			printf("sub r%d 0 r%d\n", r, mid);
			return r;
		case ASSIGN:
			left_node = unwrap_paren(n->lhs);
			right = gen(n->rhs, in_register, changed);

			// skips the helper function to avoid loading
			r = left_node->val - 'x';

			printf("add r%d 0 r%d\n", r, right);
			in_register[r] = true;
			changed[r] = true;
			return r;
		case PREINC:
			mid_node = unwrap_paren(n->mid);
			r = get_register(mid_node, in_register);
			// modify the value in the register directly
			printf("add r%d r%d 1\n", r, r);
			changed[r] = true;
			return r;
		case PREDEC:
			mid_node = unwrap_paren(n->mid);
			r = get_register(mid_node, in_register);
			printf("sub r%d r%d 1\n", r, r);
			changed[r] = true;
			return r;
		case POSTINC:
			mid_node = unwrap_paren(n->mid);
			r = get_register(mid_node, in_register);
			r_new = new_reg();
			printf("add r%d 0 r%d\n", r_new, r);
			printf("add r%d r%d 1\n", r, r);
			changed[r] = true;
			return r_new;
		case POSTDEC:
			mid_node = unwrap_paren(n->mid);
			r = get_register(mid_node, in_register);
			r_new = new_reg();
			printf("add r%d 0 r%d\n", r_new, r);
			printf("sub r%d r%d 1\n", r, r);
			changed[r] = true;
			return r_new;
		default:
			err("Unexpected AST node.");

	}
}

int get_register(AST *n, bool in_register[]) {
	int r = n->val - 'x';

	if (in_register[r] == false) {
		printf("load r%d [%d]\n", r, addr_of(n));
		in_register[r] = true;
	}

	return r;
}

void codegen(AST *root, bool in_register[], bool changed[]) {
	if (root == NULL) return;
	next_reg = 3; // 0, 1, 2 are used by x, y, z
	gen(root, in_register, changed);
}

void freeAST(AST *now) {
	if (now == NULL) return;
	freeAST(now->lhs);
	freeAST(now->mid);
	freeAST(now->rhs);
	free(now);
}

void token_print(Token *in, size_t len) {
	const static char KindName[][20] = {
		"Assign", "Add", "Sub", "Mul", "Div", "Rem", "Inc", "Dec", "Inc", "Dec", "Identifier", "Constant", "LPar", "RPar", "Plus", "Minus", "End"
	};
	const static char KindSymbol[][20] = {
		"'='", "'+'", "'-'", "'*'", "'/'", "'%'", "\"++\"", "\"--\"", "\"++\"", "\"--\"", "", "", "'('", "')'", "'+'", "'-'"
	};
	const static char format_str[] = "<Index = %3d>: %-10s, %-6s = %s\n";
	const static char format_int[] = "<Index = %3d>: %-10s, %-6s = %d\n";
	for(int i = 0; i < len; i++) {
		switch(in[i].kind) {
			case LPAR:
			case RPAR:
			case PREINC:
			case PREDEC:
			case ADD:
			case SUB:
			case MUL:
			case DIV:
			case REM:
			case ASSIGN:
			case PLUS:
			case MINUS:
				fprintf(stderr, format_str, i, KindName[in[i].kind], "symbol", KindSymbol[in[i].kind]);
				break;
			case CONSTANT:
				fprintf(stderr, format_int, i, KindName[in[i].kind], "value", in[i].val);
				break;
			case IDENTIFIER:
				fprintf(stderr, format_str, i, KindName[in[i].kind], "name", (char*)(&(in[i].val)));
				break;
			case END:
				fprintf(stderr, "<Index = %3d>: %-10s\n", i, KindName[in[i].kind]);
				break;
			default:
				fputs("=== unknown token ===", stderr);
		}
	}
}

void AST_print(AST *head) {
	static char indent_str[MAX_LENGTH] = "  ";
	static int indent = 2;
	const static char KindName[][20] = {
		"Assign", "Add", "Sub", "Mul", "Div", "Rem", "PreInc", "PreDec", "PostInc", "PostDec", "Identifier", "Constant", "Parentheses", "Parentheses", "Plus", "Minus"
	};
	const static char format[] = "%s\n";
	const static char format_str[] = "%s, <%s = %s>\n";
	const static char format_val[] = "%s, <%s = %d>\n";
	if (head == NULL) return;
	char *indent_now = indent_str + indent;
	indent_str[indent - 1] = '-';
	fprintf(stderr, "%s", indent_str);
	indent_str[indent - 1] = ' ';
	if (indent_str[indent - 2] == '`')
		indent_str[indent - 2] = ' ';
	switch (head->kind) {
		case ASSIGN:
		case ADD:
		case SUB:
		case MUL:
		case DIV:
		case REM:
		case PREINC:
		case PREDEC:
		case POSTINC:
		case POSTDEC:
		case LPAR:
		case RPAR:
		case PLUS:
		case MINUS:
			fprintf(stderr, format, KindName[head->kind]);
			break;
		case IDENTIFIER:
			fprintf(stderr, format_str, KindName[head->kind], "name", (char*)&(head->val));
			break;
		case CONSTANT:
			fprintf(stderr, format_val, KindName[head->kind], "value", head->val);
			break;
		default:
			fputs("=== unknown AST type ===", stderr);
	}
	indent += 2;
	strcpy(indent_now, "| ");
	AST_print(head->lhs);
	strcpy(indent_now, "` ");
	AST_print(head->mid);
	AST_print(head->rhs);
	indent -= 2;
	(*indent_now) = '\0';
}
/* --------------------------------------------------------------------------
 * Copyright (C) 2023 Jorge Giner Cordero
 * License: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>
 * --------------------------------------------------------------------------
 */

/* Bytecode compiler. Compiles the lines in lines.c and generates the compiled
 * program in code.c, str.c and data.c .
 */

#include <config.h>
#include "ecma55.h"
#include "arraydsc.h"
#include "dbg.h"
#include "list.h"
#include "grammar.h"
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

/* Type of variables: VARTYPE_UNDEF, VARTYPE_NUM, etc... */
static enum var_type s_vartype[N_VARNAMES][N_SUBVARS];

/* If a variable is list or table, this saves the dimensions. */
static int s_vardim[N_VARNAMES][2];

/* If a variable has been dimensioned (explicit or implicit) */
static signed char s_dimensioned[N_VARNAMES];

/* When we parse, this is incremented for each different var we see to
 * calculate the final ram size needed for the program and the position
 * of the next previously unseen variable in that ram area, taking into
 * account the arrays dimensions.
 */
static int s_ramsize;

/* For each variable, its position in ram. */
static int s_rampos[N_VARNAMES][N_SUBVARS];

/* If an OPTION BASE 0/1 has been declared already. */
static int s_option_declared;

/* Base index for array access, 0 or 1. */
static int s_base_index;

/* If we have accessed or dimensioned any array. */
static int s_array_access;

/* Struct used for to handle line references from other lines.
 * line_num:	program number line.
 * pc:		program counter where this line compiled code starts.
 *		-1 if we have not compiled the line still.
 */
struct line_pc {
	int line_num;
	int pc;
};

/* Array ordered by line number (so we can do binary search).
 * For each line, stores the pc where it starts and the number of the line.
 * pc -1 if line still not compiled.
 */
static struct line_pc *s_line_pc = NULL;

/* Length of s_line_pc */
static int s_line_pc_len = 0;

/* Index in s_line_pc of the next line. */
static int s_line_pc_top;

/* This is a reference made from pc to another line.
 * line_pc:	line we point to.
 * pc:		pc from where we point to.
 * next:	linked next node.
 */
struct line_ref {
	struct line_pc *line_pc;
	int pc;
	struct line_ref *next;
};

static struct line_ref *line_ref_list = NULL;

/* Current compiling line. */
static int s_cur_line_num;

/* If we have already printed an out of memory. */
static int s_no_mem;

struct usrfun {
	struct usrfun *next;
	int name;
	int param;
	int nparams;
	int pc;
	int vrampos;
	int stack_inc;
	int stack_dec;
};

static struct usrfun *usrfun_list = NULL;

static int s_in_fun_def = 0;
static struct usrfun *s_cur_fun = NULL;

/* This is basically the structure of the program in terms of for blocks,
 * so we can check if there are jumps into for blocks, which is forbidden.
 */
struct for_block {
	int coded_var;		/* variable for this FOR */
	int cmp_pc;		/* PC of the FOR_CMP_OP for this for */
	int start_line_num;	/* includes FOR */
	int end_line_num;	/* includes NEXT */
	struct for_block *parent;	/* Block that includes us */
	struct for_block *children;	/* First block included */
	struct for_block *next;		/* Next in children list */
};

struct for_block *s_main_block = NULL;
struct for_block *s_cur_block = NULL;

/* Here we put all the jumps */
struct jump_inf {
	struct jump_inf *next;
	int from_line;
	int to_line;
};

struct jump_inf *s_jumps = NULL;

static int s_end_seen = 0;

/* Number or errors while parsing. */
static int s_nerrors = 0;

/* We calculate here at compile time the maximum stack size needed at
 * runtime.
 */
static int s_stack_max;
static int s_stack_size;

/* Returns the number of errors. Set to 0 when init_parser() is called.
 * At max will be INT_MAX.
 */
int get_parser_nerrors(void)
{
	return s_nerrors;
}

/*
 * A new error has been seen. Only increment up to INT_MAX. */
static void new_error(void)
{
	if (s_nerrors == INT_MAX) {
		return;
	}
	s_nerrors++;
}

/*
 * Prints a compilation error.
 * If lineno < 0 doesn't print the line number.
 */
static void cerrorln(int ecode, int lineno, int nl)
{	
	if (ecode == E_NO_MEM || ecode == E_BIG_RAM) {
		/* print only once the 'no memory' error */
		if (s_no_mem) {
			return;
		}
		s_no_mem = 1;
	}
	new_error();
	eprintln(ecode, lineno);
	if (nl) {
		enl();
	}
}

/*
 * Prints a compilation error and the current line number.
 */
void cerror(int ecode, int nl)
{
	cerrorln(ecode, s_cur_line_num, nl);
}

void cwarn(int ecode)
{
	wprintln(ecode, s_cur_line_num);
}

void yyerror(char const *s)
{
	cerror(E_SYNTAX, 1);
	print_lex_last_context();
	// fprintf(stderr, "%s\n", s);
}

static void add_instr(union instruction instr)
{
	if (add_code_instr(instr) != 0) {
		cerrorln(E_NO_MEM, -1, 1);
	}
}

static void add_to_stack_size(int delta)
{
	if (s_in_fun_def && s_cur_fun != NULL) {
		if (delta > 0)
			s_cur_fun->stack_inc += delta;
		else if (delta < 0)
			s_cur_fun->stack_dec += delta;
		if (s_nerrors == 0)
			assert(s_cur_fun->stack_inc + 
				s_cur_fun->stack_dec >= 0);
	} else {
		s_stack_size += delta;
		if (s_nerrors == 0)
			assert(s_stack_size >= 0);
		if (s_stack_size > s_stack_max)
			s_stack_max = s_stack_size;
	}
}

void add_op_instr(enum vm_opcode opcode)
{
	union instruction instr;

	instr.opcode = opcode;
	add_instr(instr);

	add_to_stack_size(get_opcode_stack_inc(opcode));
	add_to_stack_size(get_opcode_stack_dec(opcode));
}

void add_id_instr(int id)
{
	union instruction instr;

	instr.id = id;
	add_instr(instr);
}

void add_num_instr(double num)
{
	union instruction instr;

	instr.num = num;
	add_instr(instr);
}

void compile_line(int num, const char *str)
{
	s_in_fun_def = 0;
	s_cur_fun = NULL;
	s_cur_line_num = num;
	s_line_pc[s_line_pc_top++].pc = get_code_size();
	add_op_instr(LINE_OP);
	add_id_instr(num);
	set_lex_input(str);

	if (s_end_seen) {
		cerror(E_LINES_AFTER_END, 1);
	}

	yyparse();
}

int get_rampos(int coded_var)
{
	int vindex1, vindex2;

	vindex1 = var_index1(coded_var);
	vindex2 = var_index2(coded_var);
	return s_rampos[vindex1][vindex2];
}

int get_parsed_ram_size(void)
{
	return s_ramsize;
}

int get_dim(int coded_var, int ndim)
{
	int vindex1;

	vindex1 = var_index1(coded_var);
	return s_vardim[vindex1][ndim];
}

/* Given max_idx the max index declared for an array and s_base_index,
 * returns the size required for the array. If the array is too big,
 * will return INT_MAX.
 */
static int adjust_dimension(int max_idx)
{
	int size;

	if (max_idx == INT_MAX && s_base_index == 0) {
		/* This would mean that the array would require INT_MAX+1
		 * elems.
		 */
		goto error;
	}
	size = max_idx - s_base_index + 1;
	if (is_ram_too_big(size)) {
		goto error;
	}
	return size;

error:	cerror(E_BIG_ARRAY, 1);
	return INT_MAX;
}

static void ram_exhausted(void)
{
	s_ramsize = INT_MAX;
	cerror(E_BIG_RAM, 1);
}

static void add_size_to_ram(int len)
{
	if (iadd_overflows_int(s_ramsize, len)) {
		ram_exhausted();
	} else if (is_ram_too_big(s_ramsize + len)) {
		ram_exhausted();
	} else {
		s_ramsize += len;
	}
}

static void add_list_size_to_ram(int len1)
{
	add_size_to_ram(len1);
}

static void add_table_size_to_ram(int len1, int len2)
{
	if (imul_overflows_int(len1, len2)) {
		ram_exhausted();
	} else {
		add_size_to_ram(len1 * len2);
	}
}

static void print_var_type(int var_type)
{
	switch (var_type) {
	case VARTYPE_LIST:
		fputs("a one-dimension array", stderr);
		break;
	case VARTYPE_TABLE:
		fputs("a two-dimension array", stderr);
		break;
	case VARTYPE_NUM:
		fputs("a numeric variable", stderr);
		break;
	default:
		fputs("?", stderr);
	}
}

static void type_mismatch(int column, int coded_var, int old_type, int now_type)
{
	cerror(E_TYPE_MISMATCH, 0);
	print_var(stderr, coded_var);
	enl();
	fputs(" info: it was previously used ", stderr);
	switch (old_type) {
	case VARTYPE_LIST: 
		fputs("or DIM as ", stderr);
		print_var_type(VARTYPE_LIST);
		enl();
		break;
	case VARTYPE_TABLE: 
		fputs("or DIM as ", stderr);
		print_var_type(VARTYPE_TABLE);
		enl();
		break;
	case VARTYPE_NUM: 
		fputs("as ", stderr);
		print_var_type(VARTYPE_NUM);
		enl();
		break;
	default: 
		assert(0);
	}
	/*
	fputs(" info: here it is used as ", stderr);
	print_var_type(var_type);
	enl();
	*/
	print_lex_context(column);
}

void numvar_declared(int column, int coded_var, int var_type)
{
	int vindex1, vindex2;
	
	if (is_numvar_wdigit(coded_var) && var_type != VARTYPE_NUM) {
		cerror(E_NUMVAR_ARRAY, 0);
		print_var(stderr, coded_var);
		enl();
		print_lex_context(column);
		return;
	}

	vindex1 = var_index1(coded_var);
	vindex2 = var_index2(coded_var);
	if (s_vartype[vindex1][vindex2] == VARTYPE_UNDEF) {
		s_vartype[vindex1][vindex2] = var_type;
		s_rampos[vindex1][vindex2] = s_ramsize;
		set_ram_var_pos(s_ramsize, coded_var); 
		if (var_type == VARTYPE_LIST || var_type == VARTYPE_TABLE) {
			s_dimensioned[vindex1] = 1;
			s_array_access = 1;
		}
		if (var_type == VARTYPE_LIST) {
			set_array_descriptor(vindex1, s_ramsize,
					     s_vardim[vindex1][0], 1);
			add_list_size_to_ram(s_vardim[vindex1][0]);
		} else if (var_type == VARTYPE_TABLE) {
			set_array_descriptor(vindex1, s_ramsize,
					     s_vardim[vindex1][0],
					     s_vardim[vindex1][1]);
			add_table_size_to_ram(s_vardim[vindex1][0],
					      s_vardim[vindex1][1]);
		} else {
			add_size_to_ram(1);		
		}
		return;
	}

	if (s_vartype[vindex1][vindex2] != var_type) {
		type_mismatch(column, coded_var, s_vartype[vindex1][vindex2],
			      var_type);
	}
}

/* Returns 0 if something failed. */
void numvar_dimensioned(int column, int idx1_col, int idx2_col,
		        int coded_var, int var_type, int max_idx1,
			int max_idx2)
{
	int vindex1, vindex2, rampos;
	
	rampos = s_ramsize;
	if (is_numvar_wdigit(coded_var) && var_type != VARTYPE_NUM) {
		cerror(E_NUMVAR_ARRAY, 0);
		print_var(stderr, coded_var);
		enl();
		print_lex_context(column);
		return;
	}

	if (max_idx1 < s_base_index) {
		cerror(E_INVAL_DIM, 1);
		print_lex_context(idx1_col);
		max_idx1 = s_base_index;
	}

	if (var_type == VARTYPE_TABLE && max_idx2 < s_base_index) {
		cerror(E_INVAL_DIM, 1);
		print_lex_context(idx2_col);
		max_idx2 = s_base_index;
	}

	vindex1 = var_index1(coded_var);
	vindex2 = var_index2(coded_var);
	if (s_vartype[vindex1][vindex2] == VARTYPE_UNDEF) {		
		s_vartype[vindex1][vindex2] = var_type;
		s_dimensioned[vindex1] = 1;
		s_array_access = 1;
		s_rampos[vindex1][vindex2] = rampos;
		s_vardim[vindex1][0] = adjust_dimension(max_idx1);
		set_ram_var_pos(s_ramsize, coded_var);
		if (var_type == VARTYPE_LIST) {
			set_array_descriptor(vindex1, rampos,
					     s_vardim[vindex1][0], 1);
			add_list_size_to_ram(s_vardim[vindex1][0]);
		} else if (var_type == VARTYPE_TABLE) {
			s_vardim[vindex1][1] = adjust_dimension(max_idx2);
			set_array_descriptor(vindex1, rampos,
					     s_vardim[vindex1][0],
					     s_vardim[vindex1][1]);
			add_table_size_to_ram(s_vardim[vindex1][0],
					      s_vardim[vindex1][1]);
		}
		return;
	}

	if (s_vartype[vindex1][vindex2] != var_type) {
		type_mismatch(column, coded_var, s_vartype[vindex1][vindex2],
			      var_type);
		return;
	}

	if (s_dimensioned[vindex1]) {
		cerror(E_DUP_DIM, 0);
		print_var(stderr, coded_var);
		enl();
		print_lex_context(column);
	}
}

void option_decl(int column, int op_col, int n)
{
	if (s_option_declared) {
		cerror(E_DUP_OPTION, 1);
		print_lex_context(column);
		return;
	}

	s_option_declared = 1;

	if (s_array_access) {
		cerror(E_LATE_OPTION, 1);
		print_lex_context(column);
	}

	if (n == 0) {
		s_base_index = 0;
	} else if (n == 1) {
		s_base_index = 1;
	} else {
		cerror(E_SYNTAX, 1);
		print_lex_context(op_col);
	}
}

int get_parsed_base(void)
{
	return s_base_index;
}

static int compare_line_pc_elem(const void *a, const void *b)
{
	struct line_pc *p, *q;

	p = (struct line_pc *) a;
	q = (struct line_pc *) b;
	if (p->line_num < q->line_num)
		return -1;
	else if (p->line_num > q->line_num)
		return 1;
	
	return 0;
}

static struct line_pc *find_line(int line_num)
{
	return bsearch(&line_num, s_line_pc, s_line_pc_len, sizeof(*s_line_pc),
	    compare_line_pc_elem);
}

static void patch_line_references(void)
{
	struct line_ref *lr;

	for (lr = line_ref_list; lr != NULL; lr = lr->next) {
		set_id_instr(lr->pc, lr->line_pc->pc);
	}
}

void strvar_decl(int coded_var)
{
	int index1, index2;

	index1 = var_index1(coded_var);
	index2 = var_index2(coded_var);
	if (s_rampos[index1][index2] == -1) {
		s_rampos[index1][index2] = s_ramsize;
		set_ram_var_pos(s_ramsize, coded_var);
		add_size_to_ram(1);
	}
}

int str_decl(const char *start, size_t len)
{
	int pos;

	if (add_string(start, len, &pos) != 0) {
		cerror(E_NO_MEM, 1);
		return 0;
	}

	return pos;
}

/* Allocates memory and inits s_line_pc.
 * Returns 0 on success, E_NO_MEM if no memory.
 */
static int init_line_pc(void)
{
	int i;
	struct basic_line *p;

	s_line_pc_len = s_line_list_size;
	if (imul_overflows_int(s_line_list_size, (int) sizeof *s_line_pc))
		return E_NO_MEM;

	if ((s_line_pc = malloc(s_line_list_size * sizeof *s_line_pc)) == NULL)
		return E_NO_MEM;

	s_line_pc_len = s_line_list_size;
	s_line_pc_top = 0;

	for (p = s_line_list, i = 0; p != NULL; p = p->next, i++) {
		s_line_pc[i].line_num = p->number;
		s_line_pc[i].pc = -1;
	}

	return 0;
}

static void free_line_pc(void)
{
	if (s_line_pc != NULL) {
		free(s_line_pc);
		s_line_pc = NULL;
		s_line_pc_len = 0;
		s_line_pc_top = 0;
	}
}

void data_str_decl(int i, enum data_datum_type type)
{
	if (add_data_str(i, type) != 0) {
		cerror(E_NO_MEM, 1);
	}
}

static struct usrfun *find_usrfun(int name)
{
	struct usrfun *p;

	for (p = usrfun_list; p != NULL; p = p->next)
		if (p->name == name)
			return p;

	return NULL;
}

void fun_decl(int column, int name, int nparams, int param, int pc)
{
	struct usrfun *p;

	s_in_fun_def = 1;

	p = find_usrfun(name);
	if (p != NULL) {
		cerror(E_FUN_REDECLARED, 1);
		print_lex_context(column);
		s_cur_fun = p;
		return;
	}

	if ((p = malloc(sizeof *p)) == NULL) {
		cerror(E_NO_MEM, 1);
		return;
	}

	p->name = name;
	p->nparams = nparams;
	p->param = param;
	p->pc = pc;
	p->vrampos = s_ramsize;
	p->stack_inc = 0;
	p->stack_dec = 0;
	add_size_to_ram(1);
	s_cur_fun = p;
	list_add(usrfun_list, (struct usrfun *) NULL, p);

	/*
	printf("defined fun %c (%d, %c) %d\n", name, nparams,
		get_var_letter(param), pc);
	*/
}

void numvar_expr(int column, int coded_var)
{
	if (s_in_fun_def && s_cur_fun != NULL && s_cur_fun->nparams > 0 &&
	    coded_var == s_cur_fun->param) {
		add_op_instr(GET_FN_VAR_OP);
		add_id_instr(usrfun_list->vrampos);
	} else {
		numvar_declared(column, coded_var, VARTYPE_NUM);
		add_op_instr(GET_VAR_OP);
		add_id_instr(get_rampos(coded_var));
	}
}

void list_expr(int column, int coded_var)
{
	if (s_in_fun_def && s_cur_fun != NULL && s_cur_fun->nparams > 0 &&
		coded_var == s_cur_fun->param)
	{
		cerror(E_FUNARG_AS_ARRAY, 1);
		print_lex_context(column);
	} else {
		numvar_declared(column, coded_var, VARTYPE_LIST);
		add_op_instr(GET_LIST_OP);
		add_id_instr(var_index1(coded_var));
	}
}

void table_expr(int column, int coded_var)
{
	if (s_in_fun_def &&s_cur_fun != NULL && s_cur_fun->nparams > 0 &&
		coded_var == s_cur_fun->param)
	{
		cerror(E_FUNARG_AS_ARRAY, 1);
		print_lex_context(column);
	} else {
		numvar_declared(column, coded_var, VARTYPE_TABLE);
		add_op_instr(GET_TABLE_OP);
		add_id_instr(var_index1(coded_var));
	}
}

void check_type(YYSTYPE a, enum pstack_type t)
{
	if (a.type != t) {
		if (t == PSTACK_NUM) {
			cerror(E_NUM_EXPECT, 1);
			print_lex_context(a.column);
		} else {
			cerror(E_STR_EXPECT, 1);
			print_lex_context(a.column);
		}
	}
}

int binary_expr(YYSTYPE a, YYSTYPE b, int op)
{
	check_type(a, PSTACK_NUM);
	check_type(b, PSTACK_NUM);		
	add_op_instr(op);
	return PSTACK_NUM;
}

void boolean_expr(YYSTYPE a, YYSTYPE relop, YYSTYPE b)
{
	if (a.type == PSTACK_NUM) {
		check_type(b, PSTACK_NUM);
		switch (relop.u.i) {
		case '<': add_op_instr(LESS_OP); break;
		case '>': add_op_instr(GREATER_OP); break;
		case '=': add_op_instr(EQ_OP); break;
		case LESS_EQ: add_op_instr(LESS_EQ_OP); break;
		case GREATER_EQ: add_op_instr(GREATER_EQ_OP); break;
		case NOT_EQ: add_op_instr(NOT_EQ_OP); break;
		}
	} else {
		check_type(b, PSTACK_STR);
		switch (relop.u.i) {
		case '=': add_op_instr(EQ_STR_OP); break;
		case NOT_EQ: add_op_instr(NOT_EQ_STR_OP); break;
		default:
			cerror(E_STR_REL_EQ, 1);
			print_lex_context(relop.column);
		}
	}
}

void usrfun_call(int column, int name, int nparams)
{
	struct usrfun *p;

	p = find_usrfun(name);
	if (p == NULL || p == s_cur_fun) {
		cerror(E_UNDEF_FUN, 0);
		fprintf(stderr, "FN%c\n", (char) name);
		print_lex_context(column);
		return;
	}

	if (p->nparams != nparams) {
		cerror(E_BAD_NPARAMS, 0);
		fprintf(stderr, "FN%c\n", (char) name);
		print_lex_context(column);
		return;
	}

	if (p->nparams > 0) {
		add_op_instr(LET_VAR_OP);
		add_id_instr(p->vrampos);
	}

	add_op_instr(GOSUB_OP);
	add_id_instr(p->pc);

	add_to_stack_size(p->stack_inc);
	add_to_stack_size(p->stack_dec);
}

void ifun_call(int column, int ifun, int nparams)
{
	if (get_ifun_nparams(ifun) != nparams) {
		cerror(E_BAD_NPARAMS, 0);
		fprintf(stderr, "%s\n", get_ifun_name(ifun));
		print_lex_context(column);
		return;
	}

	if (nparams == 0) {
		add_op_instr(IFUN0_OP);
	} else {
		add_op_instr(IFUN1_OP);
	}

	add_id_instr(ifun);
}

static void add_jump(int from_line, int to_line)
{
	struct jump_inf *jump;

	if ((jump = malloc(sizeof *jump)) == NULL) {
		cerror(E_NO_MEM, 1);
		return;
	}

	jump->from_line = from_line;
	jump->to_line = to_line;
	jump->next = NULL;
	list_add(s_jumps, (struct jump_inf *) NULL, jump);
}

static enum error_code add_for_block(int line_num)
{
	struct for_block *block;

	if ((block = malloc(sizeof *block)) == NULL)
		return E_NO_MEM;
	
	memset(block, 0, sizeof *block);
	block->start_line_num = line_num;
	block->end_line_num = line_num;

	if (s_cur_block == NULL) {
		list_add(s_main_block, (struct for_block *) NULL, block);
	} else {
		list_add(s_cur_block->children, (struct for_block *) NULL,
		    block);
	}

	block->parent = s_cur_block;
	s_cur_block = block;

	return 0;
}

static void end_for_block(int line_num)
{
	if (s_cur_block != NULL) {
		s_cur_block->end_line_num = line_num;
		if (s_cur_block->parent != NULL)
			s_cur_block = s_cur_block->parent;
	}
}

static void free_block(struct for_block *b)
{
	struct for_block *p, *q;

	if (b == NULL)
		return;

	for (p = b->children; p != NULL; p = q) {
		q = p->next;
		free_block(p);
	}
	free(b);
}

static void free_blocks(void)
{
	free_block(s_main_block);
	s_main_block = NULL;
	s_cur_block = NULL;
}

static struct for_block *find_line_in_block(int line_num, struct for_block *b)
{
	struct for_block *p, *r;

	if (b == NULL)
		return NULL;
		
	for (p = b->children; p != NULL; p = p->next) {
		if ((r = find_line_in_block(line_num, p)) != NULL)
			return r;
	}

	/* Don't consider the FOR line as inside (line_num > ... */
	if (line_num > b->start_line_num &&
	    line_num <= b->end_line_num)
		    return b;
	
	return NULL;
}

static struct for_block *find_line_in_blocks(int line_num)
{
	return find_line_in_block(line_num, s_main_block);
}

static void check_jump(struct jump_inf *p)
{
	struct for_block *fb, *tb;

	fb = find_line_in_blocks(p->from_line);
	tb = find_line_in_blocks(p->to_line);

	while (fb != tb && fb != NULL) {
		fb = fb->parent;
	}

	if (fb == NULL) {
		cerrorln(E_JUMP_INTO_FOR, p->from_line, 1);
	}
}

static void check_jumps(void)
{
	struct jump_inf *p;

	for (p = s_jumps; p != NULL; p = p->next) {
		check_jump(p);
	}
}

static void check_same_outer_for(int var_column, int coded_var)
{
	struct for_block *b;

	for (b = s_cur_block; b != s_main_block; b = b->parent) {
		if (b->coded_var == coded_var) {
			cerrorln(E_NESTED_FOR, s_cur_line_num, 0);
			fprintf(stderr, "%d\n", b->start_line_num);
			print_lex_context(var_column);
			break;
		}
	}
}

void for_decl(int var_column, int coded_var)
{
	int pc;
	
	/* Check if there is a parent FOR with the same var */
	check_same_outer_for(var_column, coded_var);

	if (add_for_block(s_cur_line_num) != 0) {
		cerrorln(E_NO_MEM, -1, 1);
		return;
	}
	
	numvar_declared(var_column, coded_var, VARTYPE_NUM);
	add_op_instr(FOR_OP);

	/* own2, step */
	add_id_instr(s_ramsize);
	add_size_to_ram(1);

	/* own1, limit */
	add_id_instr(s_ramsize);
	add_size_to_ram(1);

	/* var */
	add_id_instr(get_rampos(coded_var));

	pc = get_code_size();
	add_op_instr(FOR_CMP_OP);
	add_id_instr(0);

	s_cur_block->coded_var = coded_var;
	s_cur_block->cmp_pc = pc;
}

void next_decl(int var_column, int coded_var)
{
	struct for_block *p;

	p = s_cur_block;
	if (p == s_main_block || p->coded_var != coded_var) {
		cerrorln(E_NEXT_WOUT_FOR, s_cur_line_num, 1);
		return;
	}

	numvar_declared(var_column, coded_var, VARTYPE_NUM);
	add_op_instr(NEXT_OP);
	add_id_instr(p->cmp_pc);
	set_id_instr(p->cmp_pc + 1, get_code_size());
	end_for_block(s_cur_line_num);
}

void add_line_ref(int column, int line_num)
{
	struct line_pc *lpc;
	struct line_ref *lrp;

	add_jump(s_cur_line_num, line_num);

	if ((lpc = find_line(line_num)) == NULL) {
		cerror(E_NO_LINE, 1);
		print_lex_context(column);
		return;
	}

	if (lpc->pc >= 0) {
		/* We know the jump address. */
		add_id_instr(lpc->pc);
		return;
	}

	/* We don't know still the jump address. */
	if ((lrp = malloc(sizeof *lrp)) == NULL) {
		cerrorln(E_NO_MEM, -1, 1);
		return;
	}
	
	lrp->line_pc = lpc;
	lrp->pc = get_code_size();
	lrp->next = line_ref_list;
	line_ref_list = lrp;
	add_id_instr(0);
}

static void check_fors_without_next(void)
{
	struct for_block *b;

	for (b = s_cur_block; b != s_main_block; b = b->parent) {
		cerrorln(E_FOR_WITHOUT_NEXT, b->start_line_num, 1);
	}
}

int get_parsed_stack_size(void)
{
	return s_stack_max;
}

void end_parsing(void)
{
	s_main_block->end_line_num = s_cur_line_num;
	if (s_end_seen == 0) {
		cerror(E_END_UNSEEN, 1);
	}
	
	if (s_nerrors == 0) {
		check_fors_without_next();
	}

	if (s_nerrors == 0) {
		patch_line_references();
	}

	if (s_nerrors == 0) {
		check_jumps();
	}
}

/* Inits the parser, to call before yyparse().
 * Returns 0 on success; E_NO_MEM if no memory.
 */
int init_parser(void)
{
	int i, j;

	s_nerrors = 0;
	s_no_mem = 0;
	s_ramsize = 0;
	s_base_index = 0;
	s_option_declared = 0;
	s_array_access = 0;
	s_end_seen = 0;
	s_stack_size = 0;
	s_stack_max = 0;
	reset_array_descriptors();
	reset_ram_var_map();
	for (i = 0; i < N_VARNAMES; i++) {
		s_vardim[i][0] = s_vardim[i][1] = 11;
		s_dimensioned[i] = 0;
		for (j = 0; j < N_SUBVARS; j++) {
			s_vartype[i][j] = VARTYPE_UNDEF;
			s_rampos[i][j] = -1;
		}
	}

	if (add_for_block(1) != 0)
		return E_NO_MEM;

	return init_line_pc();
}

void end_decl(void)
{
	s_end_seen = 1;
	add_op_instr(END_OP);
}

/* Frees all the parser allocated data. To call after yyparse(). */
void free_parser(void)
{
	free_line_pc();
	list_free_all(line_ref_list);
	list_free_all(usrfun_list);
	list_free_all(s_jumps);
	free_blocks();
}

/* ===========================================================================
 * bas55, an implementation of the Minimal BASIC programming language.
 * ===========================================================================
 */

#ifndef ECMA55_H
#define ECMA55_H

#ifndef STDIO_H
#include <stdio.h>
#endif

/* Max number of characters in a BASIC line, without new line characters. */
#define LINE_MAX_CHARS	80

/* Maximum line number allowed. */
#define LINE_NUM_MAX	9999

/* Maximum number of errors allowed */
#define MAX_ERRORS	20

/* Significant digits that we read / print */
#define PRECISION_DIGITS	6

/* Different variable names */
enum { N_VARNAMES = 'Z' - 'A' + 1 };

/* Vars 0-9 + without digit + strvar */
enum { N_SUBVARS = 12 };

/* Number of elements in an array. */
#define NELEMS(v)			(sizeof(v) / sizeof(v[0]))

/* 1 if the multiplication of two positive 'signed int' will overflow
 * INT_MAX. */
#define imul_overflows_int(a,b)		((INT_MAX / (b)) < (a))

/* 1 if the multiplication of two positive 'signed int' as unsigned will
 * overflow UINT_MAX. */
#define imul_overflows_uint(a,b)	((UINT_MAX/(b)) < ((unsigned int)(a)))

/* 1 if the sum of two positive 'signed int' will overflow INT_MAX. */
#define iadd_overflows_int(a,b)		((INT_MAX - (b)) < (a))

/* ecma55.c */

void print_version(FILE *);
void print_copyright(FILE *);

/* edit.c */

void edit(void);

/* util.c */

enum error_code grow_array(void *p, int elem_size, int cur_len, int grow_k,
    void **new_array, int *new_len);
size_t min_size(size_t a, size_t b);
void toupper_str(char *str);
void copy_to_str(char *dst, const char *src, size_t len);
int is_little_endian(void);
int infinite_sign(double d);
int is_nan(double d);
double round(double d);
int round_to_int(double d);
void print_chars(FILE *f, const char *s, size_t len);

/* getlin.c */

void get_line_init(void);
void get_line_set_question_mode(int set);
enum error_code get_line(const char *prompt, char *buf, int maxlen, FILE *fp);

/* line.c */

/**
 * The BASIC line.
 *
 * 'number'	The line number.
 * 'str'	Pointer to the line text which is allocated at the end of the
 *		struct. It does not contain the line number.
 * 'next'	Next line or NULL.
 */
struct basic_line {
	int number;
	char *str;
	struct basic_line *next;
};

extern struct basic_line *s_line_list;
extern int s_line_list_size;
extern int s_program_ok;
extern int s_source_changed;

void del_lines(void);
enum error_code add_line(int line_num, const char *start, const char *end);
void del_line(int line_num);
enum error_code renum_lines(void);
int is_greatest_line(int lineno);
int line_exists(int lineno);

/* err.c */

enum error_code {
	/* GENERAL */

	E_OK,
	E_NO_MEM,

	/* BASIC */
	E_INVAL_LINE_NUM,
	E_LINE_TOO_LONG,
	E_INVAL_CMD,
	E_INDEX_RANGE,
	E_STACK_OFLOW,
	E_STACK_UFLOW,
	E_SYNTAX,
	E_NO_LINE,
	E_DUP_OPTION,
	E_LATE_OPTION,
	E_DUP_DIM,
	E_TYPE_MISMATCH,
	E_INVAL_DIM,
	E_NUMVAR_ARRAY,
	E_BIG_ARRAY,
	E_BIG_RAM,
	E_NEXT_WOUT_FOR,
	E_STR_NOEND,
	E_INVAL_TAB,
	E_FUN_REDECLARED,
	E_FUNARG_AS_ARRAY,
	E_UNDEF_FUN,
	E_BAD_NPARAMS,
	E_TOO_FEW_INPUT,
	E_TOO_MUCH_INPUT,
	E_VOID_INPUT,
	E_CONST_OVERFLOW,
	E_JUMP_INTO_FOR,
	E_FNAME_TOO_LONG,
	E_EOF,
	E_FOPEN,
	E_SPACE_LINE_NUM,
	E_EMPTY_LINE,
	E_BAD_NARGS,
	E_BAD_FNAME,
	E_END_UNSEEN,
	E_LINES_AFTER_END,
	E_DIV_BY_ZERO,
	E_OP_OVERFLOW,
	E_ZERO_POW_NEG,
	E_NEG_POW_REAL,
	E_FOR_WITHOUT_NEXT,
	E_NESTED_FOR,
	E_DOM,
	E_INVAL_CHARS,
	E_DUP_LINE,
	E_INVAL_LINE_ORDER,
	E_BIGNUM,
	E_INIT_VAR,
	E_INIT_ARRAY,
	E_READ_OFLOW,
	E_READ_STR,
	E_KEYW_SPC
};

void eprint(enum error_code ecode);
void eprintln(enum error_code ecode, int lineno);
void wprint(enum error_code ecode);
void wprintln(enum error_code ecode, int lineno);
void enl(void);
void eprogname(void);

/* cmd.c */

struct cmd_arg {
	const char *str;
	size_t len;
};

extern int s_debug_mode;

void parse_n_run_cmd(const char *str);
int load(const char *fname, int max_errors, int batch_mode);
void run_cmd(struct cmd_arg *args, int nargs);

/* str.c */

/* Reference counted strings. */
struct refcnt_str {
	char *str;
	int count;
};

extern struct refcnt_str **strings;
extern int nstrings;

void free_strings(void);
int init_strings(void);
int add_string(const char *start, size_t len, int *pos);
void inc_string_refcount(int i);
void dec_string_refcount(int i);
void set_string_refcount(int i, int n);
void mark_const_strings(void);
void reset_strings(void);

/* data.c */

enum data_datum_type {
	DATA_DATUM_QUOTED_STR,
	DATA_DATUM_UNQUOTED_STR,
};

void free_data(void);
enum error_code add_data_str(int i, enum data_datum_type type);

void restore_data(void);
enum error_code read_data_str(int *i, enum data_datum_type *type);

/* lex.c */

extern const char *s_lex_str_start;
extern const char *s_lex_str_end;

void set_lex_input(const char *str);
void print_lex_context(int column);
int chk_basic_chars(const char *s, size_t len, int ignore_case, size_t *index);
int yylex(void);

/* ifun.c */

int get_internal_fun(const char *name);
int get_ifun_nparams(int i);
const char *get_ifun_name(int i);
double call_ifun0(int i);
double call_ifun1(int i, double d);

/* vm.c */

enum vm_opcode {
	PUSH_NUM_OP,
	PUSH_STR_OP,
	PRINT_NL_OP,
	PRINT_COMMA_OP,
	PRINT_TAB_OP,
	PRINT_NUM_OP,
	PRINT_STR_OP,
	LET_VAR_OP,
	LET_LIST_OP,
	LET_TABLE_OP,
	LET_STRVAR_OP,
	GET_VAR_OP,
	GET_FN_VAR_OP,
	GET_STRVAR_OP,
	GET_LIST_OP,
	GET_TABLE_OP,
	ADD_OP,
	SUB_OP,
	MUL_OP,
	DIV_OP,
	POW_OP,
	NEG_OP,
	LINE_OP,
	GOSUB_OP,
	RETURN_OP,
	GOTO_OP,
	ON_GOTO_OP,
	GOTO_IF_TRUE_OP,
	LESS_OP,
	GREATER_OP,
	LESS_EQ_OP,
	GREATER_EQ_OP,
	EQ_OP,
	NOT_EQ_OP,
	EQ_STR_OP,
	NOT_EQ_STR_OP,
	FOR_OP,
	FOR_CMP_OP,
	NEXT_OP,
	RESTORE_OP,
	READ_VAR_OP,
	READ_LIST_OP,
	READ_TABLE_OP,
	READ_STRVAR_OP,
	IFUN0_OP,
	IFUN1_OP,
	RANDOMIZE_OP,
	INPUT_OP,
	INPUT_NUM_OP,
	INPUT_STR_OP,
	INPUT_END_OP,
	INPUT_LIST_OP,
	INPUT_TABLE_OP,
	END_OP,
	VM_NOPS
};

void reset_ram_var_map(void);
void set_ram_var_pos(int rampos, int coded_var);
void reset_array_descriptors(void);
void set_array_descriptor(int vindex, int rampos, int dim1, int dim2);
int is_ram_too_big(int ramsize);
void set_gosub_stack_capacity(int capacity);
int get_opcode_stack_inc(int opcode);
int get_opcode_stack_dec(int opcode);
void run(int ramsize, int array_base_index, int stack_size);

/* code.c */

union instruction {
	enum vm_opcode opcode;
	int id;
	double num;
};

union instruction *code;

void free_code(void);
int get_code_size(void);
enum error_code add_code_instr(union instruction instr);
void set_id_instr(int i, int id);

/* codedvar.c */

int encode_var(const char *var_name);
int is_strvar(int coded_var);
int is_numvar(int coded_var);
int is_numvar_wdigit(int coded_var);
int get_var_letter(int coded_var);
int get_var_suffix(int coded_var);
int var_index1(int coded_var);
int var_index2(int coded_var);
void print_var(FILE *f, int coded_var);

/* parse.c */

/*
enum {
	PSTACK_TOK,
	PSTACK_I,
	PSTACK_NUM,
	PSTACK_FUN_PARAM,
	PSTACK_STR
};
*/

struct pstack_value {
	int column;
	union {
		int i;
		struct {
			double d;
			int i;
		} num;
		struct {
			int param;
			int nparams;
		} fun_param;
		struct {
			const char *start;
			size_t len;
		} str;
	} u;
};

#define YYSTYPE struct pstack_value

enum var_type {
	VARTYPE_UNDEF,
	VARTYPE_NUM,
	VARTYPE_LIST,
	VARTYPE_TABLE,
	VARTYPE_STR
};

int init_parser(void);
void compile_line(int num, const char *str);
void end_parsing(void);
void free_parser(void);
int get_parser_nerrors(void);

int get_parsed_ram_size(void);
int get_parsed_base(void);
int get_parsed_stack_size(void);

void cerror(int ecode, int nl);
void cwarn(int ecode);
void yyerror(char const *);

void add_op_instr(enum vm_opcode opcode);
void add_id_instr(int id);
void add_num_instr(double num);

int get_rampos(int coded_var);
int get_dim(int coded_var, int ndim);

void data_num_decl(double d);
void data_str_decl(int i, enum data_datum_type type);
void numvar_declared(int column, int coded_var, int var_type);
void numvar_dimensioned(int column, int idx1_col, int idx2_col, int coded_var,
			int var_type, int max_idx1, int max_idx2);
void option_decl(int base);
void add_line_ref(int line_num);
void for_decl(int var_column, int coded_var);
void next_decl(int var_column, int coded_var);
void strvar_decl(int coded_var);
int str_decl(const char *start, size_t len);
void fun_decl(int name, int nparams, int param, int pc);
void numvar_expr(int column, int coded_var);
void list_expr(int column, int coded_var);
void table_expr(int column, int coded_var);
void usrfun_call(int name, int nparams);
void ifun_call(int ifun, int nparams);
void end_decl(void);

/* datalex.c */

enum num_type {
	NUM_TYPE_NONE,
	NUM_TYPE_INT,
	NUM_TYPE_FLOAT
};

union data_elem {
	double num;
	struct {
		const char *start;
		size_t len;
	} str;
};

enum data_elem_type {
	DATA_ELEM_EOF,
	DATA_ELEM_NUM,
	DATA_ELEM_QUOTED_STR,
	DATA_ELEM_UNQUOTED_STR,
	DATA_ELEM_COMMA,
	DATA_ELEM_INVAL_CHAR
};

enum data_elem_as {
	DATA_ELEM_AS_IS,
	DATA_ELEM_AS_UNQUOTED_STR
};

void parse_quoted_str(const char *start, size_t *len);
enum num_type check_if_number(const char *p);
int parse_int(const char *start, size_t *len);
double parse_double(const char *start, size_t *len);
enum data_elem_type parse_data_elem(union data_elem *delem,
    const char *start, size_t *len, enum data_elem_as parse_as);

#endif

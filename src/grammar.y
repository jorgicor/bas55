%{
/*
Copyright (c) 2014, 2015 Jorge Giner Cordero

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <stddef.h>
#include <stdio.h>
#include "ecma55.h"

#define YYERROR_VERBOSE

static int s_on_goto_nelems;
static int s_on_goto_pc;
static int s_save_pc;
%}

%token BASE DATA DEF DIM END FOR GO GOSUB GOTO
%token IF INPUT LET NEXT ON OPTION PRINT RANDOMIZE
%token READ REM RESTORE RETURN STEP STOP SUB TAB
%token THEN TO
%token LESS_EQ GREATER_EQ NOT_EQ
%token BAD_ID INVAL_CHAR

%union {
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
}

%token <num> NUM
%token <num> INT
%token <i> NUMVAR
%token <i> STRVAR
%token <str> STR
%token <str> QUOTED_STR
%token <i> USRFN
%token <i> IFUN
%type <i> rel eq_rel
%type <fun_param> fnparam
%type <i> var_loc_rest

%left '-' '+'
%left '*' '/'
%left NEG	/* unary minus */
%left '^'

%%

stmnt:
	  rem_stmnt
	| end_stmnt
	| dim_stmnt
	| let_stmnt
	| print_stmnt
	| goto_stmnt
	| gosub_stmnt
	| return_stmnt
	| stop_stmnt
	| on_stmnt
	| if_stmnt
	| for_stmnt
	| next_stmnt
	| def_stmnt
	| input_stmnt
	| read_stmnt
	| restore_stmnt
	| data_stmnt
	| option_stmnt
	| randomize_stmnt
	;

rem_stmnt:
	REM
	;

end_stmnt:
	END	{ end_decl(); }
	;
	
goto_stmnt:
	goto INT
		{
			add_op_instr(GOTO_OP);
			add_line_ref($2.i);
		}
	;
	
goto:
	GOTO
	| GO TO
	;
	
gosub_stmnt:
	gosub INT
		{
			add_op_instr(GOSUB_OP);
			add_line_ref($2.i);
		}
	;
	
gosub:
	GOSUB
	| GO SUB
	;

return_stmnt:
	RETURN		{ add_op_instr(RETURN_OP); }
	;
	
stop_stmnt:
	STOP		{ add_op_instr(END_OP); }
	;
	
on_stmnt:
	ON expr goto
		{ 			
			add_op_instr(ON_GOTO_OP);
			s_on_goto_nelems = 0;
			s_on_goto_pc = get_code_size();
			add_id_instr(0);
		}
		num_list
		{
			set_id_instr(s_on_goto_pc, s_on_goto_nelems);
		}
	;
	
num_list:
	num_list_int
	| num_list ',' num_list_int	
	;

num_list_int:
	INT
		{ 
			s_on_goto_nelems++;
			add_line_ref($1.i);
		}
	;
	
if_stmnt:
	IF expr rel expr THEN INT
		{
			switch ($3) {
			case '<': add_op_instr(LESS_OP); break;
			case '>': add_op_instr(GREATER_OP); break;
			case '=': add_op_instr(EQ_OP); break;
			case LESS_EQ: add_op_instr(LESS_EQ_OP); break;
			case GREATER_EQ: add_op_instr(GREATER_EQ_OP); break;
			case NOT_EQ: add_op_instr(NOT_EQ_OP); break;
			}
			add_op_instr(GOTO_IF_TRUE_OP);
			add_line_ref($6.i);
		}
	| IF str_expr eq_rel str_expr THEN INT
		{
			switch ($3) {
			case '=': add_op_instr(EQ_STR_OP); break;
			case NOT_EQ: add_op_instr(NOT_EQ_STR_OP); break;
			}
			add_op_instr(GOTO_IF_TRUE_OP);
			add_line_ref($6.i);
		}
	;
	
str_expr:
	STR
		{
			add_op_instr(PUSH_STR_OP);
			add_id_instr(str_decl($1.start, $1.len));	
		}
	| QUOTED_STR
		{
			add_op_instr(PUSH_STR_OP);
			add_id_instr(str_decl($1.start, $1.len));	
		}
	| STRVAR
		{
			strvar_decl($1);
			add_check_init_var_code($1);
			add_op_instr(GET_STRVAR_OP);
			add_id_instr(get_rampos($1));
		}
	;

rel:
	eq_rel		{ $$ = $1; }
	| '<'		{ $$ = '<'; }
	| '>'		{ $$ = '>'; }
	| LESS_EQ	{ $$ = LESS_EQ; }
	| GREATER_EQ	{ $$ = GREATER_EQ; }
	;
	 	
eq_rel:
	'='		{ $$ = '='; }
	| NOT_EQ	{ $$ = NOT_EQ; }
	;
	
for_stmnt:
	FOR NUMVAR '=' expr TO expr step	{ for_decl($2); }
	;
	
step:
	/* empty */		{
					add_op_instr(PUSH_NUM_OP);
					add_num_instr(1.0);
				}
	| STEP expr
	;
	
next_stmnt:
	NEXT NUMVAR		{ next_decl($2); }
	;
	
def_stmnt:
	DEF USRFN fnparam '=' 
		{
			add_op_instr(GOTO_OP);
			s_save_pc = get_code_size();
			add_id_instr(0);
			fun_decl($2, $3.nparams, $3.param, get_code_size());
		}
		expr
		{
			add_op_instr(RETURN_OP);
			set_id_instr(s_save_pc, get_code_size());
		}
	;
	
fnparam:
	/* empty */		{ $$.nparams = 0; $$.param = '$'; }
	| '(' NUMVAR ')'	{ $$.nparams = 1; $$.param = $2; }
	;
	
input_stmnt:
	INPUT
		{
			add_op_instr(INPUT_OP);
		}
	var_list
		{
			add_op_instr(INPUT_END_OP);
		}
	;
	
var_list:
	var_loc
	| var_list ',' var_loc
	;

var_loc:
	NUMVAR
		{
			add_op_instr(INPUT_NUM_OP);
			s_save_pc = get_code_size();
			add_id_instr(0);
		}
	var_loc_rest
		{
			if ($3 == VARTYPE_NUM) {
				numvar_declared($1, VARTYPE_NUM);
				add_set_init_var_code($1);
				add_op_instr(LET_VAR_OP);
				add_id_instr(get_rampos($1));
			} else if ($3 == VARTYPE_LIST) {
				numvar_declared($1, VARTYPE_LIST);
				add_set_init_var_code($1);
				add_op_instr(INPUT_LIST_OP);
				add_id_instr(get_rampos($1));
				add_id_instr(get_dim($1, 0));
			} else {
				numvar_declared($1, VARTYPE_TABLE);
				add_set_init_var_code($1);
				add_op_instr(INPUT_TABLE_OP);
				add_id_instr(get_rampos($1));
				add_id_instr(get_dim($1, 0));
				add_id_instr(get_dim($1, 1));
			}
			set_id_instr(s_save_pc, get_code_size());
		}
	| STRVAR
		{			
			add_op_instr(INPUT_STR_OP);
			s_save_pc = get_code_size();
			add_id_instr(0);
			strvar_decl($1);
			add_set_init_var_code($1);
			add_op_instr(LET_STRVAR_OP);
			add_id_instr(get_rampos($1));
			set_id_instr(s_save_pc, get_code_size());
		}
	;
	
var_loc_rest:
	/* empty */			{ $$ = VARTYPE_NUM; }
	| '(' expr ')'			{ $$ = VARTYPE_LIST; }
	| '(' expr ',' expr ')'		{ $$ = VARTYPE_TABLE; }
	;
	
read_stmnt:
	READ read_var_list
	;
	
read_var_list:
	read_var_loc
	| read_var_list ',' read_var_loc
	;

read_var_loc:
	STRVAR
		{
			strvar_decl($1);
			add_set_init_var_code($1);
			add_op_instr(READ_STRVAR_OP);
			add_id_instr(get_rampos($1));
		}
	| NUMVAR
		{
			numvar_declared($1, VARTYPE_NUM);
			add_set_init_var_code($1);
			add_op_instr(READ_VAR_OP);
			add_id_instr(get_rampos($1));
		}
	| NUMVAR '(' expr ')'
		{
			numvar_declared($1, VARTYPE_LIST);
			add_set_init_var_code($1);
			add_op_instr(READ_LIST_OP);
			add_id_instr(get_rampos($1));
			add_id_instr(get_dim($1, 0));
		}
	| NUMVAR '(' expr ',' expr ')'
		{
			numvar_declared($1, VARTYPE_TABLE);
			add_set_init_var_code($1);
			add_op_instr(READ_TABLE_OP);
			add_id_instr(get_rampos($1));
			add_id_instr(get_dim($1, 0));
			add_id_instr(get_dim($1, 1));
		}
	;
	
restore_stmnt:
	RESTORE		{ add_op_instr(RESTORE_OP); }
	;
	
data_stmnt:
	DATA dat_list
	;
	
dat_list:
	datum
	| dat_list ',' datum
	;
	
datum:
	STR
		{
			data_str_decl(str_decl($1.start, $1.len),
				DATA_DATUM_UNQUOTED_STR);
		}
	| QUOTED_STR
		{
			data_str_decl(str_decl($1.start, $1.len),
				DATA_DATUM_QUOTED_STR);
		}
	;
	
dim_stmnt:
	DIM dim_list
	;
	
dim_list:
	dim_decl
	| dim_list ',' dim_decl
	;
	
dim_decl:
	NUMVAR '(' INT ')'		
		{
			numvar_dimensioned($1, VARTYPE_LIST, $3.i, 0);
			add_set_init_var_code($1);
		}
	| NUMVAR '(' INT ',' INT ')'
		{
			numvar_dimensioned($1, VARTYPE_TABLE, $3.i, $5.i);
			add_set_init_var_code($1);
		}
	;
	
option_stmnt:
	OPTION BASE INT		{ option_decl($3.i); }
	;
	
randomize_stmnt:
	RANDOMIZE		{ add_op_instr(RANDOMIZE_OP); }
	;

print_stmnt:
	PRINT print_expr
	;

print_expr:
	/* empty */			{ add_op_instr(PRINT_NL_OP); }
	| print_item			{ add_op_instr(PRINT_NL_OP); }
	| print_list
	| print_list print_item		{ add_op_instr(PRINT_NL_OP); }
	;

print_item_sep:
	print_item print_sep
	| print_sep
	;

print_list:
	print_item_sep
	| print_list print_item_sep
	;

print_sep:
	','	{ add_op_instr(PRINT_COMMA_OP); }
	| ';'
	;
	
print_item:
	expr
		{
			add_op_instr(PRINT_NUM_OP);
		}
	| str_expr
		{
			add_op_instr(PRINT_STR_OP);
		}
	| TAB '(' expr ')'
		{
			add_op_instr(PRINT_TAB_OP);
		}
	;
	
let_stmnt:
	LET STRVAR '=' str_expr
		{
			strvar_decl($2);
			add_set_init_var_code($2);
			add_op_instr(LET_STRVAR_OP);
			add_id_instr(get_rampos($2));
		}
	| LET NUMVAR '=' expr
		{
			numvar_declared($2, VARTYPE_NUM);
			add_set_init_var_code($2);
			add_op_instr(LET_VAR_OP);
			add_id_instr(get_rampos($2));
		}
	| LET NUMVAR '(' expr ')' '=' expr
		{
			numvar_declared($2, VARTYPE_LIST);
			add_set_init_var_code($2);
			add_op_instr(LET_LIST_OP);
			add_id_instr(get_rampos($2));
			add_id_instr(get_dim($2, 0));
		}
	| LET NUMVAR '(' expr ',' expr ')' '=' expr
		{
			numvar_declared($2, VARTYPE_TABLE);
			add_set_init_var_code($2);
			add_op_instr(LET_TABLE_OP);
			add_id_instr(get_rampos($2));
			add_id_instr(get_dim($2, 0));
			add_id_instr(get_dim($2, 1));
		}
	;
	
expr:
	INT						
		{
			add_op_instr(PUSH_NUM_OP);
			add_num_instr($1.d);
		}
	| NUM
		{
			add_op_instr(PUSH_NUM_OP);
			add_num_instr($1.d);
		}
	| NUMVAR
		{
			numvar_expr($1);
		}
	| NUMVAR '(' expr ')'
		{
			list_expr($1);
		}
	| NUMVAR '(' expr ',' expr ')'
		{
			table_expr($1);
		}
	| USRFN			{ usrfun_call($1, 0); }
	| USRFN '(' expr ')'	{ usrfun_call($1, 1); }
	| IFUN			{ ifun_call($1, 0); }
	| IFUN '(' expr ')'	{ ifun_call($1, 1); }
	| expr '+' expr		{ add_op_instr(ADD_OP);	}
	| expr '-' expr		{ add_op_instr(SUB_OP);	}
	| expr '*' expr		{ add_op_instr(MUL_OP);	}
	| expr '/' expr		{ add_op_instr(DIV_OP);	}
	| '-' expr %prec NEG	{ add_op_instr(NEG_OP);	}
	| '+' expr %prec NEG
	| expr '^' expr		{ add_op_instr(POW_OP);	}
	| '(' expr ')'
	;

%%

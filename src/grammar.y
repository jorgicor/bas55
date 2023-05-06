%{
/* --------------------------------------------------------------------------
 * Copyright (C) 2023 Jorge Giner Cordero
 * This file is part of bas55 (ECMA-55 Minimal BASIC System).
 * License: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>
 * --------------------------------------------------------------------------
 */

/* Minimal BASIC grammar. */

#include <config.h>
#include "ecma55.h"
#include <stddef.h>

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

%token NUM
%token INT
%token NUMVAR
%token STRVAR
%token STR
%token QUOTED_STR
%token USRFN
%token IFUN

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
			add_line_ref($2.column, $2.u.num.i);
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
			add_line_ref($2.column, $2.u.num.i);
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
			check_type($2, PSTACK_NUM);
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
			add_line_ref($1.column, $1.u.num.i);
		}
	;
	
if_stmnt:
	IF expr rel expr THEN INT
		{
			boolean_expr($2, $3, $4);
			add_op_instr(GOTO_IF_TRUE_OP);
			add_line_ref($6.column, $6.u.num.i);
		}
	;
	
str_expr:
	STR
		{
			add_op_instr(PUSH_STR_OP);
			add_id_instr(str_decl($1.u.str.start, $1.u.str.len));	
			$$.type = PSTACK_STR;
		}
	| QUOTED_STR
		{
			add_op_instr(PUSH_STR_OP);
			add_id_instr(str_decl($1.u.str.start, $1.u.str.len));	
			$$.type = PSTACK_STR;
		}
	| STRVAR
		{
			strvar_decl($1.u.i);
			add_op_instr(GET_STRVAR_OP);
			add_id_instr(get_rampos($1.u.i));
			$$.type = PSTACK_STR;
		}
	;

rel:
	eq_rel		{ $$.u.i = $1.u.i; }
	| '<'		{ $$.u.i = '<'; }
	| '>'		{ $$.u.i = '>'; }
	| LESS_EQ	{ $$.u.i = LESS_EQ; }
	| GREATER_EQ	{ $$.u.i = GREATER_EQ; }
	;
	 	
eq_rel:
	'='		{ $$.u.i = '='; }
	| NOT_EQ	{ $$.u.i = NOT_EQ; }
	;
	
for_stmnt:
	FOR NUMVAR '=' expr TO expr step	
		{
			check_type($4, PSTACK_NUM);
			check_type($6, PSTACK_NUM);
			for_decl($2.column, $2.u.i);
		}
	;
	
step:
	/* empty */		
		{
			add_op_instr(PUSH_NUM_OP);
			add_num_instr(1.0);
		}
	| STEP expr
		{
			check_type($2, PSTACK_NUM);
		}
					
	;
	
next_stmnt:
	NEXT NUMVAR		{ next_decl($2.column, $2.u.i); }
	;

def_stmnt_head:
	DEF USRFN fnparam
		{
			add_op_instr(GOTO_OP);
			s_save_pc = get_code_size();
			add_id_instr(0);
			fun_decl($2.column, $2.u.i, $3.u.fun_param.nparams,
				 $3.u.fun_param.param, get_code_size());
		}
	;
	
def_stmnt:
	def_stmnt_head '=' expr
		{
			check_type($3, PSTACK_NUM);
			add_op_instr(RETURN_OP);
			set_id_instr(s_save_pc, get_code_size());
		}
	;
	
fnparam:
	/* empty */
		{
			$$.u.fun_param.nparams = 0;
			$$.u.fun_param.param = '$';
		}
	| '(' NUMVAR ')'
		{
			$$.u.fun_param.nparams = 1;
			$$.u.fun_param.param = $2.u.i;
		}
	| '(' STRVAR ')'
		{
			$$.u.fun_param.nparams = 1;
			$$.u.fun_param.param = encode_var2(
				get_var_letter($2.u.i), '\0');
			cerror(E_NUMVAR_EXPECT, 1);
			print_lex_context($2.column);	
		}
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
			if ($3.u.i == VARTYPE_NUM) {
				numvar_declared($1.column, $1.u.i, VARTYPE_NUM);
				add_op_instr(LET_VAR_OP);
				add_id_instr(get_rampos($1.u.i));
			} else if ($3.u.i == VARTYPE_LIST) {
				numvar_declared($1.column, $1.u.i,
						VARTYPE_LIST);
				add_op_instr(INPUT_LIST_OP);
				add_id_instr(var_index1($1.u.i));
			} else {
				numvar_declared($1.column, $1.u.i,
						VARTYPE_TABLE);
				add_op_instr(INPUT_TABLE_OP);
				add_id_instr(var_index1($1.u.i));
			}
			set_id_instr(s_save_pc, get_code_size());
		}
	| STRVAR
		{			
			add_op_instr(INPUT_STR_OP);
			s_save_pc = get_code_size();
			add_id_instr(0);
			strvar_decl($1.u.i);
			add_op_instr(LET_STRVAR_OP);
			add_id_instr(get_rampos($1.u.i));
			set_id_instr(s_save_pc, get_code_size());
		}
	;
	
var_loc_rest:
	/* empty */
		{ 
			$$.u.i = VARTYPE_NUM;
		}
	| '(' expr ')'
		{
			check_type($2, PSTACK_NUM);
			$$.u.i = VARTYPE_LIST;
		}
	| '(' expr ',' expr ')'
		{
			check_type($2, PSTACK_NUM);
			check_type($4, PSTACK_NUM);
			$$.u.i = VARTYPE_TABLE;
		}
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
			strvar_decl($1.u.i);
			add_op_instr(READ_STRVAR_OP);
			add_id_instr(get_rampos($1.u.i));
		}
	| NUMVAR
		{
			numvar_declared($1.column, $1.u.i, VARTYPE_NUM);
			add_op_instr(READ_VAR_OP);
			add_id_instr(get_rampos($1.u.i));
		}
	| NUMVAR '(' expr ')'
		{
			check_type($3, PSTACK_NUM);	
			numvar_declared($1.column, $1.u.i, VARTYPE_LIST);
			add_op_instr(READ_LIST_OP);
			add_id_instr(var_index1($1.u.i));
		}
	| NUMVAR '(' expr ',' expr ')'
		{
			check_type($3, PSTACK_NUM);	
			check_type($5, PSTACK_NUM);	
			numvar_declared($1.column, $1.u.i, VARTYPE_TABLE);
			add_op_instr(READ_TABLE_OP);
			add_id_instr(var_index1($1.u.i));
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
			data_str_decl(str_decl($1.u.str.start, $1.u.str.len),
				DATA_DATUM_UNQUOTED_STR);
		}
	| QUOTED_STR
		{
			data_str_decl(str_decl($1.u.str.start, $1.u.str.len),
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
			numvar_dimensioned($1.column, $3.column, 0, $1.u.i,
					   VARTYPE_LIST, $3.u.num.i, 0);
		}
	| NUMVAR '(' INT ',' INT ')'
		{
			numvar_dimensioned($1.column, $3.column, $5.column,
					   $1.u.i, VARTYPE_TABLE, $3.u.num.i,
					   $5.u.num.i);
		}
	;
	
option_stmnt:
	OPTION BASE INT
		{
			 option_decl($1.column, $3.column, $3.u.num.i);
		}
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
			if ($1.type == PSTACK_NUM) {
				add_op_instr(PRINT_NUM_OP);
			} else {
				add_op_instr(PRINT_STR_OP);
			}
		}
	| TAB '(' expr ')'
		{
			check_type($3, PSTACK_NUM);
			add_op_instr(PRINT_TAB_OP);
		}
	;
	
let_stmnt:
	LET STRVAR '=' expr
		{
			check_type($4, PSTACK_STR);
			strvar_decl($2.u.i);
			add_op_instr(LET_STRVAR_OP);
			add_id_instr(get_rampos($2.u.i));
		}
	| LET NUMVAR '=' expr
		{
			check_type($4, PSTACK_NUM);
			numvar_declared($2.column, $2.u.i, VARTYPE_NUM);
			add_op_instr(LET_VAR_OP);
			add_id_instr(get_rampos($2.u.i));
		}
	| LET NUMVAR '(' expr ')' '=' expr
		{
			check_type($4, PSTACK_NUM);
			check_type($7, PSTACK_NUM);
			numvar_declared($2.column, $2.u.i, VARTYPE_LIST);
			add_op_instr(LET_LIST_OP);
			add_id_instr(var_index1($2.u.i));
		}
	| LET NUMVAR '(' expr ',' expr ')' '=' expr
		{
			check_type($4, PSTACK_NUM);
			check_type($6, PSTACK_NUM);
			check_type($9, PSTACK_NUM);
			numvar_declared($2.column, $2.u.i, VARTYPE_TABLE);
			add_op_instr(LET_TABLE_OP);
			add_id_instr(var_index1($2.u.i));
		}
	;
	
expr:
	str_expr
		{
			$$.type = $1.type;
		}
	| INT						
		{
			add_op_instr(PUSH_NUM_OP);
			add_num_instr($1.u.num.d);
			$$.type = PSTACK_NUM;
		}
	| NUM
		{
			add_op_instr(PUSH_NUM_OP);
			add_num_instr($1.u.num.d);
			$$.type = PSTACK_NUM;
		}
	| NUMVAR
		{
			numvar_expr($1.column, $1.u.i);
			$$.type = PSTACK_NUM;
		}
	| NUMVAR '(' expr ')'
		{
			check_type($3, PSTACK_NUM);
			list_expr($1.column, $1.u.i);
			$$.type = PSTACK_NUM;
		}
	| NUMVAR '(' expr ',' expr ')'
		{
			check_type($3, PSTACK_NUM);
			check_type($5, PSTACK_NUM);
			table_expr($1.column, $1.u.i);
			$$.type = PSTACK_NUM;
		}
	| USRFN
		{
			usrfun_call($1.column, $1.u.i, 0);
			$$.type = PSTACK_NUM;
		}
	| USRFN '(' expr ')'
		{
			check_type($3, PSTACK_NUM);
			usrfun_call($1.column, $1.u.i, 1);
			$$.type = PSTACK_NUM;
		}
	| IFUN
		{
			ifun_call($1.column, $1.u.i, 0);
			$$.type = PSTACK_NUM;
		}
	| IFUN '(' expr ')'
		{
			check_type($3, PSTACK_NUM);
			ifun_call($1.column, $1.u.i, 1);
			$$.type = PSTACK_NUM;
		}
	| expr '+' expr		{ $$.type = binary_expr($1, $3, ADD_OP); }
	| expr '-' expr		{ $$.type = binary_expr($1, $3, SUB_OP); }
	| expr '*' expr		{ $$.type = binary_expr($1, $3, MUL_OP); }
	| expr '/' expr		{ $$.type = binary_expr($1, $3, DIV_OP); }
	| '-' expr %prec NEG
		{
			check_type($2, PSTACK_NUM);
			add_op_instr(NEG_OP);
			$$.type = PSTACK_NUM;
		}
	| '+' expr %prec NEG	{ $$.type = $2.type; }
	| expr '^' expr		{ $$.type = binary_expr($1, $3, POW_OP); }
	| '(' expr ')'		{ $$.type = $2.type; }
	;

%%

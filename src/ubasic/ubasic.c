/*
 * Copyright (c) 2006, Adam Dunkels
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */


//#define DEBUG_BASIC
#ifdef DEBUG_BASIC
#define DEBUG_PRINTF(...)  printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif

#include "ubasic.h"
#include "tokenizer.h"
#include "../sine.h"
#include "../ws2812.h"

//#include <printf.h>
#include "../printf.h" /* printf() */
//#include "../malloc.h"
#define NULL ((void*)0)

extern volatile unsigned int time;

#define MAX_STRINGLEN 40
static char string[MAX_STRINGLEN];

#define MAX_GOSUB_STACK_DEPTH 10
static int gosub_stack[MAX_GOSUB_STACK_DEPTH];
static int gosub_stack_ptr;

static int current_line = 0;

struct for_state {
  int line_after_for;
  int for_variable;
  int to;
};

#define MAX_FOR_STACK_DEPTH 4
static struct for_state for_stack[MAX_FOR_STACK_DEPTH];
static int for_stack_ptr;

#define MAX_VARNUM 26
static VARIABLE_TYPE variables[MAX_VARNUM];

static int ended;

void exit(int i)
{
	ended = i;
}

unsigned int random()
{
	static int x = 0;
	x = 1103515245 * x + 12345;
  if(x < 0)
    x = -x;
	return x;
}

static VARIABLE_TYPE expr(void);
static void line_statement(void);
static void statement(void);


peek_func peek_function = NULL;
poke_func poke_function = NULL;

/*---------------------------------------------------------------------------*/
void
ubasic_init(const char *program)
{
  compile(program);
  for_stack_ptr = gosub_stack_ptr = 0;
  ended = 0;
}
/*---------------------------------------------------------------------------*/
void
ubasic_init_peek_poke(const char *program, peek_func peek, poke_func poke)
{
  compile(program);
  for_stack_ptr = gosub_stack_ptr = 0;
  peek_function = peek;
  poke_function = poke;
  ended = 0;
}
/*---------------------------------------------------------------------------*/
static void
accept(int token)
{
  if(token != tokenizer_token()) {
    printf("Token on line %i not what was expected (expected %s, got %s)\n",
    		current_line, tok_to_str(token), tok_to_str(tokenizer_token()));
    tokenizer_error_print();
    exit(-1);
    return;
  }
  DEBUG_PRINTF("Expected %s, got it\n", tok_to_str(token));
  tokenizer_next();
}
/*---------------------------------------------------------------------------*/
static int
varfactor(void)
{
  int r;
  DEBUG_PRINTF("varfactor: obtaining %d from variable %d\n", variables[tokenizer_variable_num()], tokenizer_variable_num());
  r = ubasic_get_variable(tokenizer_variable_num());
  accept(TOKENIZER_VARIABLE);
  return r;
}
/*---------------------------------------------------------------------------*/
static int
sin_statement(void)
{
  int r;

  accept(TOKENIZER_LEFTPAREN);
  r = expr();
  accept(TOKENIZER_RIGHTPAREN);
  DEBUG_PRINTF("sin_statement: sin( %d ) == %d\n", r, sin(r));

  return sin(r);
}
/*---------------------------------------------------------------------------*/
static int
factor(void)
{
  int r;

  DEBUG_PRINTF("factor: %s\n", tok_to_str(tokenizer_token()));
  switch(tokenizer_token()) {
  case TOKENIZER_NUMBER:
    r = tokenizer_num();
    DEBUG_PRINTF("factor: number %d\n", r);
    accept(TOKENIZER_NUMBER);
    break;
  case TOKENIZER_LEFTPAREN:
    accept(TOKENIZER_LEFTPAREN);
    r = expr();
    accept(TOKENIZER_RIGHTPAREN);
    break;
  case TOKENIZER_SINE:
	accept(TOKENIZER_SINE);
	r = sin_statement();
	break;
  case TOKENIZER_RANDOM:
	accept(TOKENIZER_RANDOM);
	r = random();
	break;
  default:
    r = varfactor();
    break;
  }
  return r;
}
/*---------------------------------------------------------------------------*/
static int
term(void)
{
  int f1, f2;
  int op;

  f1 = factor();
  op = tokenizer_token();
  DEBUG_PRINTF("term: %s\n", tok_to_str(op));
  while(op == TOKENIZER_ASTR ||
       op == TOKENIZER_SLASH ||
       op == TOKENIZER_MOD) {
    tokenizer_next();
    f2 = factor();
    DEBUG_PRINTF("term: %d %d %d\n", f1, op, f2);
    switch(op) {
    case TOKENIZER_ASTR:
      f1 = f1 * f2;
      break;
    case TOKENIZER_SLASH:
      f1 = f1 / f2;
      break;
    case TOKENIZER_MOD:
      f1 = f1 % f2;
      break;
    }
    op = tokenizer_token();
  }
  DEBUG_PRINTF("term: %d\n", f1);
  return f1;
}
/*---------------------------------------------------------------------------*/
static VARIABLE_TYPE
expr(void)
{
  int t1, t2;
  int op;

  t1 = term();
  op = tokenizer_token();
  DEBUG_PRINTF("expr: %s\n", tok_to_str(op));
  while(op == TOKENIZER_PLUS ||
       op == TOKENIZER_MINUS ||
       op == TOKENIZER_AND ||
       op == TOKENIZER_OR) {
    tokenizer_next();
    t2 = term();
    DEBUG_PRINTF("expr: %d %d %d\n", t1, op, t2);
    switch(op) {
    case TOKENIZER_PLUS:
      t1 = t1 + t2;
      break;
    case TOKENIZER_MINUS:
      t1 = t1 - t2;
      break;
    case TOKENIZER_AND:
      t1 = t1 & t2;
      break;
    case TOKENIZER_OR:
      t1 = t1 | t2;
      break;
    }
    op = tokenizer_token();
  }
  DEBUG_PRINTF("expr: %d\n", t1);
  return t1;
}
/*---------------------------------------------------------------------------*/
static int
relation(void)
{
  int r1, r2;
  int op;

  r1 = expr();
  op = tokenizer_token();
  DEBUG_PRINTF("relation: token %d\n", op);
  while(op == TOKENIZER_LT ||
       op == TOKENIZER_GT ||
       op == TOKENIZER_EQ) {
    tokenizer_next();
    r2 = expr();
    DEBUG_PRINTF("relation: %d %d %d\n", r1, op, r2);
    switch(op) {
    case TOKENIZER_LT:
      r1 = r1 < r2;
      break;
    case TOKENIZER_GT:
      r1 = r1 > r2;
      break;
    case TOKENIZER_EQ:
      r1 = r1 == r2;
      break;
    }
    op = tokenizer_token();
  }
  return r1;
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
static void
jump_linenum(int linenum)
{
  tokenizer_goto(linenum);
  DEBUG_PRINTF("Jumped to: %d\n", tokenizer_num());
}
/*---------------------------------------------------------------------------*/
static void
goto_statement(void)
{
  accept(TOKENIZER_GOTO);
  jump_linenum(tokenizer_num());
}
/*---------------------------------------------------------------------------*/
static void
print_statement(void)
{
  accept(TOKENIZER_PRINT);
  do {
    DEBUG_PRINTF("Print loop\n");
    if(tokenizer_token() == TOKENIZER_STRING) {
      tokenizer_string(string, sizeof(string));
      printf("%s", string);
      tokenizer_next();
    } else if(tokenizer_token() == TOKENIZER_COMMA) {
      printf(" ");
      tokenizer_next();
    } else if(tokenizer_token() == TOKENIZER_SEMICOLON) {
      tokenizer_next();
    } else if(tokenizer_token() == TOKENIZER_VARIABLE ||
          tokenizer_token() == TOKENIZER_NUMBER) {
      printf("%d", expr());
    } else {
      break;
    }
  } while(tokenizer_token() != TOKENIZER_CR &&
      tokenizer_token() != TOKENIZER_ENDOFINPUT);
  printf("\n");
  DEBUG_PRINTF("End of print\n");
  tokenizer_next();
}
/*---------------------------------------------------------------------------*/
static void
if_statement(void)
{
  int r;

  accept(TOKENIZER_IF);

  r = relation();
  DEBUG_PRINTF("if_statement: relation %d\n", r);
  accept(TOKENIZER_THEN);
  if(r) {
    statement();
  } else {
    do {
      tokenizer_next();
    } while(tokenizer_token() != TOKENIZER_ELSE &&
        tokenizer_token() != TOKENIZER_CR &&
        tokenizer_token() != TOKENIZER_ENDOFINPUT);
    if(tokenizer_token() == TOKENIZER_ELSE) {
      tokenizer_next();
      statement();
    } else if(tokenizer_token() == TOKENIZER_CR) {
      tokenizer_next();
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
let_statement(void)
{
  int var;

  var = tokenizer_variable_num();

  accept(TOKENIZER_VARIABLE);
  accept(TOKENIZER_EQ);
  ubasic_set_variable(var, expr());
  DEBUG_PRINTF("let_statement: assign %d to %d\n", variables[var], var);
  accept(TOKENIZER_CR);

}
/*---------------------------------------------------------------------------*/
static void
gosub_statement(void)
{
  int linenum;
  accept(TOKENIZER_GOSUB);
  linenum = tokenizer_num();
  accept(TOKENIZER_NUMBER);
  accept(TOKENIZER_CR);
  if(gosub_stack_ptr < MAX_GOSUB_STACK_DEPTH) {
    gosub_stack[gosub_stack_ptr] = tokenizer_num();
    gosub_stack_ptr++;
    jump_linenum(linenum);
  } else {
    DEBUG_PRINTF("gosub_statement: gosub stack exhausted\n");
  }
}
/*---------------------------------------------------------------------------*/
static void
return_statement(void)
{
  accept(TOKENIZER_RETURN);
  if(gosub_stack_ptr > 0) {
    gosub_stack_ptr--;
    jump_linenum(gosub_stack[gosub_stack_ptr]);
  } else {
    DEBUG_PRINTF("return_statement: non-matching return\n");
  }
}
/*---------------------------------------------------------------------------*/
static void
next_statement(void)
{
  int var;

  accept(TOKENIZER_NEXT);
  var = tokenizer_variable_num();
  accept(TOKENIZER_VARIABLE);
  if(for_stack_ptr > 0 &&
     var == for_stack[for_stack_ptr - 1].for_variable) {
    ubasic_set_variable(var,
                       ubasic_get_variable(var) + 1);
    if(ubasic_get_variable(var) <= for_stack[for_stack_ptr - 1].to) {
      jump_linenum(for_stack[for_stack_ptr - 1].line_after_for);
    } else {
      for_stack_ptr--;
      accept(TOKENIZER_CR);
    }
  } else {
    DEBUG_PRINTF("next_statement: non-matching next (expected %d, found %d)\n", for_stack[for_stack_ptr - 1].for_variable, var);
    accept(TOKENIZER_CR);
  }

}
/*---------------------------------------------------------------------------*/
static void
for_statement(void)
{
  int for_variable, to;

  accept(TOKENIZER_FOR);
  for_variable = tokenizer_variable_num();
  accept(TOKENIZER_VARIABLE);
  accept(TOKENIZER_EQ);
  ubasic_set_variable(for_variable, expr());
  accept(TOKENIZER_TO);
  to = expr();
  accept(TOKENIZER_CR);

  if(for_stack_ptr < MAX_FOR_STACK_DEPTH) {
    for_stack[for_stack_ptr].line_after_for = tokenizer_num();
    for_stack[for_stack_ptr].for_variable = for_variable;
    for_stack[for_stack_ptr].to = to;
    DEBUG_PRINTF("for_statement: new for, var %d to %d\n",
                for_stack[for_stack_ptr].for_variable,
                for_stack[for_stack_ptr].to);

    for_stack_ptr++;
  } else {
    DEBUG_PRINTF("for_statement: for stack depth exceeded\n");
  }
}
/*---------------------------------------------------------------------------*/
static void
peek_statement(void)
{
  VARIABLE_TYPE peek_addr;
  int var;

  accept(TOKENIZER_PEEK);
  peek_addr = expr();
  accept(TOKENIZER_COMMA);
  var = tokenizer_variable_num();
  accept(TOKENIZER_VARIABLE);
  accept(TOKENIZER_CR);

  ubasic_set_variable(var, peek_function(peek_addr));
}
/*---------------------------------------------------------------------------*/
static void
poke_statement(void)
{
  VARIABLE_TYPE poke_addr;
  VARIABLE_TYPE value;

  accept(TOKENIZER_POKE);
  poke_addr = expr();
  accept(TOKENIZER_COMMA);
  value = expr();
  accept(TOKENIZER_CR);

  poke_function(poke_addr, value);
}
/*---------------------------------------------------------------------------*/
static void
end_statement(void)
{
  accept(TOKENIZER_END);
  exit(1);
}
/*---------------------------------------------------------------------------*/
static void
clr_statement(void)
{
  accept(TOKENIZER_CLR);
  clear_data();
  accept(TOKENIZER_CR);
}
/*---------------------------------------------------------------------------*/
static void
update_statement(void)
{
  accept(TOKENIZER_UPDATE);
  send_strip_data(expr()*3);
  accept(TOKENIZER_CR);
}
/*---------------------------------------------------------------------------*/
static void
sleep_statement(void)
{
  unsigned int sleep_for;
  unsigned int start = time;
  accept(TOKENIZER_SLEEP);
  sleep_for = expr();
  while(time - start < sleep_for);

  accept(TOKENIZER_CR);
}
/*---------------------------------------------------------------------------*/
static void
statement(void)
{
  int token;

  token = tokenizer_token();

  switch(token) {
  case TOKENIZER_PRINT:
    print_statement();
    break;
  case TOKENIZER_IF:
    if_statement();
    break;
  case TOKENIZER_GOTO:
    goto_statement();
    break;
  case TOKENIZER_GOSUB:
    gosub_statement();
    break;
  case TOKENIZER_RETURN:
    return_statement();
    break;
  case TOKENIZER_FOR:
    for_statement();
    break;
  case TOKENIZER_PEEK:
    peek_statement();
    break;
  case TOKENIZER_POKE:
    poke_statement();
    break;
  case TOKENIZER_NEXT:
    next_statement();
    break;
  case TOKENIZER_END:
    end_statement();
    break;
  case TOKENIZER_LET:
    accept(TOKENIZER_LET);
    /* Fall through. */
  case TOKENIZER_VARIABLE:
    let_statement();
    break;
  case TOKENIZER_CLR:
    clr_statement();
    break;
  case TOKENIZER_UPDATE:
    update_statement();
    break;
  case TOKENIZER_SLEEP:
    sleep_statement();
    break;
  default:
    DEBUG_PRINTF("ubasic.c: statement(): not implemented %d\n", token);
    exit(-2);
  }
}
/*---------------------------------------------------------------------------*/
static void
line_statement(void)
{
  DEBUG_PRINTF("----------- Line number %d ---------\n", tokenizer_num());
  current_line = tokenizer_num();
  //index_add(tokenizer_num(), tokenizer_pos());
  accept(TOKENIZER_NUMBER);
  statement();
  return;
}
/*---------------------------------------------------------------------------*/
void
ubasic_run(void)
{
  if(tokenizer_finished() || ended) {
    DEBUG_PRINTF("uBASIC program finished\n");
    return;
  }

  line_statement();
}
/*---------------------------------------------------------------------------*/
int
ubasic_finished(void)
{
  if(tokenizer_finished())
    return -3;

  return ended;
}
/*---------------------------------------------------------------------------*/
void
ubasic_set_variable(int varnum, VARIABLE_TYPE value)
{
  if(varnum >= 0 && varnum <= MAX_VARNUM) {
    variables[varnum] = value;
  }
}
/*---------------------------------------------------------------------------*/
VARIABLE_TYPE
ubasic_get_variable(int varnum)
{
  if(varnum >= 0 && varnum <= MAX_VARNUM) {
    return variables[varnum];
  }
  return 0;
}
/*---------------------------------------------------------------------------*/

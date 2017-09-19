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



#ifdef DEBUG_BASIC
#define DEBUG_PRINTF(...)  printf(__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif

#include "tokenizer.h"
#include "../stdlib.h"


static char const *ptr, *nextptr;


struct token{
  int token;
  int data;
};

struct line {
  int line_num;
  int token_pos;
};

struct line jump_table[30] = {0};
struct token tokens[300];
int token_pos;

#define MAX_NUMLEN 6

struct keyword_token {
  char *keyword;
  int token;
};

static int current_token = TOKENIZER_ERROR;

static const struct keyword_token keywords[] = {
  {"let", TOKENIZER_LET},
  {"print", TOKENIZER_PRINT},
  {"if", TOKENIZER_IF},
  {"then", TOKENIZER_THEN},
  {"else", TOKENIZER_ELSE},
  {"for", TOKENIZER_FOR},
  {"to", TOKENIZER_TO},
  {"next", TOKENIZER_NEXT},
  {"goto", TOKENIZER_GOTO},
  {"gosub", TOKENIZER_GOSUB},
  {"return", TOKENIZER_RETURN},
  {"call", TOKENIZER_CALL},
  {"rem", TOKENIZER_REM},
  {"peek", TOKENIZER_PEEK},
  {"poke", TOKENIZER_POKE},
  {"end", TOKENIZER_END},
  {"sin", TOKENIZER_SINE},
  {"update", TOKENIZER_UPDATE},
  {"clr", TOKENIZER_CLR},
  {"sleep", TOKENIZER_SLEEP},
  {"rnd", TOKENIZER_RANDOM},
  {NULL, TOKENIZER_ERROR}
};

static const char* _tok_to_str[] =
{
  "ERROR",
  "ENDOFINPUT",
  "NUMBER",
  "STRING",
  "VARIABLE",
  "LET",
  "PRINT",
  "IF",
  "THEN",
  "ELSE",
  "FOR",
  "TO",
  "NEXT",
  "GOTO",
  "GOSUB",
  "RETURN",
  "CALL",
  "REM",
  "PEEK",
  "POKE",
  "END",
  "COMMA",
  "SEMICOLON",
  "PLUS",
  "MINUS",
  "AND",
  "OR",
  "ASTR",
  "SLASH",
  "MOD",
  "HASH",
  "LEFTPAREN",
  "RIGHTPAREN",
  "LT",
  "GT",
  "EQ",
  "CR",
  "SIN",
  "UPDATE",
  "CLR",
  "SLEEP",
  "RND",
};

const char* tok_to_str(int tok)
{
  return _tok_to_str[tok];
}

/*---------------------------------------------------------------------------*/
static int
singlechar(void)
{
  if(*ptr == '\n') {
    return TOKENIZER_CR;
  } else if(*ptr == ',') {
    return TOKENIZER_COMMA;
  } else if(*ptr == ';') {
    return TOKENIZER_SEMICOLON;
  } else if(*ptr == '+') {
    return TOKENIZER_PLUS;
  } else if(*ptr == '-') {
    return TOKENIZER_MINUS;
  } else if(*ptr == '&') {
    return TOKENIZER_AND;
  } else if(*ptr == '|') {
    return TOKENIZER_OR;
  } else if(*ptr == '*') {
    return TOKENIZER_ASTR;
  } else if(*ptr == '/') {
    return TOKENIZER_SLASH;
  } else if(*ptr == '%') {
    return TOKENIZER_MOD;
  } else if(*ptr == '(') {
    return TOKENIZER_LEFTPAREN;
  } else if(*ptr == '#') {
    return TOKENIZER_HASH;
  } else if(*ptr == ')') {
    return TOKENIZER_RIGHTPAREN;
  } else if(*ptr == '<') {
    return TOKENIZER_LT;
  } else if(*ptr == '>') {
    return TOKENIZER_GT;
  } else if(*ptr == '=') {
    return TOKENIZER_EQ;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
get_next_token(void)
{
  struct keyword_token const *kt;
  int i;

  DEBUG_PRINTF("get_next_token(): '%s'\n", ptr);

  if(*ptr == 0) {
    return TOKENIZER_ENDOFINPUT;
  }

  if(isdigit(*ptr)) {
    for(i = 0; i < MAX_NUMLEN; ++i) {
      if(!isdigit(ptr[i])) {
        if(i > 0) {
          nextptr = ptr + i;
          return TOKENIZER_NUMBER;
        } else {
          DEBUG_PRINTF("get_next_token: error due to too short number\n");
          return TOKENIZER_ERROR;
        }
      }
      if(!isdigit(ptr[i])) {
        DEBUG_PRINTF("get_next_token: error due to malformed number\n");
        return TOKENIZER_ERROR;
      }
    }
    DEBUG_PRINTF("get_next_token: error due to too long number\n");
    return TOKENIZER_ERROR;
  } else if(singlechar()) {
    nextptr = ptr + 1;
    return singlechar();
  } else if(*ptr == '"') {
    nextptr = ptr;
    do {
      ++nextptr;
    } while(*nextptr != '"');
    ++nextptr;
    return TOKENIZER_STRING;
  } else {
    for(kt = keywords; kt->keyword != NULL; ++kt) {
      if(strncmp(ptr, kt->keyword, strlen(kt->keyword)) == 0) {
        nextptr = ptr + strlen(kt->keyword);
        return kt->token;
      }
    }
  }

  if(*ptr >= 'a' && *ptr <= 'z') {
    nextptr = ptr + 1;
    return TOKENIZER_VARIABLE;
  }


  return TOKENIZER_ERROR;
}
/*---------------------------------------------------------------------------*/
void
tokenizer_goto(int program)
{
  int i;
  for(i = 0; i < (sizeof(jump_table)/sizeof(jump_table[0])); i++) {
    if(jump_table[i].line_num == program) {
      token_pos = jump_table[i].token_pos; 
      return;
    }
    if(jump_table[i].line_num == 0)
      return;
  }
}
/*---------------------------------------------------------------------------*/
void
tokenizer_init(const char *program)
{
}
/*---------------------------------------------------------------------------*/
int
tokenizer_token(void)
{
  return tokens[token_pos].token;
}
/*---------------------------------------------------------------------------*/
void
_tokenizer_next(void)
{

  if(tokenizer_finished()) {
    return;
  }

  DEBUG_PRINTF("tokenizer_next: %p\n", nextptr);
  ptr = nextptr;

  while(*ptr == ' ') {
    ++ptr;
  }
  current_token = get_next_token();

  if(current_token == TOKENIZER_REM) {
      while(!(*nextptr == '\n' || *ptr == 0)) {
        ++nextptr;
      }
      if(*nextptr == '\n') {
        ++nextptr;
      }
      _tokenizer_next();
  }

  DEBUG_PRINTF("tokenizer_next: '%s' %d\n", ptr, current_token);
  return;
}

void
tokenizer_next(void)
{

  if(tokenizer_finished()) {
    return;
  }
  DEBUG_PRINTF("tokenizer_next: %p\n", nextptr);
 

  current_token = tokens[token_pos++].token;

  if(current_token == TOKENIZER_REM) {
      while(tokens[token_pos].token != TOKENIZER_CR || !tokenizer_finished()) {
        token_pos++;
      }
  }

  DEBUG_PRINTF("tokenizer_next: '%s' %d\n", ptr, current_token);
  return;
}
/*---------------------------------------------------------------------------*/
VARIABLE_TYPE
tokenizer_num(void)
{
  return tokens[token_pos].data;
}
/*---------------------------------------------------------------------------*/
void
tokenizer_string(char *dest, int len)
{
  dest = "NO STRING";
/*  char *string_end;
  int string_len;

  if(tokenizer_token() != TOKENIZER_STRING) {
    return;
  }
  string_end = strchr(ptr + 1, '"');
  if(string_end == NULL) {
    return;
  }
  string_len = string_end - ptr - 1;
  if(len < string_len) {
    string_len = len;
  }
  memcpy(dest, ptr + 1, string_len);
  dest[string_len] = 0;
  */
}
/*---------------------------------------------------------------------------*/
void
tokenizer_error_print(void)
{
  DEBUG_PRINTF("tokenizer_error_print: '%s'\n", ptr);
}
/*---------------------------------------------------------------------------*/
int
tokenizer_finished(void)
{
  return tokens[token_pos].token == TOKENIZER_ENDOFINPUT;
}
/*---------------------------------------------------------------------------*/
int
tokenizer_variable_num(void)
{
  return tokens[token_pos].data;
}
/*---------------------------------------------------------------------------*/
int
tokenizer_pos(void)
{
    return token_pos;
}


void compile(const char *program)
{
  ptr = program;
  current_token = get_next_token();

  int i = 0;
  int table_entry = 0;
  int prev_token = TOKENIZER_CR;
  tokens[0].token = current_token; 
  while(current_token != TOKENIZER_ENDOFINPUT) {
    if(prev_token == TOKENIZER_CR && current_token == TOKENIZER_NUMBER) {
      tokens[i].data = atoi(ptr); 
      jump_table[table_entry].line_num = atoi(ptr);
      jump_table[table_entry].token_pos = i;
      table_entry++;
    } else if(current_token == TOKENIZER_NUMBER) {
      tokens[i].data = atoi(ptr); 
    } else if(current_token == TOKENIZER_STRING) {
      //(char*)tokens[i].data =tokenizer_string();
    } else if(current_token == TOKENIZER_VARIABLE) {
      tokens[i].data = *ptr - 'a'; 
    }

    prev_token = current_token;
    _tokenizer_next();
    tokens[++i].token = current_token; 
  }

  token_pos = 0;
}

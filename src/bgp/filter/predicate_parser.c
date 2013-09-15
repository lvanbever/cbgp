// ==================================================================
// @(#)predicate_parser.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 29/02/2004
// $Id: predicate_parser.c,v 1.2 2009-08-31 09:36:46 bqu Exp $
//
// C-BGP, BGP Routing Solver
// Copyright (C) 2002-2008 Bruno Quoitin
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
// 02111-1307  USA
// ==================================================================

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <libgds/cli.h>
#include <libgds/memory.h>
#include <libgds/stack.h>

#ifdef HAVE_READLINE_READLINE_H
#include <readline/readline.h>
#endif

#include <bgp/filter/filter.h>
#include <bgp/filter/registry.h>
#include <bgp/filter/predicate_parser.h>

#define MAX_DEPTH 100

typedef enum {
  TOKEN_NONE,
  TOKEN_EOF,
  TOKEN_ATOM,
  TOKEN_NOT,
  TOKEN_AND,
  TOKEN_OR,
  TOKEN_PAROPEN,
  TOKEN_PARCLOSE,
  TOKEN_MAX,
} parser_token_t;

//#define FSM_DEBUG 1

#ifdef FSM_DEBUG
static char * token_names[TOKEN_MAX]= {
  "none",
  "eof",
  "atom",
  "not",
  "and",
  "or",
  "par-open",
  "par-close",
};
#endif /* FSM_DEBUG */

typedef enum {
  PARSER_STATE_WAIT_UNARY_OP,
  PARSER_STATE_WAIT_BINARY_OP,
  PARSER_STATE_WAIT_ARG,
  PARSER_STATE_MAX
} parser_state_t;

#ifdef FSM_DEBUG
static char * state_names[PARSER_STATE_MAX]= {
  "unary-op",
  "binary-op",
  "argument",
};
#endif /* FSM_DEBUG */

typedef struct {
  int cmd;
  bgp_ft_matcher_t * left_predicate;
} _parser_stack_item_t;

static struct {
  parser_state_t     state;
  int                cmd;
  bgp_ft_matcher_t * left_predicate;
  bgp_ft_matcher_t * right_predicate;
  int                atom_error;
  gds_stack_t      * stack;
  unsigned int       column;
} _parser;

#ifdef FSM_DEBUG
static void _dump_stack()
{
  unsigned int index;
  _parser_stack_item_t * item;

  stream_printf(gdserr, "state [\n  %s\n", state_names[_parser.state]);
  stream_printf(gdserr, "  stack [\n");
  stream_printf(gdserr, "    0:%s (l:", token_names[_parser.cmd]);
  filter_matcher_dump(gdserr, _parser.left_predicate);
  stream_printf(gdserr, ", r:");
  filter_matcher_dump(gdserr, _parser.right_predicate);
  stream_printf(gdserr, ")\n");
  for (index= stack_depth(_parser.stack); index > 0 ; index--) {
    item= (_parser_stack_item_t *) _parser.stack->items[index-1];
    stream_printf(gdserr, "    %d:%s (l:",
		  (stack_depth(_parser.stack)-index)+1,
		  token_names[item->cmd]);
    filter_matcher_dump(gdserr, item->left_predicate);
    stream_printf(gdserr, ")\n");
  }
  stream_printf(gdserr, "  ]\n");
  stream_printf(gdserr, "]\n");
}
#endif /* FSM_DEBUG */

#ifdef FSM_DEBUG
static void _dump_op()
{
  stream_printf(gdserr, "  operation: and(");
  filter_matcher_dump(gdserr, _parser.left_predicate);
  stream_printf(gdserr, ",");
  filter_matcher_dump(gdserr, _parser.right_predicate);
  stream_printf(gdserr, ")\n");
}
#endif /* FSM_DEBUG */


// -----[ predicate_parser_perror ]--------------------------------
void predicate_parser_perror(gds_stream_t * stream, int error)
{
  char * msg= predicate_parser_strerror(error);
  if (msg != NULL)
    stream_printf(stream, msg);
  else
    stream_printf(stream, "unknown error (%i)", error);
}

// -----[ predicate_parser_strerror ]------------------------------
char * predicate_parser_strerror(int error)
{
#define PREDICATE_PARSER_ERROR_BUF_SIZE 1000
  static char buf[PREDICATE_PARSER_ERROR_BUF_SIZE];
  cli_error_t cli_error;

  switch (error) {
  case PREDICATE_PARSER_SUCCESS:
    return "success";
  case PREDICATE_PARSER_ERROR_UNEXPECTED:
    return "unexpected";
  case PREDICATE_PARSER_ERROR_UNFINISHED_STRING:
    return "unterminated string";
  case PREDICATE_PARSER_ERROR_ATOM:
    ft_registry_get_error(&cli_error);
    snprintf(buf, PREDICATE_PARSER_ERROR_BUF_SIZE,
	     "atom-parsing error (%s (%s))",
	     cli_strerror(_parser.atom_error),
	     cli_error.user_msg);
    return buf;
  case PREDICATE_PARSER_ERROR_UNEXPECTED_EOF:
    return "unexpected end-of-file";
  case PREDICATE_PARSER_ERROR_PAR_MISMATCH:
    return "parentheses mismatch";
  case PREDICATE_PARSER_ERROR_STATE_TOO_LARGE:
    return "state too large";
  case PREDICATE_PARSER_ERROR_UNARY_OP:
    return "unary operation not allowed here";
  }
  return NULL;
}

// -----[ _predicate_parser_get_atom ]-------------------------------
/**
 * Extract the next atom from the expression.
 *
 * Todo: keep quotes obtained in string-state ?
 *       
 */
static int _predicate_parser_get_atom(const char ** expr,
				      bgp_ft_matcher_t ** matcher)
{
  char * pos;
  char * sub_expr;
  const char * tmp_expr;
  int len;
  int error;


  // The following loop should be removed. It should be the role of
  // libgds's tokenizer to parse this. We need to add a set of exit
  // chars that would contain the predicate parser's special chars:
  // { ')', '&', '|', '!' }
  // --> Need to issue a feature-request for libgds.
  //
  // Expected benefits:
  // - faster (no need to parse string twice)
  // - more consistent (single place for code modifications/fixes)

  tmp_expr= *expr;
  while (*tmp_expr && ((pos= strchr(")&|!", *tmp_expr)) == NULL)) {

    // Mini state-machine (enter "string state")
    if (*tmp_expr == '\"') {
      tmp_expr++;
      // Eat anything while in "string state"...
      while (*tmp_expr && (*tmp_expr != '\"'))
	tmp_expr++;
      // End was reached before "string state" was left :-(
      if (!*tmp_expr)
	return PREDICATE_PARSER_ERROR_UNFINISHED_STRING;
    }

    tmp_expr++;
  }

  len= tmp_expr-(*expr);

  sub_expr= (char *) MALLOC(sizeof(char)*((len)+1));
  strncpy(sub_expr, *expr, len);
  sub_expr[len]= 0;
  tmp_expr= sub_expr;

  error= ft_registry_predicate_parser(tmp_expr, matcher);
  if (error != 0) {
    _parser.atom_error= error;
    error= PREDICATE_PARSER_ERROR_ATOM;
  }
  FREE(sub_expr);

  (*expr)+= len;

  if (error < 0)
    return error;
  else
    return len;
}

// -----[ _predicate_parser_get ]------------------------------------
static int _predicate_parser_get(const char ** expr)
{
  char cur_char;
  int error;

  while (**expr != 0) {
    cur_char= **expr;
    (*expr)++;
    _parser.column++;
    switch (cur_char) {
    case '!': return TOKEN_NOT;
    case '&': return TOKEN_AND;
    case '|': return TOKEN_OR;
    case '(': return TOKEN_PAROPEN;
    case ')': return TOKEN_PARCLOSE;
    case ' ':
    case '\t':
      break;
    default:
      (*expr)--;
      _parser.column--;
      error= _predicate_parser_get_atom(expr, &_parser.left_predicate);
      if (error >= 0) {
	_parser.column+= error;
	return TOKEN_ATOM;
      } else
	return error;
    }
  }
  return TOKEN_EOF;
}

// -----[ _predicate_parser_push_state ]-----------------------------
/**
 * Push a new state on the stack. This must only be done for
 * operations that will be processed later (because their right-value
 * is not known yet).
 *
 * Typical stackable operations are
 *   AND(l,?)
 *   OR(l,?)
 *   NOT(?)
 *   PAROPEN(?)
 */
static int _predicate_parser_push_state(int cmd)
{
  _parser_stack_item_t * item;

#ifdef FSM_DEBUG
  stream_printf(gdserr, "push-state(%s)\n", token_names[cmd]);
#endif /* FSM_DEBUG */

  // Push previous state
  item= (_parser_stack_item_t *) MALLOC(sizeof(_parser_stack_item_t));
  item->left_predicate= _parser.left_predicate;
  item->cmd= cmd;
  if (stack_push(_parser.stack, item) != 0) {
    FREE(item);
    return -1;
  }

  // Initialise current state
  _parser.left_predicate= NULL;
  _parser.right_predicate= NULL;
  _parser.cmd= TOKEN_NONE;
  _parser.state= PARSER_STATE_WAIT_UNARY_OP;
  return 0;
}

// -----[ _predicate_parser_pop_state ]------------------------------
static int _predicate_parser_pop_state()
{
  _parser_stack_item_t * item;

#ifdef FSM_DEBUG
  stream_printf(gdserr, "  pop-state(");
#endif /* FSM_DEBUG */

  _parser.cmd= TOKEN_NONE;
  assert(!stack_is_empty(_parser.stack));

  item= (_parser_stack_item_t *) stack_pop(_parser.stack);
  _parser.right_predicate= _parser.left_predicate;
  _parser.left_predicate= item->left_predicate;
  _parser.cmd= item->cmd;
  _parser.state= PARSER_STATE_WAIT_BINARY_OP;

#ifdef FSM_DEBUG
  stream_printf(gdserr, "l:");
  filter_matcher_dump(gdserr, _parser.left_predicate);
  stream_printf(gdserr, ",r:");
  filter_matcher_dump(gdserr, _parser.right_predicate);
  stream_printf(gdserr, ",command:%s)\n", token_names[_parser.cmd]);
#endif /* FSM_DEBUG */

  FREE(item);
  return 0;
}

// -----[ _predicate_parser_apply_block_ops ]------------------------
/**
 * Apply all the stacked operations until the end of the current
 * block. A block is limited by TOKEN_EOF or by TOKEN_PAROPEN.
 */
static int _predicate_parser_apply_block_ops()
{

#ifdef FSM_DEBUG
  stream_printf(gdserr, "apply-ops [\n");
#endif /* FSM_DEBUG */

  do {

#ifdef FSM_DEBUG
    _dump_op();
#endif /* FSM_DEBUG */

    switch (_parser.cmd) {
    case TOKEN_NONE:
      break;
    case TOKEN_AND:
      _parser.left_predicate= filter_match_and(_parser.left_predicate,
					       _parser.right_predicate);
      break;
    case TOKEN_NOT:
      _parser.left_predicate= filter_match_not(_parser.right_predicate);
      break;
    case TOKEN_OR:
      _parser.left_predicate= filter_match_or(_parser.left_predicate,
					      _parser.right_predicate);
      break;
    case TOKEN_PARCLOSE:
      if (stack_is_empty(_parser.stack))
	return PREDICATE_PARSER_ERROR_PAR_MISMATCH;
      _predicate_parser_pop_state();
      if (_parser.cmd != TOKEN_PAROPEN)
	return PREDICATE_PARSER_ERROR_PAR_MISMATCH;
      _parser.left_predicate= _parser.right_predicate;
      break;

    default:
      stream_printf(gdserr, "Error: unsupported operation %d\n",
		 _parser.cmd);
      abort();
    }

    _parser.cmd= TOKEN_NONE;
    _parser.right_predicate= NULL;

    if (stack_is_empty(_parser.stack))
      break;

    if (((_parser_stack_item_t *) stack_top(_parser.stack))->cmd
	== TOKEN_PAROPEN)
      break;

    if (_predicate_parser_pop_state() < 0)
      break;

  } while (1);

#ifdef FSM_DEBUG
  stream_printf(gdserr, "]\n");
#endif /* FSM_DEBUG */

  return PREDICATE_PARSER_SUCCESS;
}

// -----[ _predicate_parser_wait_unary_op ]--------------------------
/**
 * This state only allows ATOM, NOT and PAR-OPEN
 */
static int _predicate_parser_wait_unary_op(int token)
{
  switch (token) {
  case TOKEN_ATOM:
    _parser.cmd= TOKEN_NONE;
    _predicate_parser_apply_block_ops();
    _parser.state= PARSER_STATE_WAIT_BINARY_OP;
    break;
  case TOKEN_EOF:
    return PREDICATE_PARSER_ERROR_UNEXPECTED_EOF;
  case TOKEN_NOT:
    if (_predicate_parser_push_state(token) < 0)
      return PREDICATE_PARSER_ERROR_STATE_TOO_LARGE;
    _parser.state= PARSER_STATE_WAIT_ARG;
    break;
  case TOKEN_PARCLOSE:
    return PREDICATE_PARSER_ERROR_UNEXPECTED;
  case TOKEN_PAROPEN:
    if (_predicate_parser_push_state(token))
      return PREDICATE_PARSER_ERROR_STATE_TOO_LARGE;
    break;
  case TOKEN_AND:
  case TOKEN_OR:
    return PREDICATE_PARSER_ERROR_LEFT_EXPR;
  default:
    abort();
  }
  return PREDICATE_PARSER_SUCCESS;
}

// -----[ _predicate_parser_wait_binary_op ]-------------------------
/**
 * This states only allows AND, OR, EOF and PAR-CLOSE
 */
static int _predicate_parser_wait_binary_op(int token)
{
  switch (token) {
  case TOKEN_AND:
  case TOKEN_OR:
    if (_predicate_parser_push_state(token) < 0)
      return PREDICATE_PARSER_ERROR_UNEXPECTED;
    _parser.state= PARSER_STATE_WAIT_UNARY_OP;
    break;
  case TOKEN_ATOM:
    return PREDICATE_PARSER_ERROR_UNEXPECTED;
  case TOKEN_EOF:
    return PREDICATE_PARSER_END;
  case TOKEN_NOT:
    return PREDICATE_PARSER_ERROR_UNARY_OP;
  case TOKEN_PARCLOSE:
    _parser.cmd= TOKEN_PARCLOSE;
    return _predicate_parser_apply_block_ops();
  case TOKEN_PAROPEN:
    return PREDICATE_PARSER_ERROR_UNEXPECTED;
  default:
    abort();
  }
  return PREDICATE_PARSER_SUCCESS;
}

// -----[ _preficate_parser_wait_arg ]-------------------------------
/**
 * This states only allows ATOM and PAR-OPEN
 */
static int _predicate_parser_wait_arg(int token)
{
  switch (token) {
  case TOKEN_ATOM:
    _parser.cmd= TOKEN_NONE;
    _predicate_parser_apply_block_ops();
    _parser.state= PARSER_STATE_WAIT_BINARY_OP;
    break;
  case TOKEN_AND:
  case TOKEN_OR:
    return PREDICATE_PARSER_ERROR_UNEXPECTED;
  case TOKEN_NOT:
    return PREDICATE_PARSER_ERROR_UNARY_OP;
  case TOKEN_EOF:
    return PREDICATE_PARSER_ERROR_UNEXPECTED_EOF;
  case TOKEN_PAROPEN:
    if (_predicate_parser_push_state(token) < 0)
      return PREDICATE_PARSER_ERROR_STATE_TOO_LARGE;
    _parser.state= PARSER_STATE_WAIT_UNARY_OP;
    break;
  default:
    abort();
  }
  return PREDICATE_PARSER_SUCCESS;
}

// -----[ _predicate_parser_init ]-----------------------------------
static void _predicate_parser_init()
{
  _parser.state= PARSER_STATE_WAIT_UNARY_OP;
  _parser.stack= stack_create(MAX_DEPTH);
  _parser.cmd= TOKEN_NONE;
  _parser.left_predicate= NULL;
  _parser.right_predicate= NULL;
  _parser.column= 0;
}

// -----[ _predicate_parser_done ]-----------------------------------
static void _predicate_parser_destroy()
{
  _parser_stack_item_t * item;

  while (!stack_is_empty(_parser.stack)) {
    item= (_parser_stack_item_t *) stack_pop(_parser.stack);
    filter_matcher_destroy(&item->left_predicate);
    FREE(item);
  }
  stack_destroy(&_parser.stack);
}

// ----- predicate_parser -------------------------------------------
/**
 * 
 */
int predicate_parser(const char * expr, bgp_ft_matcher_t ** matcher_ref)
{
  int error= PREDICATE_PARSER_SUCCESS;
  int token;

  _predicate_parser_init();

  while (error == PREDICATE_PARSER_SUCCESS) {

#ifdef FSM_DEBUG
    _dump_stack();
#endif /* FSM_DEBUG */

    // Get next token
    token= _predicate_parser_get(&expr);
    if (token < 0) {
      error= token;
      break;
    }

#ifdef FSM_DEBUG
    stream_printf(gdserr, "----[token:%s]---->\n",
		  token_names[token]);
#endif /* FSM_DEBUG */

    switch (_parser.state) {
    case PARSER_STATE_WAIT_UNARY_OP:
      error= _predicate_parser_wait_unary_op(token); break;
    case PARSER_STATE_WAIT_BINARY_OP:
      error= _predicate_parser_wait_binary_op(token); break;
    case PARSER_STATE_WAIT_ARG:
      error= _predicate_parser_wait_arg(token); break;
    default:
      abort();
    }
  }

  // In case the stack is not empty when the FSM has finished,
  // and no error was returned, throw a parenthesis mismatch error.
  if (!stack_is_empty(_parser.stack))
    if (error >= 0)
      error= PREDICATE_PARSER_ERROR_PAR_MISMATCH;

  // In case of error, free the current predicate.
  // Otherwise, return the predicate by reference.
  if (error < 0)
    filter_matcher_destroy(&_parser.left_predicate);
  else
    *matcher_ref= _parser.left_predicate;

  _predicate_parser_destroy();

  return (error < 0)?error:PREDICATE_PARSER_SUCCESS;
}

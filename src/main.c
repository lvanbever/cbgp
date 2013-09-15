// ==================================================================
// @(#)main.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 22/11/2002
// $Id: main.c,v 1.39 2009-03-10 14:47:34 bqu Exp $
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
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "api.h"

#include <libgds/memory.h>
#include <libgds/str_util.h>

#include <cli/common.h>

//#include <net/ospf.h>

// -----[ simulator's modes] -----
#define CBGP_MODE_DEFAULT     0
#define CBGP_MODE_INTERACTIVE 1
#define CBGP_MODE_SCRIPT      2
#define CBGP_MODE_EXECUTE     3
#define CBGP_MODE_OSPF        4

// -----[ global options ]-----
uint8_t mode   = CBGP_MODE_DEFAULT;
char * arg_mode= NULL;

// -----[ simulation_cli_help ]--------------------------------------
/**
 * This function shows a command-line usage screen.
 */
void simulation_cli_help()
{
  libcbgp_banner();
  printf("\nUsage: cbgp [OPTIONS]\n");
  printf("\n");
  printf("  -h             display this message.\n");
  printf("  -l LOGFILE     output log to LOGFILE instead of stderr.\n");
  printf("  -c SCRIPT      load and execute SCRIPT file.\n");
  printf("                 (without this option, commands are taken from stdin)\n");
  printf("  -e COMMAND     execute the given command\n");
  printf("  -D param=value defines a parameter\n");

#ifdef OSPF_SUPPORT
  printf("  -o             test OSPF model (cbgp must be compiled with --enable-ospf option).\n");
#endif

  printf("\n");
}

// -----[ simulation_set_mode ]--------------------------------------
/**
 * Change the simulation mode.
 */
void simulation_set_mode(uint8_t new_mode, const char * new_arg)
{
  if (mode != CBGP_MODE_DEFAULT) {
    fprintf(stderr, "Error: multiple modes specified on command-line.\n");
    simulation_cli_help();
    exit(EXIT_FAILURE);
  }
   
  mode= new_mode;
  if (new_arg != NULL)
    arg_mode= str_create(new_arg);
}

/////////////////////////////////////////////////////////////////////
// INITIALIZATION AND FINALIZATION SECTION
/////////////////////////////////////////////////////////////////////

void _main_done() __attribute((destructor));

void _main_done()
{
  str_destroy(&arg_mode);
  libcbgp_done();
}

/////////////////////////////////////////////////////////////////////
//
// MAIN PROGRAM
//
/////////////////////////////////////////////////////////////////////

// ----- main -------------------------------------------------------
/**
 * This is C-BGP's main entry point.
 */
int main(int argc, char ** argv) {
  int result;
  int exit_code= EXIT_SUCCESS;
  gds_tokenizer_t * tokenizer;
  const gds_tokens_t * tokens;

  libcbgp_init(argc, argv);

  // Process command-line options
  while ((result= getopt(argc, argv, "mc:D:e:hil:ot:")) != -1) {
    switch (result) {
    case 'c':
      simulation_set_mode(CBGP_MODE_SCRIPT, optarg);
      break;
    case 'D':
      tokenizer= tokenizer_create("=", NULL, NULL);
      assert(tokenizer_run(tokenizer, optarg) == TOKENIZER_SUCCESS);
      tokens= tokenizer_get_tokens(tokenizer);
      assert(tokens_get_num(tokens) == 2);
      libcbgp_set_param(tokens_get_string_at(tokens, 0),
			tokens_get_string_at(tokens, 1));
      tokenizer_destroy(&tokenizer);
      break;
    case 'e':
      simulation_set_mode(CBGP_MODE_EXECUTE, optarg);
      break;
    case 'h':
      simulation_cli_help();
      exit(EXIT_SUCCESS);
      break;
    case 'l':
      libcbgp_set_debug_file(optarg);
      break;
#ifdef OSPF_SUPPORT
    case 'o':  //only to test ospf function
      simulation_set_mode(CBGP_MODE_OSPF, NULL);
      break;
#endif
    default:
      simulation_cli_help();
      exit(EXIT_FAILURE);
    }
  }

  if ((mode == CBGP_MODE_DEFAULT) && isatty(0))
    simulation_set_mode(CBGP_MODE_INTERACTIVE, NULL);

  /* Run simulation in selected mode... */
  switch (mode) {
  case CBGP_MODE_DEFAULT:
    if (libcbgp_exec_stream(stdin) != CLI_SUCCESS)
      exit_code= EXIT_FAILURE;
    break;
  case CBGP_MODE_INTERACTIVE:
    if (libcbgp_interactive() != CLI_SUCCESS)
      exit_code= EXIT_FAILURE;
    break;
  case CBGP_MODE_SCRIPT:
    exit_code= (libcbgp_exec_file(arg_mode) == 0)?
      EXIT_SUCCESS:EXIT_FAILURE;
    break;
  case CBGP_MODE_EXECUTE:
    exit_code= (libcbgp_exec_cmd(arg_mode) == 0)?
      EXIT_SUCCESS:EXIT_FAILURE;
    break;

  case CBGP_MODE_OSPF:
#ifdef OSPF_SUPPORT
    exit_code= ospf_test();
    if (exit_code != 0)
      printf("ospf_test ok!\n");
    else 
      printf("ospf_test has failed :-(\n");
#else
    printf("You must compile cbgp with --enable-ospf option to use OSPF model");
#endif
    break;
  }

  return exit_code;
}

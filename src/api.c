// ==================================================================
// @(#)api.c
//
// Application Programming Interface for the C-BGP library (libcsim).
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 25/10/2006
// $Id: api.c,v 1.17 2009-08-31 09:33:02 bqu Exp $
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

#include <limits.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <api.h>

#include <libgds/assoc_array.h>
#include <libgds/gds.h>
#include <libgds/tokenizer.h>
#include <libgds/tokens.h>

#include <bgp/as.h>
#include <bgp/aslevel/as-level.h>
#include <bgp/attr/comm.h>
#include <bgp/attr/comm_hash.h>
#include <bgp/attr/path.h>
#include <bgp/attr/path_hash.h>
#include <bgp/attr/path_segment.h>
#include <bgp/domain.h>
#include <bgp/filter/filter.h>
#include <bgp/filter/registry.h>
#include <bgp/mrtd.h>
#include <bgp/filter/predicate_parser.h>
#include <bgp/qos.h>
#include <bgp/aslevel/rexford.h>
#include <bgp/route.h>
#include <bgp/route_map.h>
#include <cli/common.h>
#include <net/igp_domain.h>
#include <net/netflow.h>
#include <net/network.h>
#include <net/ntf.h>
#include <net/tm.h>
#include <sim/simulator.h>
#include <ui/help.h>
#include <ui/rl.h>

#define COPYRIGHT_MSG				\
  "  Copyright (C) 2002-2012 Bruno Quoitin\n"		\
  "  Networking Lab\n"						\
  "  Computer Science Institute\n"					\
  "  University of Mons (UMONS)\n"					\
  "  Mons, Belgium\n"							\
  "  \n"								\
  "  This software is free for personal and educational uses.\n"	\
  "  See file COPYING for details.\n"					\


static gds_assoc_array_t * _main_params= NULL;
static param_lookup_t      _main_params_lookup;

// -----[ _signal_handler ]-------------------------------------------
/**
 * This is an ANSI C signal handler for SIGINT. The signal handler is
 * used to interrupt running simulations without quitting C-BGP.
 */
static void _signal_handler(int signum)
{
  network_t * network;
  simulator_t * sim;

  if (signum != SIGINT)
    return;

  network= network_get_default();
  if (network == NULL)
    return;

  sim= network_get_simulator(network);
  if ((sim != NULL) && sim_is_running(sim)) {
    cbgp_warn("Simulation interrupted by user.\n");
    sim_cancel(sim);
  }
}

// -----[ _file_is_executable ]--------------------------------------
/**
 *
 */
static inline int _file_is_executable(const char * path)
{
  struct stat sb;

  if (stat(path, &sb) < 0)
    return -1;

  // Regular file ?
  if ((sb.st_mode & S_IFMT) != S_IFREG)
    return 0;

  // Owner execute permission
  if (sb.st_uid == geteuid())
    return (sb.st_mode & S_IXUSR? 1 : 0);

  // Group execute permission
  if (sb.st_gid == getegid())
    return (sb.st_mode & S_IXGRP ? 1 : 0);

  // Others execute permission
  return (sb.st_mode & S_IXOTH ? 1 : 0);

}

// -----[ _get_exec_path ]-------------------------------------------
static inline char * _get_exec_path(const char * cmd)
{
  char * slash;
  char buf[PATH_MAX+1];
  char buf2[PATH_MAX+1];
  //char * exec_path;
  char * search_path;
  unsigned int index;
  gds_tokenizer_t * tokenizer;
  const gds_tokens_t * tokens;
  int result;

  // Locate slash
  slash= strchr(cmd, '/');
  if (!slash) {

    search_path= getenv("PATH");
    if (search_path == NULL)
      return NULL;
    if (strlen(search_path) <= 0)
      return NULL;

    tokenizer= tokenizer_create(":", NULL, NULL);
    tokenizer_run(tokenizer, search_path);

    tokens= tokenizer_get_tokens(tokenizer);

    buf[0]= '\0';
    for (index= 0; index < tokens_get_num(tokens); index++) {
      snprintf(buf, PATH_MAX+1, "%s/%s", tokens_get_string_at(tokens, index), cmd); 
      result= _file_is_executable(buf);
      if (result < 0)
	continue; // Could not stat file
      if (result > 0)
	break; // File is executable
    }
    if (strlen(buf) == 0)
      return NULL;
    cmd= buf;
  }

  // get absolute real path
  if (realpath(cmd, buf2) == NULL) {
    perror("realpath");
    return NULL;
  }

  // Extract path
  slash= strrchr(buf2, '/');
  if (slash) 
    return str_ncreate(buf2, slash-buf2);

  return NULL;
}

/////////////////////////////////////////////////////////////////////
//
// Initialization and configuration of the library.
//
/////////////////////////////////////////////////////////////////////

// -----[ libcbgp_init ]---------------------------------------------
/**
 * Initialize the C-BGP library.
 */
CBGP_EXP_DECL
void libcbgp_init(int argc, char * argv[])
{
  char * exec_path;

#ifdef __MEMORY_DEBUG__
  gds_init(GDS_OPTION_MEMORY_DEBUG);
#else
  gds_init(0);
#endif
  libcbgp_init2();

  exec_path= _get_exec_path(argv[0]);
  if (exec_path != NULL)
    libcbgp_set_param("EXEC_PATH", exec_path);
  str_destroy(&exec_path);

  assert(signal(SIGINT, _signal_handler) != SIG_ERR);
}

// -----[ libcbgp_done ]---------------------------------------------
/**
 * Free the resources allocated by the C-BGP library.
 */
CBGP_EXP_DECL
void libcbgp_done()
{
  libcbgp_done2();
  gds_destroy();
}

// -----[ libcbgp_banner ]-------------------------------------------
/**
 *
 */
CBGP_EXP_DECL
void libcbgp_banner()
{
  stream_printf(gdsout, "C-BGP Routing Solver version %s\n", PACKAGE_VERSION);
  stream_printf(gdsout, "\n%s\n", COPYRIGHT_MSG);
}

// -----[ libcbgp_set_debug_callback ]-------------------------------
/**
 *
 */
CBGP_EXP_DECL
void libcbgp_set_debug_callback(gds_stream_cb_f cb, void * ctx)
{
  gds_stream_t * debug= stream_create_callback(cb, ctx);
  
  if (debug == NULL) {
    cbgp_warn("couln't direct debug log to callback %p.\n", cb);
    return;
  }

  stream_destroy(&gdsdebug);
  gdsdebug= debug;
}

// -----[ libcbgp_set_err_callback ]---------------------------------
/**
 *
 */
CBGP_EXP_DECL
void libcbgp_set_err_callback(gds_stream_cb_f cb, void * ctx)
{
  gds_stream_t * err= stream_create_callback(cb, ctx);
  
  if (err == NULL) {
    cbgp_warn("couln't direct error log to callback %p.\n", cb);
    return;
  }

  stream_destroy(&gdserr);
  gdserr= err;
}

// -----[ libcbgp_set_out_callback ]---------------------------------
/**
 *
 */
CBGP_EXP_DECL
void libcbgp_set_out_callback(gds_stream_cb_f cb, void * ctx)
{
  gds_stream_t * out= stream_create_callback(cb, ctx);

  if (out == NULL) {
    cbgp_warn("couln't direct output to callback %p.\n", cb);
    return;
  }

  stream_destroy(&gdsout);
  gdsout= out;
}

// -----[ libcbgp_set_debug_level ]----------------------------------
/**
 *
 */
CBGP_EXP_DECL
void libcbgp_set_debug_level(stream_level_t level)
{
  stream_set_level(gdsdebug, level);
}

// -----[ libcbgp_set_err_level ]------------------------------------
/**
 *
 */
CBGP_EXP_DECL
void libcbgp_set_err_level(stream_level_t level)
{
  stream_set_level(gdserr, level);
}

// -----[ libcbgp_set_debug_file ]-----------------------------------
/**
 *
 */
CBGP_EXP_DECL
void libcbgp_set_debug_file(const char * filename)
{
  gds_stream_t * debug= stream_create_file(filename);

  if (debug == NULL) {
    fprintf(stderr, "Warning: couln't direct debug log to \"%s\".\n",
	    filename);
    return;
  }

  stream_destroy(&gdsdebug);
  gdsdebug= debug;
}


/////////////////////////////////////////////////////////////////////
//
// API
//
/////////////////////////////////////////////////////////////////////

// -----[ libcbgp_set_param ]----------------------------------------
CBGP_EXP_DECL
void libcbgp_set_param(const char * name, const char * value)
{
  assoc_array_set(_main_params, name, strdup(value));
}

// -----[ libcbgp_get_param ]----------------------------------------
CBGP_EXP_DECL
const char * libcbgp_get_param(const char * name)
{
  return assoc_array_get(_main_params, name);
}

// -----[ libcbgp_has_param ]----------------------------------------
CBGP_EXP_DECL
int libcbgp_has_param(const char * name)
{
  return assoc_array_exists(_main_params, name);
}

// -----[ libcbgp_get_param_lookup ]---------------------------------
CBGP_EXP_DECL
param_lookup_t libcbgp_get_param_lookup()
{
  return _main_params_lookup;
}

// -----[ libcbgp_exec_cmd ]-----------------------------------------
CBGP_EXP_DECL
int libcbgp_exec_cmd(const char * cmd)
{
  return cli_execute_line(cli_get(), cmd);
}

// -----[ libcbgp_exec_file ]----------------------------------------
CBGP_EXP_DECL
int libcbgp_exec_file(const char * filename)
{
  FILE * script= fopen(filename, "r");

  if (script == NULL) {
    stream_printf(gdserr, "Error: Unable to open script file \"%s\"\n",
		  filename);
    return -1;
  }
  
  if (libcbgp_exec_stream(script) != CLI_SUCCESS) {
    fclose(script);
    return -1;
  }

  fclose(script);
  return 0;
}

// -----[ libcbgp_exec_stream ]--------------------------------------
/**
 *
 */
CBGP_EXP_DECL
int libcbgp_exec_stream(FILE * stream)
{
  return cli_execute_stream(cli_get(), stream);
}

// -----[ libcbgp_interactive ]--------------------------------------
/**
 *
 */
CBGP_EXP_DECL
int libcbgp_interactive()
{
  int result= CLI_SUCCESS;
  char * line;

  libcbgp_banner();
  _help_init();
  _rl_init();

  fprintf(stdout, "cbgp> init.\n");

  do {

    // Get user-input
    line= rl_gets();

    // EOF has been caught (Ctrl-D), exit
    if (line == NULL) {
      fprintf(stdout, "\n");
      break;
    }

    // Execute command
    result= libcbgp_exec_cmd(line);
    if (result < CLI_SUCCESS)
      cli_dump_error(gdserr, cli_get());

  } while (result != CLI_SUCCESS_TERMINATE);

  fprintf(stdout, "cbgp> done.\n");

  _rl_destroy();
  _help_destroy();

  return result;
}


/////////////////////////////////////////////////////////////////////
//
// Initialization and configuration of the library (use with care).
//
/////////////////////////////////////////////////////////////////////

// -----[ _param_destroy ]-------------------------------------------
static void _param_destroy(void * item)
{
  free(item);
}

// -----[ libcbgp_init2 ]--------------------------------------------
void libcbgp_init2()
{
  // Initialize log.
  libcbgp_set_err_level(STREAM_LEVEL_WARNING);
  libcbgp_set_debug_level(STREAM_LEVEL_WARNING);

  // Initialize global params
  _main_params= assoc_array_create(_param_destroy);
  _main_params_lookup.lookup= default_lookup;
  _main_params_lookup.ctx   = _main_params;

  // Hash init code commented in order to allow parameter setup
  // through he command-line/script (initialization is performed
  // just-in-time).
  //_comm_hash_init();
  //_path_hash_init();

  _network_init();
  _bgp_domain_init();
  _ft_registry_init();
  _filter_path_regex_init();
  _route_maps_init();
  _ntf_init();
  _netflow_init();
  _tm_init();
  _cli_common_init();
}

// -----[ libcbgp_done2 ]--------------------------------------------
void libcbgp_done2()
{
  _cli_common_destroy();
  _tm_done();
  _netflow_done();
  _ntf_done();
  _message_destroy();
  _route_maps_destroy();
  _filter_path_regex_destroy();
  _ft_registry_done();
  _bgp_domain_destroy();
  _network_done();
  _mrtd_destroy();
  _path_hash_destroy();
  _comm_hash_destroy();
  _bgp_route_destroy();
  _aslevel_destroy();
  _path_destroy();
  _path_segment_destroy();
  _comm_destroy();

  assoc_array_destroy(&_main_params);
}





// ==================================================================
// @(#)common.c
//
// @author Bruno Quoitin (bruno.quoitin@umons.ac.be)
// @date 15/07/2003
// $Id: common.c,v 1.31 2009-08-24 10:33:30 bqu Exp $
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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <libgds/assoc_array.h>
#include <libgds/cli.h>
#include <libgds/cli_commands.h>
#include <libgds/cli_params.h>
#include <libgds/gds.h>
#include <libgds/stream.h>
#include <libgds/tokenizer.h>

#include <api.h>

#include <net/prefix.h>
#include <net/util.h>

#include <bgp/aslevel/as-level.h>
#include <bgp/attr/comm_hash.h>
#include <bgp/attr/path_hash.h>
#include <bgp/filter/predicate_parser.h>
#include <bgp/mrtd.h>
#include <bgp/routes_list.h>
#include <cli/bgp.h>
#include <cli/common.h>
#include <cli/net.h>
#include <cli/sim.h>
#include <ui/output.h>
#include <ui/rl.h>

#if defined(HAVE_SETRLIMIT) || defined(HAVE_GETRLIMIT)
# include <sys/resource.h>
# ifdef RLIMIT_AS
#  define _RLIMIT_RESOURCE RLIMIT_AS
# else
#  ifdef RLIMIT_VMEM
#   define _RLIMIT_RESOURCE RLIMIT_VMEM
#  else
#   undef HAVE_SETRLIMIT
#   undef HAVE_GETRLIMIT
#  endif
# endif
#endif


static cli_t * _main_cli= NULL;

// -----[ str2out_stream ]-------------------------------------------
/*gds_stream_t * str2out_stream(const char * str)
  {
  if (strcmp("stdout", str)) {
  stream= stream_create_file(str);
  if (stream == NULL) {
  cli_set_user_error(cli_get(), "unable to create file \"%s\"", str);
  return CLI_ERROR_COMMAND_FAILED;
  }
  }
  return NULL;
  }
*/

static inline
char * _arg_enum_array(const char * text, int state,
		       const char * values[], unsigned int num_values)
{
  size_t len= strlen(text);
  static unsigned int index;
  if (state == 0)
    index= 0;
  while (index < num_values) {
    if (!strncmp(values[index], text, len))
      return strdup(values[index++]);
    index++;
  }
  return NULL;
}

#define DEF_ENUM(NAME,VALUES,SIZE)			    \
  static char * NAME(const char * text, int state) {	    \
    return _arg_enum_array(text, state, VALUES, SIZE);	    \
  }

static const char * ARRAY_ENUM_ON_OFF[2]= { "on", "off" };
DEF_ENUM(_arg_enum_on_off, ARRAY_ENUM_ON_OFF, 2);
/*static const char * ARRAY_ENUM_IN_OUT[2]= { "in", "out" };
  DEF_ENUM(_arg_enum_in_out, ARRAY_ENUM_IN_OUT, 2);*/

// -----[ cli_opt_on_off ]-------------------------------------------
cli_arg_t * cli_opt_on_off(const char * name)
{
  return cli_opt2(name, NULL, _arg_enum_on_off);
}

// -----[ cli_arg_on_off ]-------------------------------------------
cli_arg_t * cli_arg_on_off(const char * name)
{
  if (name == NULL)
    return cli_arg2("on|off", NULL, _arg_enum_on_off);
  return cli_arg(name, NULL);
}

// ----- str2boolean ------------------------------------------------
int str2boolean(const char * str, int * value)
{
  if (!strcmp(str, "on")) {
    *value= 1;
    return 0;
  } else if (!strcmp(str, "off")) {
    *value= 0;
    return 0;
  }
  return -1;
}

// ----- parse_version ----------------------------------------------
/**
 * Parse a C-BGP version.
 *
 * A C-BGP version must have the following format:
 *
 *   <major> . <minor> . <revision> [- <name> ]
 *
 * where <major>, <minor> and <revision> are positive integers and
 * <name> is a non-empty string of non-white space characters.
 */
int parse_version(const char * str, unsigned int * version)
{
  gds_tokenizer_t * tk= NULL;
  const gds_tokens_t * tks;
  char * str2;
  unsigned int sub_version;
  unsigned int factor= 1000000;
  unsigned int index;

  *version= 0;

  // Remove version suffix ("rc0", "beta", ...)
  tk= tokenizer_create("-", NULL, NULL);
  tokenizer_set_flag(tk, TOKENIZER_OPT_SINGLE_DELIM);
  if (tokenizer_run(tk, str) != TOKENIZER_SUCCESS) {
    tokenizer_destroy(&tk);
    return -1;
  }
  tks= tokenizer_get_tokens(tk);
  if ((tokens_get_num(tks) < 1) ||
      (tokens_get_num(tks) > 2)) {
    tokenizer_destroy(&tk);
    return -1;
  }
  str2= str_create(tokens_get_string_at(tks, 0));
  tokenizer_destroy(&tk);

  // Split version in sub-versions
  tk= tokenizer_create(".", NULL, NULL);
  tokenizer_set_flag(tk, TOKENIZER_OPT_SINGLE_DELIM);
  if (tokenizer_run(tk, str2) != TOKENIZER_SUCCESS) {
    tokenizer_destroy(&tk);
    return -1;
  }
  tks= tokenizer_get_tokens(tk);
  if (tokens_get_num(tks) != 3) {
    tokenizer_destroy(&tk);
    return -1;
  }
  for (index= 0; index < 3; index++) {
    if (tokens_get_uint_at(tks, index, &sub_version) ||
	(sub_version >= 100)) {
      tokenizer_destroy(&tk);
      return -1;
    }
    (*version)+= factor * sub_version;
    factor/= 100;
  }
  str_destroy(&str2);
  tokenizer_destroy(&tk);
  return 0;
}

// -----[ str2addr_id ]----------------------------------------------
/**
 * Return an IP address identifier based on a string. The string can
 * contain the dotted version of the IP address or "AS" followed by
 * an ASN in case AS-level topologies are used.
 */
int str2addr_id(const char * id, net_addr_t * addr)
{
  asn_t asn;
  as_level_topo_t * topo;

  if (strncmp("AS", id, 2))
    return str2address(id, addr);

  id+= 2;
  if (str2asn(id, &asn))
    return -1;
  topo= aslevel_get_topo();
  if (topo == NULL)
    return -1;
  *addr= topo->addr_mapper(asn);
  return 0;
}

// -----[ str2node ]-------------------------------------------------
int str2node(const char * str, net_node_t ** node)
{
  net_addr_t addr;

  if (str2address(str, &addr))
    return -1;
  *node= network_find_node(network_get_default(), addr);
  if (*node == NULL)
    return -1;
  return 0;
}

// -----[ str2node_id ]----------------------------------------------
int str2node_id(const char * str, net_node_t ** node)
{
  as_level_topo_t * topo;
  unsigned int asn;
  net_addr_t addr;
  
  // Node address
  if (strncmp("AS", str, 2))
    return str2node(str, node);

  // Node ASN
  str+= 2;
  if (str_as_uint(str, &asn))
    return -1;
  topo= aslevel_get_topo();
  if (topo == NULL)
    return -1;
  addr= topo->addr_mapper(asn);
  *node= network_find_node(network_get_default(), addr);
  if (*node == NULL)
    return -1;
  return 0;
}

// -----[ str2dest_id ]----------------------------------------------
int str2dest_id(const char * str, ip_dest_t * dest)
{
  as_level_topo_t * topo;
  unsigned int asn;

  // Classical destination (address / prefix)
  if (strncmp("AS", str, 2))
    return ip_string_to_dest(str, dest);

  // ASN destination
  str+= 2;
  if (str_as_uint(str, &asn))
    return -1;
  topo= aslevel_get_topo();
  if (topo == NULL)
    return -1;
  dest->type= NET_DEST_ADDRESS;
  dest->addr= topo->addr_mapper(asn);
  return 0;
}

// -----[ str2subnet ]-----------------------------------------------
int str2subnet(const char * str, net_subnet_t ** subnet)
{
  ip_pfx_t prefix;

  if (str2prefix(str, &prefix))
    return -1;
  *subnet= network_find_subnet(network_get_default(), prefix);
  if (*subnet == NULL)
    return -1;
  return 0;
}

// -----[ cli_arg_file ]---------------------------------------------
cli_arg_t * cli_arg_file(const char * name, cli_arg_check_f check)
{
#ifdef _FILENAME_COMPLETION_FUNCTION
  return cli_arg2(name, check, _FILENAME_COMPLETION_FUNCTION);
#else
  return cli_arg(name, check);
#endif
}

// -----[ cli_opt_file ]---------------------------------------------
cli_arg_t * cli_opt_file(const char * name, cli_arg_check_f check)
{
#ifdef _FILENAME_COMPLETION_FUNCTION
  return cli_opt2(name, check, _FILENAME_COMPLETION_FUNCTION);
#else
  return cli_opt(name, check);
#endif
}

// -----[ cli_require_param ]----------------------------------------
/**
 * context: {}
 * tokens : {name}
 */
int cli_require_param(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg= cli_get_arg_value(cmd, 0);
  if (!libcbgp_has_param(arg)) {
    cli_set_user_error(cli_get(), "parameter required \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// ----- cli_require_version ----------------------------------------
int cli_require_version(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg= cli_get_arg_value(cmd, 0);
  unsigned int req_version;
  unsigned int version;
  
  // Get required version
  if (parse_version(arg, &req_version)) {
    cli_set_user_error(cli_get(), "invalid version \"\".", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }
  if (parse_version(PACKAGE_VERSION, &version)) {
    cli_set_user_error(cli_get(), "invalid version \"\".", PACKAGE_VERSION);
    return CLI_ERROR_COMMAND_FAILED;
  }
  if (req_version > version) {
    cli_set_user_error(cli_get(), "version %s > version %s.",
		       arg, PACKAGE_VERSION);
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  return CLI_SUCCESS;
}

// ----- cli_set_autoflush ------------------------------------------
int cli_set_autoflush(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg= cli_get_arg_value(cmd, 0);
  if (str2boolean(arg, &iOptionAutoFlush) != 0) {
    cli_set_user_error(cli_get(), "invalid value \"%s\" for option "
		       "autoflush", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// ----- cli_set_exitonerror ----------------------------------------
int cli_set_exitonerror(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg= cli_get_arg_value(cmd, 0);
  if (str2boolean(arg, &iOptionExitOnError) != 0) {
    cli_set_user_error(cli_get(), "invalid value \"%s\" for option "
		       "exit-on-error", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// ----- cli_set_comm_hash_size -------------------------------------
int cli_set_comm_hash_size(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg= cli_get_arg_value(cmd, 0);
  unsigned long size;

  // Get the hash size
  if (str_as_ulong(arg, &size) < 0) {
    cli_set_user_error(cli_get(), "invalid size \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Set the hash size
  if (comm_hash_set_size(size) != 0) {
    cli_set_user_error(cli_get(), "could not set comm-hash size");
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  return CLI_SUCCESS;
}

// -----[ cli_show_comm_hash_content ]-------------------------------
int cli_show_comm_hash_content(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg= cli_get_arg_value(cmd, 0);
  gds_stream_t * stream= gdsout;

  if (strcmp(arg, "stdout")) {
    stream= stream_create_file(arg);
    if (stream == NULL) {
      cli_set_user_error(cli_get(), "unable to create file \"%s\"", arg);
      return CLI_ERROR_COMMAND_FAILED;
    }
  }

  comm_hash_content(stream);
  if (stream != gdsout)
    stream_destroy(&stream);
  return CLI_SUCCESS;
}

// ----- cli_show_comm_hash_size ------------------------------------
int cli_show_comm_hash_size(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  stream_printf(gdsout, "%u\n", comm_hash_get_size());
  return CLI_SUCCESS;
}

// -----[ cli_show_comm_hash_stat ]----------------------------------
int cli_show_comm_hash_stat(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg= cli_get_arg_value(cmd, 0);
  gds_stream_t * stream= gdsout;

  if (strcmp(arg, "stdout")) {
    stream= stream_create_file(arg);
    if (stream == NULL) {
      cli_set_user_error(cli_get(), "unable to create file \"%s\"", arg);
      return CLI_ERROR_COMMAND_FAILED;
    }
  }

  comm_hash_statistics(stream);
  if (stream != gdsout)
    stream_destroy(&stream);
  return CLI_SUCCESS;
}

// -----[ cli_show_commands ]----------------------------------------
int cli_show_commands(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  unsigned int i;

  for (i= 0; i < ptr_array_length(_main_cli->root_cmd->sub_cmds); i++)
    cli_cmd_dump(gdsout, "  ",
		 (cli_cmd_t *) _main_cli->root_cmd->sub_cmds->data[i], 1);
  return CLI_SUCCESS;
}

// ----- cli_set_debug ----------------------------------------------
int cli_set_debug(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg= cli_get_arg_value(cmd, 0);
  if (str2boolean(arg, &iOptionDebug) != 0) {
    cli_set_user_error(cli_get(), "invalid value \"%s\" for option debug",
		       arg);
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// -----[ cli_show_mrt ]---------------------------------------------
/**
 * context: {}
 * tokens: {filename, predicate}
 */
int cli_show_mrt(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
#ifdef HAVE_BGPDUMP
  /*
  char * pcPredicate;
  SFilterMatcher * pMatcher;

  // Parse predicate
  pcPredicate= tokens_get_string_at(cmd->arg_values, 1);
  if (predicate_parse(&pcPredicate, &pMatcher)) {
    STREAM_ERR(STREAM_LEVEL_SEVERE, "Error: invalid predicate \"%s\"\n",
	    pcPredicate);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Dump routes that match the given predicate
  mrtd_load_routes(tokens_get_string_at(cmd->arg_values, 0), 1, pMatcher);
*/
  cli_set_user_error(cli_get(), "this command has been removed.");
  return CLI_ERROR_COMMAND_FAILED;
#else
  cli_set_user_error(cli_get(), "compiled without bgpdump.");
  return CLI_ERROR_COMMAND_FAILED;
#endif
}

// ----- cli_show_mem_limit -----------------------------------------
int cli_show_mem_limit(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
#ifdef HAVE_GETRLIMIT
  struct rlimit rlim;

  if (getrlimit(_RLIMIT_RESOURCE, &rlim) < 0) {
    cli_set_user_error(cli_get(), "getrlimit failed (%s)", strerror(errno));
    return CLI_ERROR_COMMAND_FAILED;
  }

  stream_printf(gdsout, "soft limit: ");
  if (rlim.rlim_cur == RLIM_INFINITY) {
    stream_printf(gdsout, "unlimited\n");
  } else {
    stream_printf(gdsout, "%u byte(s)\n", (unsigned int) rlim.rlim_cur);
  }
  stream_printf(gdsout, "hard limit: ");
  if (rlim.rlim_max == RLIM_INFINITY) {
    stream_printf(gdsout, "unlimited\n");
  } else {
    stream_printf(gdsout, "%u byte(s)\n", (unsigned int) rlim.rlim_max);
  }

  stream_flush(gdsout);

  return CLI_SUCCESS;
#else
  cli_set_user_error(cli_get(), "getrlimit is not supported by your system");
  return CLI_ERROR_COMMAND_FAILED;
#endif
}

// ----- cli_set_mem_limit ------------------------------------------
int cli_set_mem_limit(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
#if defined(HAVE_SETRLIMIT) && defined(HAVE_GETRLIMIT)
  const char * arg= cli_get_arg_value(cmd, 0);
  unsigned long l_limit;
  rlim_t limit;
  struct rlimit rlim;

  // Get the value of the memory-limit
  if (!strcmp(arg, "unlimited")) {
      limit= RLIM_INFINITY;
  } else if (!str_as_ulong(arg, &l_limit)) {
    if (sizeof(limit) < sizeof(l_limit)) {
      STREAM_ERR(STREAM_LEVEL_WARNING,
	      "Warning: limit may be larger than supported by system.\n");
    }
    limit= (rlim_t) l_limit;
  } else {
    cli_set_user_error(cli_get(), "invalid mem limit \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Get the soft limit on the process's size of virtual memory
  if (getrlimit(_RLIMIT_RESOURCE, &rlim) < 0) {
    cli_set_user_error(cli_get(), "getrlimit failed (%s)", strerror(errno));
    return CLI_ERROR_COMMAND_FAILED;
  }

  rlim.rlim_cur= limit;

  /* Set new soft limit on the process's size of virtual memory */
  if (setrlimit(_RLIMIT_RESOURCE, &rlim) < 0) {
    cli_set_user_error(cli_get(), "setrlimit failed (%s)", strerror(errno));
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  return CLI_SUCCESS;
#else
  cli_set_user_error(cli_get(), "setrlimit is not supported by your system.");
  return CLI_ERROR_COMMAND_FAILED;
#endif
}

// ----- cli_set_path_hash_size -------------------------------------
int cli_set_path_hash_size(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg= cli_get_arg_value(cmd, 0);
  unsigned long size;

  // Get the hash size
  if (str_as_ulong(arg, &size)) {
    cli_set_user_error(cli_get(), "invalid size \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Set the hash size
  if (path_hash_set_size(size) != 0) {
    cli_set_user_error(cli_get(), "could not set path-hash size");
    return CLI_ERROR_COMMAND_FAILED;
  }
  
  return CLI_SUCCESS;
}

// -----[ cli_show_path_hash_content ]-------------------------------
int cli_show_path_hash_content(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg= cli_get_arg_value(cmd, 0);;
  gds_stream_t * stream= gdsout;

  if (strcmp(arg, "stdout")) {
    stream= stream_create_file(arg);
    if (stream == NULL) {
      cli_set_user_error(cli_get(), "unable to create file \"%s\"", arg);
      return CLI_ERROR_COMMAND_FAILED;
    }
  }

  path_hash_content(stream);
  if (stream != gdsout)
    stream_destroy(&stream);
  return CLI_SUCCESS;
}

// ----- cli_show_path_hash_size ------------------------------------
int cli_show_path_hash_size(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  stream_printf(gdsout, "%u\n", path_hash_get_size());
  return CLI_SUCCESS;
}

// -----[ cli_show_path_hash_stat ]----------------------------------
int cli_show_path_hash_stat(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg= cli_get_arg_value(cmd, 0);
  gds_stream_t * stream= gdsout;

  if (strcmp(arg, "stdout")) {
    stream= stream_create_file(arg);
    if (stream == NULL) {
      cli_set_user_error(cli_get(), "unable to create file \"%s\"", arg);
      return CLI_ERROR_COMMAND_FAILED;
    }
  }

  path_hash_statistics(stream);
  if (stream != gdsout)
    stream_destroy(&stream);
  return CLI_SUCCESS;
}

// ----- cli_show_version -------------------------------------------
int cli_show_version(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  stream_printf(gdsout, "%s version: %s", PACKAGE_NAME, PACKAGE_VERSION);
#ifdef __EXPERIMENTAL__ 
  stream_printf(gdsout, " [experimental]");
#endif
#ifdef HAVE_LIBZ
  stream_printf(gdsout, " [zlib]");
#endif
#ifdef HAVE_JNI
  stream_printf(gdsout, " [jni]");
#endif
#ifdef HAVE_BGPDUMP
  stream_printf(gdsout, " [bgpdump]");
#endif
#ifdef __ROUTER_LIST_ENABLE__
  stream_printf(gdsout, " [router-list]");
#endif
#ifdef __EXPERIMENTAL_ADVERTISE_BEST_EXTERNAL_TO_INTERNAL__
  stream_printf(gdsout, " [external-best]");
#endif
  stream_printf(gdsout, "\n");

  stream_printf(gdsout, "libgds version: %s\n", gds_version());

  stream_flush(gdsout);

  return CLI_SUCCESS;
}

// -----[ cli_define ]-----------------------------------------------
/**
 * context: {}
 * tokens : {name, value}
 */
static int cli_define(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  cli_set_param(cli_get_arg_value(cmd, 0),
		cli_get_arg_value(cmd, 1));
  return CLI_SUCCESS;
}

// ----- cli_include ------------------------------------------------
/**
 * context: {}
 * tokens : {filename}
 */
static int cli_include(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg= cli_get_arg_value(cmd, 0);
  FILE * file;
  cli_error_t cli_error;
  int result;
  int line_number;

  file= fopen(arg, "r");
  if (file == NULL) {
    cli_set_user_error(cli_get(), "unable to load file \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  cli_get_error_details(cli_get(), &cli_error);
  line_number= cli_error.line_number;
  result= cli_execute_stream(_main_cli, file);
  if (result != CLI_SUCCESS) {
    cli_get_error_details(cli_get(), &cli_error);
    cli_set_user_error(cli_get(), "in file \"%s\", line %d (%s)",
		       arg, cli_error.line_number,
		       cli_error.user_msg);
    // Beeeerk, I shouldn't do that...
    cli_get()->error.line_number= line_number;
  }
  fclose(file);
  return result;
}

// ----- cli_pause --------------------------------------------------
static int cli_pause(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  stream_printf(gdsout, "Paused: hit 'Enter' to continue...");
  stream_flush(gdsout);
  fgetc(stdin);
  stream_printf(gdsout, "\n");
  return CLI_SUCCESS;
}

// ----- cli_print --------------------------------------------------
static int cli_print(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  stream_printf(gdsout, cli_get_arg_value(cmd, 0));
  stream_flush(gdsout);
  return CLI_SUCCESS;
}

// -----[ cli_quit ]-------------------------------------------------
static int cli_quit(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  return CLI_SUCCESS_TERMINATE;
}

// -----[ cli_time_diff ]--------------------------------------------
static time_t saved_time= 0;
static int cli_time_diff(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  time_t current_time= time(NULL);

  if (saved_time == 0) {
    return CLI_ERROR_COMMAND_FAILED;
  }

  stream_printf(gdsout, "%f\n", difftime(current_time, saved_time));
  return CLI_SUCCESS;
}

// -----[ cli_time_save ]--------------------------------------------
static int cli_time_save(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  saved_time= time(NULL);
  return CLI_SUCCESS;
}

// -----[ cli_chdir ]------------------------------------------------
static int cli_chdir(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg= cli_get_arg_value(cmd, 0);
  if (chdir(arg) < 0) {
    cli_set_user_error(cli_get(), "unable to change directory \"%s\" (%s)",
		       arg, strerror(errno));
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// CLI COMMANDS REGISTRATION
//
/////////////////////////////////////////////////////////////////////

// -----[ _register_set ]--------------------------------------------
static void _register_set(cli_cmd_t * parent)
{
  cli_cmd_t * group, * cmd;

  group= cli_add_cmd(parent, cli_cmd_group("set"));
  cmd= cli_add_cmd(group, cli_cmd("autoflush", cli_set_autoflush));
  cli_add_arg(cmd, cli_arg_on_off(NULL));
  cmd= cli_add_cmd(group, cli_cmd("comm-hash-size", cli_set_comm_hash_size));
  cli_add_arg(cmd, cli_arg("size", NULL));
  cmd= cli_add_cmd(group, cli_cmd("debug", cli_set_debug));
  cli_add_arg(cmd, cli_arg_on_off(NULL));
  cmd= cli_add_cmd(group, cli_cmd("exit-on-error", cli_set_exitonerror));
  cli_add_arg(cmd, cli_arg_on_off(NULL));
  cmd= cli_add_cmd(group, cli_cmd("mem-limit", cli_set_mem_limit));
  cli_add_arg(cmd, cli_arg("value", NULL));
  cmd= cli_add_cmd(group, cli_cmd("path-hash-size", cli_set_path_hash_size));
  cli_add_arg(cmd, cli_arg("size", NULL));
//  cmd= cli_add_cmd(group, cli_cmd("time-limit", cli_set_time_limit));
//  cli_add_arg(cmd, cli_arg("time", NULL));
}

//-----[ _register_show ]--------------------------------------------
static void _register_show(cli_cmd_t * parent)
{
  cli_cmd_t * group, * cmd;

  group= cli_add_cmd(parent, cli_cmd_group("show"));
  cmd= cli_add_cmd(group, cli_cmd("comm-hash-content",
				  cli_show_comm_hash_content));
  cli_add_arg(cmd, cli_arg_file("output", NULL));
  cmd= cli_add_cmd(group, cli_cmd("comm-hash-size", cli_show_comm_hash_size));
  cmd= cli_add_cmd(group, cli_cmd("comm-hash-stat", cli_show_comm_hash_stat));
  cli_add_arg(cmd, cli_arg_file("output", NULL));
  cmd= cli_add_cmd(group, cli_cmd("commands", cli_show_commands));
  cmd= cli_add_cmd(group, cli_cmd("mrt", cli_show_mrt));
  cli_add_arg(cmd, cli_arg_file("file", NULL));
  cli_add_arg(cmd, cli_arg("predicate", NULL));
  cmd= cli_add_cmd(group, cli_cmd("mem-limit", cli_show_mem_limit));
  cmd= cli_add_cmd(group, cli_cmd("path-hash-content",
				  cli_show_path_hash_content));
  cli_add_arg(cmd, cli_arg("output", NULL));
  cmd= cli_add_cmd(group, cli_cmd("path-hash-size", cli_show_path_hash_size));
  cmd= cli_add_cmd(group, cli_cmd("path-hash-stat", cli_show_path_hash_stat));
  cli_add_arg(cmd, cli_arg("output", NULL));
  cli_add_cmd(group, cli_cmd("version", cli_show_version));
}

// -----[ _register_define ]-----------------------------------------
static void _register_define(cli_cmd_t * parent)
{
  cli_cmd_t * cmd= cli_add_cmd(parent, cli_cmd("define", cli_define));
  cli_add_arg(cmd, cli_arg("name", NULL));
  cli_add_arg(cmd, cli_arg("value", NULL));
}
// -----[ _register_include ]----------------------------------------
static void _register_include(cli_cmd_t * parent)
{
  cli_cmd_t * cmd= cli_add_cmd(parent, cli_cmd("include", cli_include));
  cli_add_arg(cmd, cli_arg_file("file", NULL));
}

// -----[ _register_pause ]------------------------------------------
static void _register_pause(cli_cmd_t * parent)
{
  cli_add_cmd(parent, cli_cmd("pause", cli_pause));
}

// -----[ _register_print ]------------------------------------------
static void _register_print(cli_cmd_t * parent)
{
  cli_cmd_t * cmd= cli_add_cmd(parent, cli_cmd("print", cli_print));
  cli_add_arg(cmd, cli_arg("message", NULL));
}

// -----[ _register_quit ]-------------------------------------------
static void _register_quit(cli_cmd_t * parent)
{
  cli_add_cmd(parent, cli_cmd("quit", cli_quit));
}

// -----[ _register_require ]----------------------------------------
static void _register_require(cli_cmd_t * parent)
{
  cli_cmd_t * group, * cmd;
  
  group= cli_add_cmd(parent, cli_cmd_group("require"));
  cmd= cli_add_cmd(group, cli_cmd("param", cli_require_param));
  cli_add_arg(cmd, cli_arg("name", NULL));
  cmd= cli_add_cmd(group, cli_cmd("version", cli_require_version));
  cli_add_arg(cmd, cli_arg("version", NULL));
}

// -----[ _register_time ]-------------------------------------------
static void _register_time(cli_cmd_t * parent)
{
  cli_cmd_t * group;
  
  group= cli_add_cmd(parent, cli_cmd_group("time"));
  cli_add_cmd(group, cli_cmd("diff", cli_time_diff));
  cli_add_cmd(group, cli_cmd("save", cli_time_save));
}

// -----[ _register_chdir ]------------------------------------------
static void _register_chdir(cli_cmd_t * parent)
{
  cli_cmd_t * cmd= cli_add_cmd(parent, cli_cmd("chdir", cli_chdir));
  cli_add_arg(cmd, cli_arg("dir", NULL));
}

// -----[ _cli_on_error ]--------------------------------------------
static int _cli_on_error(cli_t * cli, int result)
{
  cli_dump_error(gdserr, cli);
  return (iOptionExitOnError?result:CLI_SUCCESS);
}

// -----[ cli_get ]--------------------------------------------------
cli_t * cli_get()
{
  cli_cmd_t * root;

  if (_main_cli == NULL) {

    _main_cli= cli_create();
    cli_set_param_lookup(_main_cli, libcbgp_get_param_lookup());
    cli_set_on_error(_main_cli, _cli_on_error);

    root= cli_get_root_cmd(_main_cli);

    /* Command classes */
    cli_register_bgp(root);
    cli_register_net(root);
    cli_register_sim(root);

    // Miscelaneous commands
    _register_define(root);
    _register_include(root);
    _register_pause(root);
    _register_print(root);
    _register_require(root);
    _register_set(root);
    _register_show(root);
    _register_time(root);
    _register_chdir(root);

    // Omnipresent commands
    _register_quit(cli_get_omni_cmd(_main_cli));
  }
  return _main_cli;
}

// -----[ cli_set_param ]------------------------------------------
void cli_set_param(const char * name, const char * value)
{
  libcbgp_set_param(name, value);
}

// -----[ cli_get_param ]------------------------------------------
const char * cli_get_param(const char * name)
{
  return libcbgp_get_param(name);
}


/////////////////////////////////////////////////////////////////////
// INITIALIZATION AND FINALIZATION SECTION
/////////////////////////////////////////////////////////////////////

// -----[ _cli_common_init ]-----------------------------------------
void _cli_common_init()
{
}

// -----[ _cli_common_destroy ]--------------------------------------
void _cli_common_destroy()
{
  cli_destroy(&_main_cli);
}

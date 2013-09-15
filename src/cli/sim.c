// ==================================================================
// @(#)sim.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 24/07/2003
// $Id: sim.c,v 1.14 2009-03-24 15:58:43 bqu Exp $
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

#include <libgds/cli.h>
#include <libgds/cli_ctx.h>
#include <libgds/cli_params.h>
#include <libgds/stream.h>
#include <libgds/str_util.h>

#include <cli/common.h>
#include <cli/sim.h>
#include <net/network.h>
#include <sim/simulator.h>

// -----[ cli_sim_clear ]--------------------------------------------
/**
 * context: {}
 * tokens : {}
 */
static int cli_sim_clear(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  sim_clear(network_get_simulator(network_get_default()));
  return CLI_SUCCESS;
}

// -----[ cli_sim_debug_queue ]--------------------------------------
/**
 * context: {}
 * tokens: {}
 */
static int cli_sim_debug_queue(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  sim_dump_events(gdserr, network_get_simulator(network_get_default()));
  return CLI_SUCCESS;
}

// -----[ _cli_sim_event_callback ]----------------------------------
static int _cli_sim_event_callback(simulator_t * sim,
				   void * ctx)
{
  char * cmd= (char *) ctx;
  int result= 0;
  cli_t * cli= cli_get();

  if (cli != NULL)
    if (cli_execute(cli, cmd) < 0)
      result= -1;
  str_destroy(&cmd);
  return result;
}

// -----[ _cli_sim_event_destroy ]-----------------------------------
static void _cli_sim_event_destroy(void * ctx)
{
  char * cmd= (char *) ctx;
  str_destroy(&cmd);
}

// -----[ _cli_sim_event_dump ]--------------------------------------
static void _cli_sim_event_dump(gds_stream_t * stream, void * ctx)
{
  char * cmd= (char *) ctx;
  stream_printf(stream, "cli-event [%s]\n", cmd);
}

static sim_event_ops_t _sim_event_ops= {
  .callback= _cli_sim_event_callback,
  .destroy = _cli_sim_event_destroy,
  .dump    = _cli_sim_event_dump,
};

// ----- cli_sim_event ----------------------------------------------
/**
 * context: {}
 * tokens: {time, command}
 */
int cli_sim_event(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg_time= cli_get_arg_value(cmd, 0);
  const char * arg_event= cli_get_arg_value(cmd, 1);
  double time;

  if (str_as_double(arg_time, &time)) {
    cli_set_user_error(cli_get(), "Invalid simulation time \"%s\"", arg_time);
    return CLI_ERROR_COMMAND_FAILED;
  }
  sim_post_event(network_get_simulator(network_get_default()),
		 &_sim_event_ops,
		 str_create(arg_event),
		 time, SIM_TIME_ABS);
  return CLI_SUCCESS;
}

// ----- cli_sim_options_loglevel -----------------------------------
/**
 * context: {}
 * tokens: {log-level}
 */
int cli_sim_options_loglevel(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg= cli_get_arg_value(cmd, 0);
  stream_level_t level;
  if (stream_str2level(arg, &level) < 0) {
    cli_set_user_error(cli_get(), "Invalid log level \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }
  stream_set_level(gdsdebug, level);
  return CLI_SUCCESS;
}

// ----- cli_sim_options_scheduler ----------------------------------
/**
 * context: {}
 * tokens: {function}
 */
int cli_sim_options_scheduler(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg= cli_get_arg_value(cmd, 0);
  sched_type_t type;

  if (sim_str2sched(arg, &type) < 0) {
    cli_set_user_error(cli_get(), "invalid scheduler \"%s\"", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }
  sim_set_default_scheduler(type);
  return CLI_SUCCESS;
}

// ----- cli_sim_queue_info -----------------------------------------
/**
 *
 */
int cli_sim_queue_info(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  sim_show_infos(gdsout, network_get_simulator(network_get_default()));
  return CLI_SUCCESS;
}

// ----- cli_sim_queue_log ------------------------------------------
/**
 *
 */
int cli_sim_queue_log(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg= cli_get_arg_value(cmd, 0);
  sim_set_log_progress(network_get_simulator(network_get_default()), arg);
  return CLI_SUCCESS;
}

// ----- cli_sim_queue_show -----------------------------------------
/**
 *
 */
int cli_sim_queue_show(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  sim_dump_events(gdsout, network_get_simulator(network_get_default()));
  return CLI_SUCCESS;
}


// ----- cli_sim_run ------------------------------------------------
int cli_sim_run(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  simulator_t * sim= network_get_simulator(network_get_default());
  int error= sim_run(sim);
  if (error) {
    if (error == ESIM_TIME_LIMIT) {
      cbgp_warn("simulation stopped @ %2.2f.\n", sim_get_time(sim));
      return CLI_SUCCESS;
    }
    cli_set_user_error(cli_get(), "could not run simulation (%s)",
		       network_strerror(error));
    return CLI_ERROR_COMMAND_FAILED;
  }
  return CLI_SUCCESS;
}

// ----- cli_sim_step -----------------------------------------------
/**
 * context: {}
 * tokens: {num-steps}
 */
int cli_sim_step(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * arg= cli_get_arg_value(cmd, 0);
  unsigned int num_steps;

  // Get number of steps
  if (str_as_uint(arg, &num_steps)) {
    cli_set_user_error(cli_get(), "invalid number of steps \"%s\".", arg);
    return CLI_ERROR_COMMAND_FAILED;
  }

  // Run the simulator for the given number of steps
  if (sim_step(network_get_simulator(network_get_default()), num_steps))
    return CLI_ERROR_COMMAND_FAILED;

  return CLI_SUCCESS;
}

// ----- cli_sim_stop -----------------------------------------------
/**
 * context: {}
 * tokens : {}
 * options: {--at=<time>}
 */
int cli_sim_stop(cli_ctx_t * ctx, cli_cmd_t * cmd)
{
  const char * opt= cli_get_opt_value(cmd, "at");
  double time;

  if (opt != NULL) {
    if (str_as_double(opt, &time)) {
      cli_set_user_error(cli_get(), "Invalid simulation time \"%s\"", opt);
      return CLI_ERROR_COMMAND_FAILED;
    }
  } else {
    cli_set_user_error(cli_get(), "Option --at=<time> must be specified.");
    return CLI_ERROR_COMMAND_FAILED;    
  }

  sim_set_max_time(network_get_simulator(network_get_default()), time);
  return CLI_SUCCESS;
}


/////////////////////////////////////////////////////////////////////
//
// CLI COMMANDS REGISTRATION
//
/////////////////////////////////////////////////////////////////////

// -----[ _register_sim_clear ]--------------------------------------
static void _register_sim_clear(cli_cmd_t * parent)
{
  cli_add_cmd(parent, cli_cmd("clear", cli_sim_clear));
}

// -----[ _register_sim_debug ]--------------------------------------
static void _register_sim_debug(cli_cmd_t * parent)
{
  cli_cmd_t * group= cli_add_cmd(parent, cli_cmd_group("debug"));
  cli_add_cmd(group, cli_cmd("queue", cli_sim_debug_queue));
}

// -----[ _register_sim_event ]--------------------------------------
static void _register_sim_event(cli_cmd_t * parent)
{
  cli_cmd_t * cmd= cli_add_cmd(parent, cli_cmd("event", cli_sim_event));
  cli_add_arg(cmd, cli_arg("time", NULL));
  cli_add_arg(cmd, cli_arg("event", NULL));
}

// -----[ _register_sim_options ]------------------------------------
static void _register_sim_options(cli_cmd_t * parent)
{
  cli_cmd_t * group, * cmd;
  group= cli_add_cmd(parent, cli_cmd_group("options"));
  cmd= cli_add_cmd(group, cli_cmd("log-level", cli_sim_options_loglevel));
  cli_add_arg(cmd, cli_arg("level", NULL));
  cmd= cli_add_cmd(group, cli_cmd("scheduler", cli_sim_options_scheduler));
  cli_add_arg(cmd, cli_arg("scheduler", NULL));
}

// -----[ _register_sim_queue ]--------------------------------------
static void _register_sim_queue(cli_cmd_t * parent)
{
  cli_cmd_t * group, * cmd;
  group= cli_add_cmd(parent, cli_cmd_group("queue"));
  cmd= cli_add_cmd(group, cli_cmd("info", cli_sim_queue_info));
  cmd= cli_add_cmd(group, cli_cmd("log", cli_sim_queue_log));
  cli_add_arg(cmd, cli_arg_file("file", NULL));
  cmd= cli_add_cmd(group, cli_cmd("show", cli_sim_queue_show));
}

// -----[ _register_sim_run ]----------------------------------------
static void _register_sim_run(cli_cmd_t * parent)
{
  cli_add_cmd(parent, cli_cmd("run", cli_sim_run));
}

// -----[ _register_sim_step ]---------------------------------------
static void _register_sim_step(cli_cmd_t * parent)
{
  cli_cmd_t * cmd= cli_add_cmd(parent, cli_cmd("step", cli_sim_step));
  cli_add_arg(cmd, cli_arg("num-steps", NULL));
}

// -----[ _register_sim_stop ]---------------------------------------
static void _register_sim_stop(cli_cmd_t * parent)
{
  cli_cmd_t * cmd= cli_add_cmd(parent, cli_cmd("stop", cli_sim_stop));
  cli_add_opt(cmd, cli_opt("at=", NULL));
}

// ----- cli_register_sim -------------------------------------------
void cli_register_sim(cli_cmd_t * parent)
{
  cli_cmd_t * group= cli_add_cmd(parent, cli_cmd_group("sim"));
  _register_sim_clear(group);
  _register_sim_debug(group);
  _register_sim_event(group);
  _register_sim_options(group);
  _register_sim_queue(group);
  _register_sim_run(group);
  _register_sim_step(group);
  _register_sim_stop(group);
}

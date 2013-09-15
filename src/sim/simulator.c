// ==================================================================
// @(#)simulator.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 13/06/2003
// $Id: simulator.c,v 1.14 2009-03-09 16:44:52 bqu Exp $
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

#include <stdlib.h>
#include <stdio.h>

#include <libgds/memory.h>

#include <net/error.h>
#include <sim/simulator.h>
#include <sim/scheduler.h>
#include <sim/static_scheduler.h>

//#define DEBUG
#include <libgds/debug.h>

sched_type_t SIM_OPTIONS_SCHEDULER= SCHEDULER_STATIC;

// -----[ sim_sched_factory_f ]--------------------------------------
typedef sched_t * (*sim_sched_factory_f)(simulator_t * sim);

static struct {
  const char          * name;
  sim_sched_factory_f   factory;
} SCHEDULERS[SCHEDULER_MAX]= {
  { "static",  sched_static_create },
  { "dynamic", sched_dynamic_create },
};

// -----[ sim_create ]-----------------------------------------------
simulator_t * sim_create(sched_type_t type)
{
  simulator_t * sim;
  
  if (type >= SCHEDULER_MAX)
    cbgp_fatal("The requested scheduler type (%d) is not supported", type);
  
  sim= (simulator_t *) MALLOC(sizeof(simulator_t));
  sim->max_time= 0;
  sim->sched= SCHEDULERS[type].factory(sim);
  sim->running= 0;
  return sim;
}

// -----[ sim_destroy ]----------------------------------------------
void sim_destroy(simulator_t ** sim_ref)
{
  if (*sim_ref != NULL) {
    (*sim_ref)->sched->ops.destroy(&(*sim_ref)->sched);
    FREE(*sim_ref);
    *sim_ref= NULL;
  }
}

// -----[ sim_cancel ]-----------------------------------------------
void sim_cancel(simulator_t * sim)
{
  sim->sched->ops.cancel(sim->sched);
}

// -----[ sim_clear ]------------------------------------------------
void sim_clear(simulator_t * sim)
{
  sim->sched->ops.clear(sim->sched);
}

// -----[ sim_run ]--------------------------------------------------
int sim_run(simulator_t * sim)
{
  int result;
  sim->running= 1;
  result= sim->sched->ops.run(sim->sched, -1);
  sim->running= 0;
  return result;
}

// -----[ sim_is_running ]-----------------------------------------
int sim_is_running(simulator_t * sim)
{
  return sim->running;
}

// -----[ sim_step ]-------------------------------------------------
int sim_step(simulator_t * sim, unsigned int num_steps)
{
  return sim->sched->ops.run(sim->sched, num_steps);
}

// -----[ sim_get_num_events ]---------------------------------------
uint32_t sim_get_num_events(simulator_t * sim)
{
  return sim->sched->ops.num_events(sim->sched);
}

// -----[ sim_get_event ]--------------------------------------------
void * sim_get_event(simulator_t * sim, unsigned int index)
{
  return sim->sched->ops.event_at(sim->sched, index);
}

// -----[ sim_post_event ]-------------------------------------------
int sim_post_event(simulator_t * sim, sim_event_ops_t * ops,
		   void * ctx, double time, sim_time_t time_type)
{
  return sim->sched->ops.post(sim->sched, ops, ctx, time, time_type);
}

// -----[ sim_get_time ]---------------------------------------------
double sim_get_time(simulator_t * sim)
{
  return sim->sched->ops.cur_time(sim->sched);
}

// -----[ sim_set_max_time ]-----------------------------------------
void sim_set_max_time(simulator_t * sim, double max_time)
{
  sim->max_time= max_time;
}

// -----[ sim_dump_events ]------------------------------------------
void sim_dump_events(gds_stream_t * stream, simulator_t * sim)
{
  sim->sched->ops.dump_events(stream, sim->sched);
}

// -----[ sim_show_infos ]-------------------------------------------
void sim_show_infos(gds_stream_t * stream, simulator_t * sim)
{
  stream_printf(stream, "scheduler   : %s\n",
		sim_sched2str(sim->sched->type));
  stream_printf(stream, "current time: %f\n", sim_get_time(sim));
  stream_printf(stream, "maximum time: %f\n", sim->max_time);
}

// -----[ sim_set_log_progress ]-------------------------------------
void sim_set_log_progress(simulator_t * sim, const char * filename)
{
  sim->sched->ops.set_log_process(sim->sched, filename);
}

// -----[ sim_sched2str ]--------------------------------------------
const char * sim_sched2str(sched_type_t type)
{
  if (type >= SCHEDULER_MAX)
    return NULL;
  return SCHEDULERS[type].name;
}

// -----[ sim_str2sched ]--------------------------------------------
int sim_str2sched(const char * str, sched_type_t * type_ref)
{
  sched_type_t type;
  for (type= 0; type < SCHEDULER_MAX; type++)
    if (!strcmp(str, SCHEDULERS[type].name)) {
      *type_ref= type;
      return 0;
    }
  return -1;
}

static sched_type_t _default_sched= SCHEDULER_STATIC;

// -----[ sim_set_default_scheduler ]------------------------------
void sim_set_default_scheduler(sched_type_t type)
{
  _default_sched= type;
}

// -----[ sim_get_default_scheduler ]------------------------------
sched_type_t sim_get_default_scheduler()
{
  return _default_sched;
}


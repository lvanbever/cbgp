// ==================================================================
// @(#)simulator.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Sebastien Tandel
//
// @date 28/11/2002
// $Id: simulator.h,v 1.14 2009-03-09 16:44:52 bqu Exp $
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

/**
 * \file
 * Provide data structures and functions to manage a network
 * simulator.
 */

#ifndef __SIMULATOR_H__
#define __SIMULATOR_H__

#include <libgds/types.h>
#include <libgds/stream.h>
#include <net/error.h>

struct sched_t;
struct simulator_t;


// -----[ sim_time_t ]-----------------------------------------------
/** Possible simulation times. */
typedef enum {
  SIM_TIME_ABS,
  SIM_TIME_REL,
  SIM_TIME_MAX
} sim_time_t;


// -----[ sched_type_t ]---------------------------------------------
/** Possible scheduling techniques. */
typedef enum {
  SCHEDULER_STATIC,
  SCHEDULER_DYNAMIC,
  SCHEDULER_MAX
} sched_type_t;


// -----[ sim_event_ops_t ]------------------------------------------
/** Virtual methods of simulation events. */
typedef struct {
  int  (*callback) (struct simulator_t * sim, void * ctx);
  void (*destroy) (void * ctx);
  void (*dump)(gds_stream_t * stream, void * ctx);
} sim_event_ops_t;


// -----[ sched_ops_t ]----------------------------------------------
/** Virtual methods of schedulers. */
typedef struct sched_ops_t {
  void         (*destroy) (struct sched_t ** self_ref);
  void         (*cancel) (struct sched_t * self);
  void         (*clear) (struct sched_t * self);
  net_error_t  (*run) (struct sched_t * self, unsigned int num_steps);
  int          (*post) (struct sched_t * self, const sim_event_ops_t * ops,
			void * ctx, double time, sim_time_t time_type);
  unsigned int (*num_events) (struct sched_t * self);
  void *       (*event_at) (struct sched_t * self, unsigned int index);
  void         (*dump_events) (gds_stream_t * stream, struct sched_t * self);
  void         (*set_log_process) (struct sched_t * self,
				   const char * file_name);
  double       (*cur_time) (struct sched_t * self);
} sched_ops_t;


// -----[ sched_t ]--------------------------------------------------
/** Definition of an events scheduler. */
typedef struct sched_t {
  sched_type_t         type;
  sched_ops_t          ops;
  struct simulator_t * sim;
} sched_t;


// -----[ simulator_t ]----------------------------------------------
/** Definition of a simulator. */
typedef struct simulator_t {
  sched_t * sched;
  double    max_time;
  int       running;
} simulator_t;


#ifdef __cplusplus
extern "C" {
#endif

  // -----[ sim_create ]---------------------------------------------
  /**
   * Create a network simulator.
   *
   * \param type is the scheduling type.
   * \retval a simulator.
   */
  simulator_t * sim_create(sched_type_t type);

  // -----[ sim_destroy ]--------------------------------------------
  /**
   * Destroy a network simulator.
   *
   * \param sim_ref is a pointer to the simulator to be destroyed.
   */
  void sim_destroy(simulator_t ** sim_ref);

  // -----[ sim_cancel ]---------------------------------------------
  /**
   * Cancel the execution of a network simulator.
   *
   * \param sim_ref is a pointer to the simulator to cancel.
   */
  void sim_cancel(simulator_t * sim);

  // -----[ sim_clear ]----------------------------------------------
  void sim_clear(simulator_t * sim);

  // -----[ sim_run ]------------------------------------------------
  /**
   * Run the simulator until its events queue is empty.
   *
   * \param sim is the simulator.
   * \retval an error code.
   */
  int sim_run(simulator_t * sim);

  // -----[ sim_is_running ]-----------------------------------------
  /**
   * Test if the simulator is currently running.
   *
   * \param sim is the simulator.
   * \return 1 if the simulator is running, 0 otherwise.
   */
  int sim_is_running(simulator_t * sim);

  // -----[ sim_step ]-----------------------------------------------
  /**
   * Run the simulator with a limit on the number of events processed.
   *
   * \param sim is the simulator.
   * \param num_steps is a limit on the number of events to be
   *   processed.
   * \retval an error code.
   */
  int sim_step(simulator_t * sim, unsigned int num_steps);

  // -----[ sim_post_event ]-----------------------------------------
  /**
   * Schedule an event on a network simulator.
   *
   * \param sim is the simulator.
   * \param ops is the set of virtual methods of the event.
   * \param ctx is the event's context.
   * \param time_type tells if the event time is absolute or relative
   *   to the current simulation time.
   * \retval an error code.
   */
  int sim_post_event(simulator_t * sim, sim_event_ops_t * ops,
		     void * ctx, double time, sim_time_t time_type);

  // -----[ sim_get_num_events ]-------------------------------------
  /**
   * Get the number of events scheduled.
   *
   * \param sim is the simulator.
   * \retval the number of events.
   */
  uint32_t sim_get_num_events(simulator_t * sim);

  // -----[ sim_get_event ]------------------------------------------
  /**
   * Get an event from the simulator's queue.
   *
   * \param sim   is the simulator.
   * \param index is the position of the requested event.
   * \retval the requested event if it exists,
   *   or NULL otherwise.
   */
  void * sim_get_event(simulator_t * sim, unsigned int index);

  // -----[ sim_get_time ]-------------------------------------------
  /**
   * Get the current simulation time.
   *
   * \param sim is the simulator.
   * \retval the current simulation time.
   */
  double sim_get_time(simulator_t * sim);

  // -----[ sim_set_max_time ]---------------------------------------
  /**
   * Set the simulation time limit.
   *
   * If the specified limit is reached, the processing of events is
   * interrupted. Note that the simulation time has different
   * meanings for static and dynamic schedulers.
   *
   * \param sim      is the simulator.
   * \param max_time is the simulation time limit.
   */
  void sim_set_max_time(simulator_t * sim, double max_time);

  // -----[ sim_dump_events ]----------------------------------------
  void sim_dump_events(gds_stream_t * stream, simulator_t * sim);

  // -----[ sim_show_infos ]-----------------------------------------
  void sim_show_infos(gds_stream_t * stream, simulator_t * sim);

  // -----[ sim_set_log_progress ]-----------------------------------
  void sim_set_log_progress(simulator_t * sim, const char * file_name);

  // -----[ sim_sched2str ]------------------------------------------
  /**
   * Convert a scheduler type to a name.
   *
   * \param type is the scheduler type.
   * \retval the scheduler's name,
   *   or NULL if the scheduler type does not exist.
   */
  const char * sim_sched2str(sched_type_t type);

  // -----[ sim_str2sched ]------------------------------------------
  /**
   * Convert a scheduler name to a type.
   *
   * \param str      is the scheduler name.
   * \param type_ref is a pointer to the returned type.
   * \retval 0 if the scheduler's name exists,
   *   or <0 if it does not exist.
   */
  int sim_str2sched(const char * str, sched_type_t * type_ref);

  // -----[ sim_set_default_scheduler ]------------------------------
  /**
   * Set the default scheduler type.
   */
  void sim_set_default_scheduler(sched_type_t type);

  // -----[ sim_get_default_scheduler ]------------------------------
  /**
   * Get the default scheduler type.
   */
  sched_type_t sim_get_default_scheduler();

#ifdef __cplusplus
}
#endif

#endif /* __SIMULATOR_H__ */


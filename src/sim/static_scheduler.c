// ==================================================================
// @(#)static_scheduler.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 30/07/2003
// $Id: static_scheduler.c,v 1.16 2009-03-10 13:13:25 bqu Exp $
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
#include <sys/time.h>

#include <libgds/fifo.h>
#include <libgds/stream.h>
#include <libgds/memory.h>
#include <sim/static_scheduler.h>
#include <net/network.h>

#define EVENT_QUEUE_DEPTH 256

typedef struct {
  const sim_event_ops_t * ops;
  void                  * ctx;
} _event_t;

typedef struct {
  sched_type_t   type;
  sched_ops_t    ops;
  simulator_t  * sim;
  gds_fifo_t   * events;
  unsigned int   cur_time;
  gds_stream_t * pProgressLogStream;
  volatile int   cancelled;
} sched_static_t;

// -----[ _event_create ]--------------------------------------------
static inline _event_t * _event_create(const sim_event_ops_t * ops,
				       void * ctx)
{
  _event_t * event= (_event_t *) MALLOC(sizeof(_event_t));
  event->ops= ops;
  event->ctx= ctx;
  return event;
}

// -----[ _event_destroy ]-------------------------------------------
static void _event_destroy(_event_t ** event_ref)
{
  if (*event_ref != NULL) {
    FREE(*event_ref);
    *event_ref= NULL;
  }
}

// -----[ _fifo_event_destroy ]--------------------------------------
static void _fifo_event_destroy(void ** item_ref)
{
  _event_t ** event_ref= (_event_t **) item_ref;
  _event_t * event= *event_ref;

  if (event == NULL)
    return;
  
  if ((event->ops != NULL) && (event->ops->destroy != NULL))
    event->ops->destroy(event->ctx);
  _event_destroy(event_ref);
}

// -----[ _log_progress ]--------------------------------------------
static inline void _log_progress(sched_static_t * sched)
{
  struct timeval tv;

  if (sched->pProgressLogStream == NULL)
    return;

  assert(gettimeofday(&tv, NULL) >= 0);
  stream_printf(sched->pProgressLogStream, "%d\t%.0f\t%d\n",
		sched->cur_time,
		((double) tv.tv_sec)*1000000 + (double) tv.tv_usec,
		fifo_depth(sched->events));
}

// -----[ _destroy ]-------------------------------------------------
static void _destroy(sched_t ** sched_ref)
{
  sched_static_t * sched;
  if (*sched_ref != NULL) {

    sched= (sched_static_t *) *sched_ref;

    // Free private part
    if (sched->events->current_depth > 0)
      cbgp_warn("%d event%s still in simulation queue.\n",
		sched->events->current_depth,
		(sched->events->current_depth>1?"s":""));
    fifo_destroy(&sched->events);
    if (sched->pProgressLogStream != NULL)
      stream_destroy(&sched->pProgressLogStream);

    FREE(sched);
    *sched_ref= NULL;
  }
}

// -----[ _cancel ]--------------------------------------------------
static void _cancel(sched_t * self)
{
  sched_static_t * sched= (sched_static_t *) self;
  sched->cancelled= 1;
}

// -----[ _clear ]---------------------------------------------------
static void _clear(sched_t * self)
{
  sched_static_t * sched= (sched_static_t *) self;
  fifo_destroy(&sched->events);
  sched->events= fifo_create(EVENT_QUEUE_DEPTH, _fifo_event_destroy);
  fifo_set_option(sched->events, FIFO_OPTION_GROW_EXPONENTIAL, 1);
}

// -----[ _run ]-----------------------------------------------------
/**
 * Process the events in the global linear queue.
 *
 * Arguments:
 * - iNumSteps: number of events to process during this run (if -1,
 *   process events until queue is empty).
 */
static net_error_t _run(sched_t * self, unsigned int num_steps)
{
  sched_static_t * sched= (sched_static_t *) self;
  _event_t * event;
  net_error_t error;

  sched->cancelled= 0;

  while ((!sched->cancelled) &&
	 ((event= (_event_t *) fifo_pop(sched->events)) != NULL)) {

    STREAM_DEBUG(STREAM_LEVEL_DEBUG, "=====<<< EVENT %2.2f >>>=====\n",
	      (double) sched->cur_time);
    error= event->ops->callback(sched->sim, event->ctx);
    _event_destroy(&event);
    if (error != ESUCCESS)
      return error;
    STREAM_DEBUG(STREAM_LEVEL_DEBUG, "\n");

    // Update simulation time
    sched->cur_time++;

    // Limit on simulation time ??
    if ((sched->sim->max_time > 0) &&
	(sched->cur_time >= sched->sim->max_time))
      return ESIM_TIME_LIMIT;

    // Limit on number of steps
    if (num_steps > 0) {
      num_steps--;
      if (num_steps == 0)
	break;
    }

    _log_progress(sched);
  }
  return ESUCCESS;
}

// -----[ _num_events ]----------------------------------------------
/**
 * Return the number of queued events.
 */
static unsigned int _num_events(sched_t * self)
{
  sched_static_t * sched= (sched_static_t *) self;
  return sched->events->current_depth;
}

// -----[ _event_at ]------------------------------------------------
void * _event_at(sched_t * self, unsigned int index)
{
  sched_static_t * sched= (sched_static_t *) self;
  uint32_t depth;
  uint32_t max_depth;
  uint32_t start;

  depth= sched->events->current_depth;
  if (index >= depth)
    return NULL;

  max_depth= sched->events->max_depth;
  start= sched->events->start_index;
  return ((_event_t *) sched->events->items[(start+index) % max_depth])->ctx;

}

// -----[ _post ]----------------------------------------------------
static int _post(sched_t * self, const sim_event_ops_t * ops,
		 void * ctx, double time, sim_time_t time_type)
{
  sched_static_t * sched= (sched_static_t *) self;
  _event_t * event= _event_create(ops, ctx);
  return fifo_push(sched->events, event);
}

// -----[ _dump_events ]---------------------------------------------
/**
 * Return information 
 */
static void _dump_events(gds_stream_t * stream, sched_t * self)
{
  sched_static_t * sched= (sched_static_t *) self;
  _event_t * event;
  uint32_t depth;
  uint32_t max_depth;
  uint32_t start;
  unsigned int index;

  depth= sched->events->current_depth;
  max_depth= sched->events->max_depth;
  start= sched->events->start_index;
  stream_printf(stream, "Number of events queued: %u (%u)\n",
	     depth, max_depth);
  for (index= 0; index < depth; index++) {
    event= (_event_t *) sched->events->items[(start+index) % max_depth];
    stream_printf(stream, "(%d) ", (start+index) % max_depth);
    stream_flush(stream);
    if (event->ops->dump != NULL) {
      event->ops->dump(stream, event->ctx);
    } else {
      stream_printf(stream, "unknown");
    }
    stream_printf(stream, "\n");
  }
}

// -----[ _set_log_progress ]----------------------------------------
/**
 *
 */
static void _set_log_progress(sched_t * self, const char * filename)
{
  sched_static_t * sched= (sched_static_t *) self;

  if (sched->pProgressLogStream != NULL)
    stream_destroy(&sched->pProgressLogStream);

  if (filename != NULL) {
    sched->pProgressLogStream= stream_create_file(filename);
    stream_printf(sched->pProgressLogStream, "# C-BGP Queue Progress\n");
    stream_printf(sched->pProgressLogStream, "# <step> <time (us)> <depth>\n");
  }
}

// -----[ _cur_time ]------------------------------------------------
static double _cur_time(sched_t * self)
{
  sched_static_t * sched= (sched_static_t *) self;
  return (double) sched->cur_time;
}

// -----[ static_scheduler_create ]---------------------------------
/**
 *
 */
sched_t * sched_static_create(simulator_t * sim)
{
  sched_static_t * sched=
    (sched_static_t *) MALLOC(sizeof(sched_static_t));

  // Initialize public part (type + ops)
  sched->type= SCHEDULER_STATIC;
  sched->sim= sim;
  sched->ops.destroy        = _destroy;
  sched->ops.cancel         = _cancel;
  sched->ops.clear          = _clear;
  sched->ops.run            = _run;
  sched->ops.post           = _post;
  sched->ops.num_events     = _num_events;
  sched->ops.event_at       = _event_at;
  sched->ops.dump_events    = _dump_events;
  sched->ops.set_log_process= _set_log_progress;
  sched->ops.cur_time       = _cur_time;

  // Initialize private part
  sched->events= fifo_create(EVENT_QUEUE_DEPTH, _fifo_event_destroy);
  fifo_set_option(sched->events, FIFO_OPTION_GROW_EXPONENTIAL, 1);
  sched->cur_time= 0;
  sched->pProgressLogStream= NULL;

  return (sched_t *) sched;
}

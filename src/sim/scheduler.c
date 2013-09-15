// ==================================================================
// @(#)scheduler.c
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Sebastien Tandel (standel@info.ucl.ac.be)
// @date 12/06/2003
// $Id: scheduler.c,v 1.7 2009-03-10 13:13:25 bqu Exp $
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

#include <libgds/fifo.h>
#include <libgds/list.h>
#include <libgds/memory.h>
#include <libgds/stream.h>

#include <sim/scheduler.h>

//#define DEBUG
#include <libgds/debug.h>

#define NUM_BUCKETS       50
#define EVENT_QUEUE_DEPTH 40000000

typedef struct {
  const sim_event_ops_t * ops;
  void                  * ctx;
} _event_t;

typedef struct {
  double       time;
  gds_fifo_t * events;
} _bucket_t;

typedef struct {
  sched_type_t   type;
  sched_ops_t    ops;
  simulator_t  * sim;
  gds_list_t   * buckets;
  float          cur_time;
  gds_stream_t * pProgressLogStream;
  volatile int   cancelled;
} sched_dynamic_t;

// -----[ _event_create ]--------------------------------------------
static inline _event_t * _event_create(const sim_event_ops_t * ops,
				       void * ctx)
{
  _event_t * ev= (_event_t *) MALLOC(sizeof(_event_t));
  ev->ops= ops;
  ev->ctx= ctx;
  return ev;
}

// -----[ _event_destroy ]-------------------------------------------
static inline void _event_destroy(_event_t ** event_ref)
{
  _event_t * event= *event_ref;
  if (event == NULL)
    return;
  FREE(event);
  *event_ref= NULL;
}

// -----[ _bucket_item_destroy ]-------------------------------------
static void _bucket_item_destroy(void ** item_ref)
{
  _event_t ** event_ref= (_event_t **) item_ref;
  _event_t * event= *event_ref;
  if (event == NULL)
    return;
  if ((event->ops != NULL) && (event->ops->destroy != NULL))
    event->ops->destroy(event->ctx);
  _event_destroy(event_ref);
}

// -----[ _bucket_create ]-------------------------------------------
static inline _bucket_t * _bucket_create(double time)
{
  _bucket_t * bucket= MALLOC(sizeof(_bucket_t));
  bucket->events= fifo_create(EVENT_QUEUE_DEPTH, _bucket_item_destroy);
  bucket->time= time;
  return bucket;
}

// -----[ _bucket_destroy ]------------------------------------------
static inline void _bucket_destroy(_bucket_t ** bucket_ref)
{
  _bucket_t * bucket= *bucket_ref;
  if (bucket == NULL)
    return;
  fifo_destroy(&bucket->events);
  FREE(bucket);
  *bucket_ref= NULL;
}

// -----[ _bucket_push ]---------------------------------------------
static inline void _bucket_push(_bucket_t * bucket, _event_t * event)
{
  assert(fifo_push(bucket->events, event) >= 0);
}

// -----[ _bucket_list_item_cmp ]------------------------------------
static int _bucket_list_item_cmp(const void * item1,
				 const void * item2)
{
  _bucket_t * bucket1= (_bucket_t *) item1;
  _bucket_t * bucket2= (_bucket_t *) item2;

  if (bucket2->time < bucket1->time) 
    return 1;
  if (bucket2->time > bucket1->time) 
    return -1;
  return 0;
}

// -----[ _bucket_list_item_destroy ]--------------------------------
static void _bucket_list_item_destroy(void ** item_ref)
{
  _bucket_destroy((_bucket_t **) item_ref);
}

// -----[ _destroy ]-------------------------------------------------
static void _destroy(sched_t ** self_ref)
{
  sched_dynamic_t * sched= *((sched_dynamic_t **) self_ref);
  if (sched == NULL)
    return;
  list_destroy(&sched->buckets);
  FREE(sched);
  self_ref= NULL;
}

// -----[ _cancel ]--------------------------------------------------
static void _cancel(sched_t * self)
{
  sched_dynamic_t * sched= (sched_dynamic_t *) self;
  sched->cancelled= 1;
}

// -----[ _clear ]---------------------------------------------------
static void _clear(sched_t * self)
{
  sched_dynamic_t * sched= (sched_dynamic_t *) self;
  list_destroy(&sched->buckets);
  sched->buckets= list_create(_bucket_list_item_cmp,
			      _bucket_list_item_destroy,
			      NUM_BUCKETS);
}

// -----[ _run ]-----------------------------------------------------
static net_error_t _run(sched_t * self, unsigned int num_steps)
{
  sched_dynamic_t * sched= (sched_dynamic_t *) self;
  _bucket_t * bucket;
  _event_t * event;

  __debug("sched_dynamic::_run(%p)\n", sched);

  while (list_length(sched->buckets) > 0) {
    bucket= (_bucket_t *) list_get_at(sched->buckets, 0);
    __debug("  +-- bucket-time: %f\n", bucket->time);

    sched->cur_time= bucket->time;

    // Limit on simulation time ??
    if ((sched->sim->max_time > 0) &&
	(sched->cur_time >= sched->sim->max_time))
      return ESIM_TIME_LIMIT;

    while (fifo_depth(bucket->events) > 0) {

      event= (_event_t *) fifo_pop(bucket->events);
      if (event->ops->callback == NULL)
	cbgp_fatal("event callback is NULL");
      __debug("event_run()\n"
	      "  +-- ctx: \"");
      /*if (event->ops->dump != NULL)
	event->ops->dump(gdserr, event->ctx);
      else
      __debug("???");*/
      __debug("\"\n");
      event->ops->callback(sched->sim, event->ctx);
      _event_destroy(&event);

      // Limit on number of steps
      if (num_steps > 0) {
	num_steps--;
	if (num_steps == 0)
	  return ESUCCESS;
      }
    }

    list_remove_at(sched->buckets, 0);
  }
  return 0;
}

// -----[ _post ]----------------------------------------------------
static int _post(sched_t * self, const sim_event_ops_t * ops,
		 void * ctx, double time, sim_time_t time_type)
{
  sched_dynamic_t * sched= (sched_dynamic_t *) self;
  _bucket_t bucket= { };
  _bucket_t * bucket_ptr;
  unsigned int index;

  if (time_type == SIM_TIME_REL)
    time= sched->cur_time+time;

  __debug("sched_dynamic::_post(%p)\n"
	  "  +-- time: %f\n"
	  "  +-- cur : %f\n", sched, time, sched->cur_time);

  bucket.time= time;

  if ((time < 0) ||
      ((time_type == SIM_TIME_ABS) &&
       (time < sched->cur_time)))
    cbgp_fatal("impossible to schedule events in the past");

  if (list_index_of(sched->buckets, &bucket, &index) < 0) {
    bucket_ptr= _bucket_create(time);
    if (list_insert_at(sched->buckets, index, bucket_ptr) < 0)
      return -1;
  } else {
    bucket_ptr= (_bucket_t *) list_get_at(sched->buckets, index);
  }
  _bucket_push(bucket_ptr, _event_create(ops, ctx));
  return 0;
}

// -----[ _num_events ]----------------------------------------------
static unsigned int _num_events(sched_t * self)
{
  sched_dynamic_t * sched= (sched_dynamic_t *) self;
  unsigned int num_events= 0;
  unsigned int index;
  _bucket_t * bucket;

  for (index= 0; index < list_length(sched->buckets); index++) {
    bucket= (_bucket_t *) list_get_at(sched->buckets, index);
    num_events+= fifo_depth(bucket->events);
  }
  return num_events;
}

// -----[ _event_at ]------------------------------------------------
static void * _event_at(sched_t * self, unsigned int index)
{
  sched_dynamic_t * sched= (sched_dynamic_t *) self;
  unsigned int index2;
  _event_t * event= NULL;
    _bucket_t * bucket;

  for (index2= 0; index2 < list_length(sched->buckets); index2++) {
    bucket= (_bucket_t *) list_get_at(sched->buckets, index2);
    if (index < fifo_depth(bucket->events)) {
      event= (_event_t *) fifo_get_at(bucket->events, index);
      break;
    }
    index-= fifo_depth(bucket->events);
  }
  return event;
}

// -----[ _dump_events ]---------------------------------------------
static void _dump_events(gds_stream_t * stream, sched_t * self)
{
}

// -----[ _set_log_progress ]----------------------------------------
static void _set_log_progress(sched_t * self, const char * filename)
{
}

// -----[ _cur_time ]------------------------------------------------
static double _cur_time(sched_t * self)
{
  sched_dynamic_t * sched= (sched_dynamic_t *) self;
  return sched->cur_time;
}

// -----[ sched_dynamic_create ]-------------------------------------
sched_t * sched_dynamic_create(simulator_t * sim)
{
  sched_dynamic_t * sched=
    (sched_dynamic_t *) MALLOC(sizeof(sched_dynamic_t));

  // Initialize public part (type + ops)
  sched->type= SCHEDULER_DYNAMIC;
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
  sched->buckets= list_create(_bucket_list_item_cmp,
			      _bucket_list_item_destroy,
			      NUM_BUCKETS);
  sched->cur_time= 0;
  sched->pProgressLogStream= NULL;
  sched->cancelled= 0;

  return (sched_t *) sched;
}

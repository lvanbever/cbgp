// ==================================================================
// @(#)static_scheduler.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 30/07/2003
// $Id: static_scheduler.h,v 1.12 2009-03-10 13:13:25 bqu Exp $
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
 * Provide data structures and functions to handle a static
 * scheduler. The static scheduler is basically a single
 * linear queue. New events are inserted at the end of the queue
 * while events to be proccessed are found at the beginning of the
 * queue (FIFO discipline).
 *
 * Insertion and extraction times are obviously O(1) in theory.
 * In practice, memory management activities can make the insertion
 * time longer, especially when the scheduler's queue must be
 * expanded.
 *
 * In addition, there is no real notion of time in a static
 * scheduler. In this implementation, the simulation time corresponds
 * to the index of the last processed event.
 */

#ifndef __STATIC_SCHEDULER_H__
#define __STATIC_SCHEDULER_H__

#include <sim/simulator.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ sched_static_create ]------------------------------------
  /**
   * Create a static scheduler instance.
   *
   * \param sim is the parent simulator.
   * \retval a static scheduler.
   */
  sched_t * sched_static_create(simulator_t * sim);

#ifdef __cplusplus
}
#endif

#endif /* __STATIC_SCHEDULER_H__ */

// ==================================================================
// @(#)scheduler.h
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @author Sebastien Tandel (sta@infonet.fundp.ac.be)
// @date 12/06/2003
// $Id: scheduler.h,v 1.3 2009-03-10 13:13:25 bqu Exp $
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
 * Provide data structures and functions to handle a dynamic
 * scheduler. This is a highly experimental and inefficient
 * implementation.
 */

#ifndef __SCHEDULER_H__
#define __SCHEDULER_H__

#include <sim/simulator.h>

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ sched_dynamic_create ]-----------------------------------
  /**
   * Create a dynamic scheduler instance.
   *
   * \param sim is the parent simulator.
   * \retval a dynamic scheduler.
   */
  sched_t * sched_dynamic_create(simulator_t * sim);

#ifdef __cplusplus
}
#endif

#endif /* __SCHEDULER_H__ */

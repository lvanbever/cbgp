// ==================================================================
// @(#)pager.h
//
// @author Bruno Quoitin (bruno.quoitin@umons.ac.be)
// @date 30/11/2007
// $Id: pager.h,v 1.2 2009-03-24 16:29:41 bqu Exp $
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

#ifndef __UI_PAGER_H__
#define __UI_PAGER_H__

// Note: the -R option makes "less" process ANSI escape sequences
//       (e.g. used to highlight text and change cursor position)
#define PAGER_CMD "less"
#define PAGER_ARGS "-R"

#define PAGER_SUCCESS 0

#ifdef __cplusplus
extern "C" {
#endif

  // -----[ pager_run ]----------------------------------------------
  int pager_run(const char * filename);

#ifdef __cplusplus
}
#endif

#endif /* __UI_PAGER_H__ */

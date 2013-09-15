// ==================================================================
// @(#)main-test.c
//
// Main source file for cbgp-test application.
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 03/04/08
// $Id: main-test.c,v 1.16 2009-03-10 13:57:49 bqu Exp $
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

#include <api.h>
#include <selfcheck.h>

// -----[ main ]-----------------------------------------------------
int main(int argc, char * argv[])
{
  int result;

  libcbgp_init(argc, argv);
  libcbgp_banner();
  result= libcbgp_selfcheck();
  libcbgp_done();

  return (result==0?EXIT_SUCCESS:EXIT_FAILURE);
}

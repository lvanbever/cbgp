// ==================================================================
// @(#)pager.c
//
// @author Bruno Quoitin (bruno.quoitin@umons.ac.be)
// @date 30/11/2007
// $Id: pager.c,v 1.2 2009-03-24 16:29:41 bqu Exp $
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

#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <ui/pager.h>

// -----[ pager_run ]------------------------------------------------
int pager_run(const char * filename) {
  char * const args[]= {
    (char *) PAGER_CMD,
    (char *) PAGER_ARGS,
    (char *) filename,
    NULL
  };
  int result;
  int status;
  struct stat sb;

  // Check that file exists
  if (stat(filename, &sb) < 0)
    return -1;

  result= fork();
  if (result < 0)
    return -1;

  // Parent process (wait until child is terminated)
  if (result > 0) {
    waitpid(result, &status, 0);
    return PAGER_SUCCESS;
  }

  // Child process (pager)
  result= execvp(PAGER_CMD, args);
  if (result < 0)
    exit(EXIT_FAILURE);

  exit(EXIT_SUCCESS);
}

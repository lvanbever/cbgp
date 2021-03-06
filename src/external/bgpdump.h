/* $Id: bgpdump.h,v 1.3 2008-01-25 11:27:33 bqu Exp $ */
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
/*

Copyright (c) 2007                      RIPE NCC


All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted, provided
that the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation, and that the name of the author not be used in advertising or
publicity pertaining to distribution of the software without specific,
written prior permission.

THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS; IN NO EVENT SHALL
AUTHOR BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 

*/

/* 

Parts of this code have been engineered after analiyzing GNU Zebra's
source code and therefore might contain declarations/code from GNU
Zebra, Copyright (C) 1999 Kunihiro Ishiguro. Zebra is a free routing
software, distributed under the GNU General Public License. A copy of
this license is included with libbgpdump.

*/


/*
-------------------------------------------------------------------------------
Original Author: Dan Ardelean (dan@ripe.net)
*/

#ifndef _BGPDUMP_H
#define _BGPDUMP_H

#include <config.h>

#include <sys/types.h>
#include <netinet/in.h>

typedef u_int32_t as_t;

typedef union union_BGPDUMP_IP_ADDRESS {
    struct in_addr	v4_addr;
#ifdef BGPDUMP_HAVE_IPV6
    struct in6_addr	v6_addr;
#endif
} BGPDUMP_IP_ADDRESS;

/* cygwin doesn't define INET[6]_ADDRSTRLEN */
#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif
#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

#ifdef BGPDUMP_HAVE_IPV6
#define BGPDUMP_ADDRSTRLEN INET6_ADDRSTRLEN
#else
#define BGPDUMP_ADDRSTRLEN INET_ADDRSTRLEN
#endif

#define ASN16_LEN sizeof(u_int16_t)
#define ASN32_LEN sizeof(u_int32_t)

#define AS_TRAN 23456

#endif

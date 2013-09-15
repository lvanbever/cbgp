// ==================================================================
// @(#)util.c
//
// Various network utility function: conversion from string to
// address and so on...
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 23/01/2007
// $Id: util.c,v 1.2 2009-03-24 16:28:39 bqu Exp $
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
#include <config.h>
#endif

#include <libgds/str_util.h>

#include <net/link.h>
#include <net/prefix.h>
#include <net/util.h>

// -----[ str2address ]----------------------------------------------
int str2address(const char * str, net_addr_t * addr)
{
  char * end_ptr;

  if (ip_string_to_address(str, &end_ptr, addr) ||
      (*end_ptr != 0))
    return -1;
  return 0;
}

// -----[ str2prefix ]-----------------------------------------------
int str2prefix(const char * str, ip_pfx_t * prefix)
{
  char * end_ptr;

  if (ip_string_to_prefix(str, &end_ptr, prefix) ||
      (*end_ptr != 0))
    return -1;
  return 0;
}

// -----[ str2tos ]--------------------------------------------------
int str2tos(const char * str, net_tos_t * tos)
{
  unsigned int u_tos;

  if (str_as_uint(str, &u_tos) < 0)
    return -1;

  *tos= (net_tos_t) u_tos;
  return 0;
}

// -----[ str2depth ]------------------------------------------------
int str2depth(const char * str, uint8_t * depth)
{
  unsigned int u_depth;

  if (str_as_uint(str, &u_depth) ||
      (u_depth > NET_LINK_MAX_DEPTH))
    return -1;
  
  *depth= (uint8_t) u_depth;
  return 0;
}

// -----[ str2capacity ]---------------------------------------------
int str2capacity(const char * str, net_link_load_t * capacity)
{
  unsigned int u_capacity;

  if (str_as_uint(str, &u_capacity))
    return -1;
  *capacity= (net_link_load_t) u_capacity;
  return 0;
}

// -----[ str2delay ]------------------------------------------------
int str2delay(const char * str, net_link_delay_t * delay)
{
  unsigned int u_delay;

  if (str_as_uint(str, &u_delay))
    return -1;
  *delay= (net_link_delay_t) u_delay;
  return 0;
}

// -----[ str2weight ]-----------------------------------------------
int str2weight(const char * str, igp_weight_t * weight)
{
  unsigned int u_weight;

  if (str_as_uint(str, &u_weight))
    return -1;
  *weight= (igp_weight_t) u_weight;
  return 0;
}

// -----[ str2domain_id ]--------------------------------------------
int str2domain_id(const char * str, unsigned int * id)
{
  if (str_as_uint(str, id) ||
      (*id > 65535))
    return -1;
  return 0;
}

// -----[ str2ttl ]--------------------------------------------------
int str2ttl(const char * str, uint8_t * ttl)
{
  unsigned int u_ttl;

  if (str_as_uint(str, &u_ttl) ||
      (u_ttl > 255))
    return -1;
  *ttl= (uint8_t) u_ttl;
  return 0;
}

// -----[ str2asn ]--------------------------------------------------
int str2asn(const char * str, asn_t * asn)
{
  unsigned int u_asn;

  if (str_as_uint(str, &u_asn) ||
      (u_asn > MAX_AS))
    return -1;
  *asn= (asn_t) u_asn;
  return 0;
}

// ==================================================================
// @(#)IPTraceElement.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// @date 18/02/2004
// $Id: IPTraceElement.java,v 1.3 2009-08-31 09:40:35 bqu Exp $
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

package be.ac.ucl.ingi.cbgp;

import be.ac.ucl.ingi.cbgp.net.Element;

/**
 * This class represents an element of an IP trace. These elements
 * can be Nodes or Subnets.
 * 
 * @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
 */
public class IPTraceElement {

	protected int hop_count;
	protected final Element element;
	
	// -----[ constructor ]-----------------------------------------
	protected IPTraceElement(Element element) {
		this.element= element;
	}
	
	// -----[ getElement ]------------------------------------------
	public Element getElement() {
		return this.element;
	}
	
	// -----[ getHopCount ]-----------------------------------------
	public int getHopCount() {
		return hop_count;
	}
	
}

// ==================================================================
// @(#)TestTraceRoute.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// $Id: TestTraceRoute.java,v 1.2 2009-08-31 09:46:14 bqu Exp $
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

package be.ac.ucl.ingi.cbgp.testing;

import static org.junit.Assert.*;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import be.ac.ucl.ingi.cbgp.CBGP;
import be.ac.ucl.ingi.cbgp.IPTrace;
import be.ac.ucl.ingi.cbgp.exceptions.CBGPException;
import be.ac.ucl.ingi.cbgp.net.IGPDomain;
import be.ac.ucl.ingi.cbgp.net.Node;

public class TestTraceRoute {
	
	protected CBGP cbgp;
	protected IGPDomain domain;
	protected Node node1, node2, node3;
	
	@Before
	public void setUp() throws CBGPException {
		cbgp= new CBGP();
		cbgp.init(null);
		domain= cbgp.netAddDomain(1);
		node1= domain.addNode("1.0.0.1");
		node2= domain.addNode("1.0.0.2");
		node3= domain.addNode("1.0.0.3");
		node1.addLTLLink(node2, true).setWeight(1);
		node2.addLTLLink(node3, true).setWeight(2);
		domain.compute();
	}
	
	@After
	public void tearDown() throws CBGPException {
		cbgp.destroy();
	}
	
	@Test
	public void testBasic() throws CBGPException {
		IPTrace trace= node1.traceRoute("1.0.0.2");
		assertNotNull(trace);
		assertEquals(IPTrace.IP_TRACE_SUCCESS, trace.getStatus());
		assertEquals((int) 2, trace.getElementsCount());
		trace= node1.traceRoute("1.0.0.3");
		assertNotNull(trace);
		assertEquals(IPTrace.IP_TRACE_SUCCESS, trace.getStatus());
		assertEquals((int) 3, trace.getElementsCount());
	}
	
}

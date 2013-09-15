// ==================================================================
// @(#)TestLink.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// $Id: TestLink.java,v 1.2 2009-08-31 09:46:14 bqu Exp $
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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import be.ac.ucl.ingi.cbgp.CBGP;
import be.ac.ucl.ingi.cbgp.exceptions.CBGPException;
import be.ac.ucl.ingi.cbgp.net.IGPDomain;
import be.ac.ucl.ingi.cbgp.net.Interface;
import be.ac.ucl.ingi.cbgp.net.Node;
import be.ac.ucl.ingi.cbgp.net.Subnet;

public class TestLink {

	CBGP cbgp;
	
	protected IGPDomain domain;
	protected Node node1;
	protected Node node2;
	protected Subnet subnet;
	
	@Before
	public void setUp() {
		cbgp= new CBGP();
		try {
			cbgp.init(null);
			domain= cbgp.netAddDomain(1);
			node1= domain.addNode("1.0.0.1");
			node2= domain.addNode("1.0.0.2");
			subnet= cbgp.netAddSubnet("192.168.1/24", 0);
		} catch (CBGPException e) {
			fail();
		}
	}
	
	@After
	public void tearDown() {
		try {
			cbgp.destroy();
			cbgp= null;
		} catch (CBGPException e) {
			fail();
		}
	}
	
	@Test
	public void testRTR() throws CBGPException {
		Interface link12= node1.addLTLLink(node2, true);
		assert(link12.getPrefix().address.toString().equals("1.0.0.2"));
		assertEquals(link12.getHead(), node1);
		assertEquals(link12.getTail(), node2);
		assertEquals(link12.getCapacity(), (long) 0);
		assertEquals(link12.getDelay(), (long) 0);
		assertEquals(link12.getWeight(), Interface.MAX_METRIC);
		assertEquals(link12.getLoad(), (long) 0);
		assertEquals(link12.getState(), true);
	}
	
	@Test
	public void testPTP() throws CBGPException {
		Interface link12= node1.addPTPLink(node2, "192.168.0.1/30", "192.168.0.2/30", true);
		assert(link12.getPrefix().address.toString().equals("192.168.0.1"));
		assertEquals(link12.getHead(), node1);
		assertEquals(link12.getTail(), node2);
		assertEquals(link12.getCapacity(), (long) 0);
		assertEquals(link12.getDelay(), (long) 0);
		assertEquals(link12.getWeight(), Interface.MAX_METRIC);
		assertEquals(link12.getLoad(), (long) 0);
		assertEquals(link12.getState(), true);
	}
	
	@Test
	public void testPTMP() throws CBGPException {
		Interface link12= node1.addPTMPLink(subnet, "192.168.1.1");
		assert(link12.getPrefix().address.toString().equals("192.168.1.1"));
		assertEquals(link12.getHead(), node1);
		assertEquals(link12.getTail(), subnet);
		assertEquals(link12.getCapacity(), (long) 0);
		assertEquals(link12.getDelay(), (long) 0);
		assertEquals(link12.getWeight(), Interface.MAX_METRIC);
		assertEquals(link12.getLoad(), (long) 0);
		assertEquals(link12.getState(), true);
	}
	
	@Test
	public void testCapacity() throws CBGPException {
		Interface link12= node1.addLTLLink(node2, false);
		assertEquals(link12.getCapacity(), (long) 0);
		final long DEFAULT_CAPACITY= 1000000; 
		link12.setCapacity(DEFAULT_CAPACITY);
		assertEquals(link12.getCapacity(), DEFAULT_CAPACITY);
	}
	
}

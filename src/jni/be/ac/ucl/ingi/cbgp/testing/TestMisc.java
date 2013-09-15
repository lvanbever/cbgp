// ==================================================================
// @(#)TestMisc.java
//
// @author Bruno Quoitin (bruno.quoitin@uclouvain.be)
// $Id: TestMisc.java,v 1.2 2009-08-31 09:46:14 bqu Exp $
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
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.util.Iterator;
import java.util.Vector;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import be.ac.ucl.ingi.cbgp.CBGP;
import be.ac.ucl.ingi.cbgp.exceptions.CBGPException;
import be.ac.ucl.ingi.cbgp.net.IGPDomain;
import be.ac.ucl.ingi.cbgp.net.Interface;
import be.ac.ucl.ingi.cbgp.net.Node;

public class TestMisc {
	
	CBGP cbgp;
	
	@Before
	public void setUp() {
		cbgp= new CBGP();
		try {
			cbgp.init(null);
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
	public void testLink() {
		Node node1= null;
		try {
			cbgp.netAddDomain(0);
			node1= cbgp.netAddNode("1.0.0.1", 0);
			assertNotNull(node1);
			Node node2= cbgp.netAddNode("1.0.0.2", 0);
			assertNotNull(node2);
			Interface link= cbgp.netAddLink("1.0.0.1", "1.0.0.2", 10);
			assertNotNull(link);
			link= null;
		} catch (CBGPException ex) {
			fail(ex.getMessage());
		}
		try {
			Vector links= node1.getLinks();
			assertNotNull(links);
			assertEquals(1, links.size());
			for (Iterator iter= links.iterator(); iter.hasNext(); ) {
				iter.next();
			}
		} catch (CBGPException e) {
			fail("could not get node links");
		}
	}
	
	@Test
	public void testLTLLink() {
		Node node1, node2, node3;
		try {
			cbgp.netAddDomain(0);
			node1= cbgp.netAddNode("1.0.0.1", 0);
			node2= cbgp.netAddNode("1.0.0.2", 0);
			node3= cbgp.netAddNode("1.0.0.3", 0);
			Interface link12= node1.addLTLLink(node2, true);
			assertNotNull(link12);
			Interface link13= node1.addLTLLink(node3, false);
			assertNotNull(link13);
			Interface link31= node3.addLTLLink(node1, false);
			assertNotNull(link31);
		} catch (CBGPException e) {
			fail(e.getMessage());
		}
	}

	@Test(expected=CBGPException.class)
	public void testLTLLinkDuplicate() throws CBGPException {
		Node node1= null, node2= null;
		try {
			cbgp.netAddDomain(0);
			node1= cbgp.netAddNode("1.0.0.1", 0);
			node2= cbgp.netAddNode("1.0.0.2", 0);
			Interface link12= node1.addLTLLink(node2, true);
			assertNotNull(link12);
		} catch (CBGPException e) {
			fail(e.getMessage());
		}
		node2.addLTLLink(node1, false);
	}
	
	@Test(expected=CBGPException.class)
	public void testLTLLinkSelfLoop() throws CBGPException {
		IGPDomain domain= cbgp.netAddDomain(1);
		Node node1= domain.addNode("1.0.0.1");
		node1.addLTLLink(node1, true);
	}
	
	@Test
	public void testGetLinks() throws CBGPException {
		IGPDomain domain= cbgp.netAddDomain(1);
		Node node1= domain.addNode("1.0.0.1");
		Node node2= domain.addNode("1.0.0.2");
		Node node3= domain.addNode("1.0.0.3");
		Interface link12= node1.addLTLLink(node2, true);
		Interface link23= node2.addLTLLink(node3, true);
		Vector<Interface> links= cbgp.netGetLinks();
		assertEquals(4, links.size());
		assertTrue(links.contains(link12));
		assertTrue(links.contains(link23));
	}

}

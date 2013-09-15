The C-BGP Routing Solver
==

C-BGP is an efficient solver for BGP, the de facto standard protocol
used for exchanging routing information accross domains in the
Internet. BGP was initially described in RFC1771 in 1995. It has since
undergone several updates recently standardized in RFC4271 (2006).

C-BGP is aimed at computing the outcome of the BGP decision process in
networks composed of several routers. For this purpose, it takes into
account the routers' configuration, the externally received BGP routes
and the network topology. It supports the complete BGP decision
process, versatile import and export filters, route-reflection, and
experimental attributes such as redistribution communities. It is
easily configurable through a CISCO-like command-line interface. C-BGP
has been described in an IEEE Network magazine paper entitled
"Modeling the Routing of an Autonomous System with C-BGP" (please
refer to this paper when you cite C-BGP).

C-BGP can be used as a research tool to experiment with modified
decision processes and additional BGP route attributes. It can also be
used by the operator of an ISP network to evaluate the impact of
logical and topological changes on the routing tables computed in its
routers. Topological changes include links and routers
failures. Logical changes include changes in the configuration of the
routers such as input/output routing policies or IGP link
weights. Thanks to its efficiency, C-BGP can be used on large
topologies with sizes of the same order of magnitude than the
Internet. For the moment, we mainly use it to study interdomain
traffic engineering techniques and to model the network of ISPs.

The original version of C-BGP was written by Bruno Quoitin within the
Computer Science and Engineering Department at Universite Catholique
de Louvain (UCL), in Belgium. It has since been contributed by others
(see the AUTHORS file for a complete list). C-BGP is written in C
language. It is mainly used and tested on Linux and Mac OS X. It has
also been used on other platforms such as FreeBSD, Solaris and even
under Windows, using the cygwin API. It should compile on most POSIX
compliant environment. C-BGP is Open-Source and provided under the
GPL license for academic use. The text of the GPL license is
available in the COPYING file included in this distribution.
Commercial use of C-BGP is covered by a separate license since
version 2.0.0.


WHERE TO OBTAIN LIBGDS & CBGP
==

The main location is now sourceforge.
	http://sourceforge.net/projects/c-bgp/
	http://sourceforge.net/projects/libgds


Older version can be obtained from
	http://cbgp.info.ucl.ac.be
	http://libgds.info.ucl.ac.be


REQUIRED LIBRARIES AND TOOLS
==

- a decent version of gcc and GNU make

- the pcre library: perl compatible regular expressions
  
- the readline library: this is optional, but it greatly enhances the cbgp
  experience :-)

- libz and libbz2

- pkg-config

Note that for libraries, you will need to install the actual library and the header files to be able to link against the library. These header files are usually available in a "-devel" package.


HOW TO BUILD LIBGDS & CBGP
==

1). build libgds

	tar xvzf libgdz-x.x.x.tar.gz
	cd libgds-x.x.x

	# at this stage you can specify an alternative installation directory
	# with --prefix=<path>, the default being /usr/local
	./configure

	make clean
	make

	# before installing, please check the section below that talks about
	# testing
	make install


Note: if you installed libgds in a non-standard directory, you will need to
make sure that pkg-config is able to find libgds. This is usually done by
adding the libgds install directory to pkg-config using the PKG_CONFIG_PATH
environment variable. For example, if libgds was installed in /home/foo/local, you will need to add it to the PKG_CONFIG_PATH variable as follows:

	export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/home/foo/local/lib/pkgconfig

you can then test that pkg-config is able to find libgds by issuing the following command:

	pkg-config libgds --modversion

the above command should return the version of the installed libgds library


2). builds cbgp

	tar xvzf cbgp-x.x.x.tar.gz
	cd cbgp-x.x.x

	# at this stage, several options can be used (use --help to obtain
	# the complete list. important options are
	# --with-readline=<path> to specify where the readline library is located
	# --with-pcre=<path> to specify where the pcre library is located
	# --with-jni to allow the java native interface to compile
	./configure

	make clean
	make

	# before installing, please check the section below that talks about
	# testing
	make install


HOW TO TEST IF LIBGDS & CBGP ARE FUNCTIONNAL
==

1). run internal unit tests

libgds and cbgp source repository contains several unit test functions that can be run with a single line

* libgds: make check
* cbgp: make check

most tests should succeed. A final line is provided that summarizes the number of tests and how many failed among them. At the time of this writing, the following numbers were reported:

* libgds: FAILURES=0 / SKIPPED=2 / TESTS=153
* cbgp: FAILURES=1 / SKIPPED=5 / TESTS=188

if the numbers you obtain deviate significantly from the above numbers, you should become suspicious and look more carefully)


2). run external black-box tests

cbgp comes with a (huge) perl script that runs several cbgp scripts and observe their behaviour and results. to run these tests, you should move to the "./valid" subdirectory in the cbgp main directory and run "./cbgp-validation.pl --no-cache". this can take some time depending on the platform.


3). run cbgp

the final test consists in running cbgp. at this point it might be interesting to check if cbgp was correctly linked with the readline library. it should provide auto-completion for most cbgp commands as well as a history of all the commands typed in the cbgp console. a simple test is done as follows for the version just compiled (not yet installed)

# assuming you are in the root cbgp directory
../src/cbgp
C-BGP Routing Solver version 2.0.0-rc3

  Copyright (C) 2002-2010 Bruno Quoitin
  Networking Lab
  Computer Science Institute
  University of Mons (UMONS)
  Mons, Belgium
  
  This software is free for personal and educational uses.
  See file COPYING for details.

help is bound to '?' key
cbgp> init.
cbgp>

here you can now type "sh" then press the "tabulation" key. it should complete with "show". press again the tabulation key twice and it should list the sub-commands of "show"

cbgp> sh <tab>
cbgp> show <tab><tab>
        comm-hash-content
        comm-hash-size
        comm-hash-stat
        commands
        mem-limit
        mrt
        path-hash-content
        path-hash-size
        path-hash-stat
        version

in case auto-completion and/or history do not seem to work, check the results of the "./configure" script for warnings related to the version and features of the readline library that was detected. in case the wrong version of the readline library was chosen, you can specify where to find the correct version by using the "--with-readline==<path>" option with the "./configure" script. for example, on my machine, i need to run

./configure -with-readline=/opt/local

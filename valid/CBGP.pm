#!/usr/bin/perl
# ===================================================================
# @(#)CBGP.pm
#
# @author Bruno Quoitin (bruno.quoitin@umons.ac.be)
# @date 19/02/2004
# @lastdate 29/09/2011
#
# @version 0.6
# ===================================================================
# This package is an helper for Perl scripts that want to use C-BGP in
# an interactive way. The package uses a pair of pipes created thanks
# to the IPC::Open2 module. Those pipes are used to write to and read
# from a C-BGP instance.
#
# Note that particular attention must be paid to the way the input and
# output are read and written. Since we are in presence of two
# separate processes that are cooperating, deadlocks can occur. Simple
# precautions will avoid most of the problems due to pipe buffering:
#
# 1). use the "set autoflush on" statement in the C-BGP script will
# activate an automatic flush of the C-BGP output after the following
# commands: show rib, show rib-in, show networks, show peers and
# record-route. More C-BGP commands will hopefully support this
# feature in the future.
#
# 2). use the autoflush() method on the output descriptor returned by
# open2() in the Perl script. This is done by the class in this
# package.
#
# 3). provide a non-buffered read of the C-BGP output through calls to
# sysread instead of read of the <> operator. This is also done by the
# class in this package.
# ===================================================================

package CBGP;

require Exporter;
@ISA= qw(Exporter);
@EXPORT= qw(spawn
	    expect
	    send
	    send_cmd
	    finalize
	    get_rt
	    get_rib
	    trace_route
	    );
$VERSION= '0.6';

use strict;
use warnings;

use IO::Handle;
use IO::Select;
use IPC::Open2;
use POSIX;
use Symbol;
use threads;
use Thread::Queue;
use Thread::Semaphore;
use CBGPValid::UI;

return 1;

# -----[ thread status ]-----
use constant THREAD_NOT_RUNNING => 0;
use constant THREAD_RUNNING => 1;
use constant THREAD_TERMINATED => 2;
use constant THREAD_ABORTED => -1;

# -----[ logging options ]-----
use constant LOG_FILE => ".CBGP.pm.log";
use constant LOG_DEFAULT => 0;

# -----[ new ]-------------------------------------------------------
# Create a new instance of a C-BGP wrapper
# -------------------------------------------------------------------
sub new(@)
{
    my $class= shift;
    my $cbgp_prog= "cbgp";

    if ($] == 5.008) {
	print STDERR "Warning (CBGP.pm): version 5.8.0 of Perl suffers from memory leaks\n";
    }

    # Any spec for the cbgp program
    if (scalar(@_) > 0) {
	$cbgp_prog= shift;
    }
    # Any spec for the log ?
    if (scalar(@_) > 0) {
	$cbgp_prog= $cbgp_prog." -l ".shift;
    } else {
	$cbgp_prog= $cbgp_prog." -l /dev/null";
    }
    # Redirect C-BGP stderr to stdout
    $cbgp_prog= $cbgp_prog." 2>&1";

    # Shared variable with input lines
    my $thread_status : shared = THREAD_NOT_RUNNING;

    my $cbgp_ref = {
	'queue' => new Thread::Queue,
	'in_file' => gensym(),             # create anonymous globs for file
				           # descriptors (one for input
	'out_file' => gensym(),            # and one for output)
	'pid' => -1,                       # PID of spawned C-BGP process
	'prog' => $cbgp_prog,              # C-BGP command-line
	'thread' => undef,                 # Reader thread
	'thread_status' => \$thread_status,
	'ready' => new Thread::Semaphore(0),
	'log' => LOG_DEFAULT,
    };
    bless $cbgp_ref;

    unlink ".CBGP.pm.log";

    return $cbgp_ref;
}


# -----[ thread_reader ]---------------------------------------------
# This thread
# -------------------------------------------------------------------
sub thread_reader($)
{
    my $self= shift;
    my $timeout= 0.1;
    my $input_buffer= "";
    my $thread_terminate= 0;

    close $self->{out_file} or
	 die "Error: unable to close output pipe";

    # Signal that the reader thread is now running
    ${$self->{thread_status}}= THREAD_RUNNING;
    $self->{ready}->up(1);

    # Loop while the input descriptor is open...
    while (${$self->{thread_status}} == THREAD_RUNNING) {

        # Create handle set for reading and add input file descriptor
        my $read_set = new IO::Select();
        $read_set->add($self->{in_file});

	my $rh_set = IO::Select->select($read_set, undef, undef, $timeout);
	if ($rh_set > 0) {

	    # We use sysread in order to bypass the buffering
	    # mechanism of Perl. New data are put at the end of the
	    # current input buffer
	    my $n= sysread($self->{in_file}, $input_buffer,
			   1024, length($input_buffer));

            my @new_lines;

	    if ($n == 0) {

		#print STDERR "Debug (CBGP thread): input closed\n";

		# End of file met, returns everything remaining in
		# input buffer
		if ($input_buffer) {
		    push @new_lines, ($input_buffer);
		}
		$input_buffer= "";
		${$self->{thread_status}}= THREAD_TERMINATED;

	    } elsif ($n > 0) {

		# Split the buffer. If the last line does not terminate
		# with an end-of-line, accumulate it in the input buffer
		if ($input_buffer=~ m/\n$/) {
		    @new_lines= split /\n/, $input_buffer;
		    $input_buffer= "";
		} else {
		    @new_lines= split /\n/, $input_buffer;
		    $input_buffer= pop @new_lines;
		}

	    } else {

		# An error has occured
		print STDERR "Error (CBGP thread): sysread: $!\n";
		print STDERR "Debug (CBGP thread): aborted\n";
		${$self->{thread_running}}= THREAD_ABORTED;

	    }

	    $self->{queue}->enqueue(@new_lines);
		
	} elsif ($rh_set < 0) {

	    # An error has occured
	    print STDERR "Error (CBGP thread): select: $!\n";
	    print STDERR "Debug (CBGP thread): aborted\n";
	    ${$self->{thread_status}}= THREAD_ABORTED;

	}
    }

    #close $self->{in_file} or
    #  die "Error: unable to close input pipe";

    #print STDERR "Debug (CBGP thread): terminated\n";
    return 0;
}


# -----[ spawn ]-----------------------------------------------------
# Spawn a C-BGP instance and a thread that reads the output of C-BGP
# -------------------------------------------------------------------
sub spawn()
{
    my $self= shift;

    # Spawn C-BGP process and returns two file descriptors. One for
    # the input pipe used to send messages to the C-BGP instance and
    # another for the output pipe to receive answers from the C-BGP
    # instance
    $self->{pid}= open2($self->{in_file}, $self->{out_file},
			$self->{prog});

    # Activate autoflush on the output descriptor
    $self->{out_file}->autoflush();

    # Spawn a new thread in order to read the C-BGP input as soon as
    # it is available. Then wait until the thread has really started
    # otherwize, input could be missed.
    $self->{thread}= threads->new(\&thread_reader, $self);
    $self->{ready}->down(1);

    # Close the input descriptor on the caller (parent) side. Keep
    # them open in the thread
    close $self->{in_file} or
	  die "Error: unable to close input pipe";

}


# -----[ expect ]----------------------------------------------------
# Expect input from the C-BGP instance. The call can be either
# blocking or non-blocking.
#
# Parameters:
# - blocking: 1 => the call will block if there is nothing to read
#             0 => the call will never block
# -------------------------------------------------------------------
sub expect($)
{
    my $self= shift;
    my $blocking= shift;

    (($blocking == 0) || ($blocking == 1)) or
	die "expect's <blocking> parameter must be 0 or 1";

    if ($blocking) {
	return $self->{queue}->dequeue;
    } else {
	return $self->{queue}->dequeue_nb;
    }

    return undef;
}

# -----[ log ]-------------------------------------------------------
# Write a message into the .CBGP.pm.log file
# -------------------------------------------------------------------
sub log($)
{
    my $self= shift;
    my $msg= shift;

    if ($self->{log}) {
	if (open(CBGP_LOG, ">>".LOG_FILE)) {
	    print CBGP_LOG "$msg";
	    close(CBGP_LOG);
	}
    }
}

# -----[ send ]------------------------------------------------------
# Send a command to the C-BGP instance
# -------------------------------------------------------------------
sub send($)
{
    my $self= shift;
    my $msg= shift;

    my $out_file= $self->{out_file};

    if (!$out_file->opened || $out_file->error ||
	(${$self->{thread_status}} != THREAD_RUNNING)) {
	#print STDERR "Error: output to CBGP has been closed\n";
	return -1;
    }

    $self->log($msg);

    $SIG{'PIPE'}= 'IGNORE';
    if (!$out_file->print("$msg")) {
      return -1;
    }
    $SIG{'PIPE'}= 'DEFAULT';

    return 0;
}

# -----[ send_cmd ]--------------------------------------------------
# Send a CLI command to C-BGP
# -------------------------------------------------------------------
sub send_cmd($$)
{
  my ($self, $cmd)= @_;

  if ($self->send("$cmd\n")) {
    show_error("Error: could not send to CBGP instance");
    show_error("       cmd \"$cmd\"");
    die "send_cmd() failed";
  }
}

# -----[ finalize ]--------------------------------------------------
# Close pipes and wait for termination of the C-BGP instance and the
# reader thread
# -------------------------------------------------------------------
sub finalize()
{
  my $self= shift;

  ${$self->{thread_status}}= THREAD_TERMINATED;

  while ($self->{thread}->is_running()) {
    select(undef, undef, undef, 0.1);
  }

  # Wait for the thread termination
  if ($self->{thread}) {
    my $res= $self->{thread}->join();
    if ($res != 0) {
      print STDERR "Error: CBGP thread exited abnormaly\n";
    }
  }

  close $self->{out_file} or
    die "Error: unable to close output pipe";

  # Wait until the C-BGP instance has terminated. This avoids an
  # accumulation of defunct processes
  waitpid($self->{pid}, 0);
}

# -----[ get_rt ]----------------------------------------------------
# Request from C-BGP the whole routing table of the given router. The
# resulting routing table is returned in an hash table where the keys
# are the canonic prefixes (see the 'canonic_prefix' function) present
# in the routing table. For each prefix, the complete route is stored
# in the corresponding hash value in an array structured as follows:
#
#   (0) next-hop
#   (1) metric: total IGP-weight for routes of type IGP
#   (2) type: string 'STATIC' / 'IGP' / 'BGP'
#
# Parameters:
# - IP address of the router
# -------------------------------------------------------------------
sub get_rt($)
{
    my $self= shift;
    my $router= shift;
    my $rt= {};
    my $error= 0;

    die if $self->send("net node $router show rt *\n");
    die if $self->send("print \"done\\n\"\n");
    while ((my $result= $self->expect(1)) ne "done") {
	($result =~ m/^\#/) and next;
	my @fields= split /\s+/, $result;
	if (scalar(@fields) == 4) {
	    my $prefix= canonic_prefix($fields[0]);
	    my $next_hop= $fields[1];
	    my $metric= $fields[2];
	    my $type= $fields[3];
	    $rt->{$prefix}= [$next_hop, $metric, $type];
	} else {
	    print "Warning: invalid route \"$result\" (skipped)\n";
	    $error= 1;
	}
    }
    return ($error)?undef:$rt;
}

# -----[ get_rib ]---------------------------------------------------
# Request from C-BGP the whole RIB of the given router. The resulting
# RIB is returned in an hash table where the keys are the canonic
# prefixes (see the 'canonic_prefix' function) present in the RIB. For
# each prefix, the complete route is stored in the corresponding hash
# value in an array structured as follows:
#
#   (0) next-hop
#   (1) local-pref
#   (2) multi-exit-discriminator
#   (3) as-path
#   (4) origin
#
# Parameters:
# - IP address of the router
# -------------------------------------------------------------------
sub get_rib($)
{
    my $self= shift;
    my $router= shift;
    my $rib= {};
    my $error= 0;

    die if $self->send("bgp router $router show rib *\n");
    die if $self->send("print \"done\\n\"\n");
    while ((my $result= $self->expect(1)) ne "done") {
	($result =~ m/^\#/) and next;
	my @fields= split /\s+/, $result, 6;
	if (scalar(@fields) == 6) {
	    my $prefix= canonic_prefix($fields[1]);
	    my $next_hop= $fields[2];
	    my $pref= $fields[3];
	    my $med= $fields[4];
	    my @path= split /\s+/, $fields[5];
	    my $origin= pop @path;
	    my $path= join " ", @path;
	    $rib->{$prefix}= [$next_hop, $pref, $med, $path, $origin];
	} else {
	    print "Warning: invalid route \"$result\" (skipped)\n";
	    $error= 1;
	}
    }
    return ($error)?undef:$rib;
}

# -----[ trace_route ]-----------------------------------------------
# Request from C-BGP the trace-route from DST router towards SRC
# router. The resulting trace route is returned in an array structured
# as follows:
#
#   (0) status: string 'SUCCESS' / 'UNREACH' / ... (see C-BGP docs)
#   (1) path: string that contains the sequence of IP addresses
#
# Parameters:
# - SRC router
# - DST router
# -------------------------------------------------------------------
sub trace_route($$) {
    my $self= shift;
    my $src= shift;
    my $dst= shift;
    my $rr= undef;

    die if $self->send("net node $src record-route-delay $dst\n");
    die if $self->send("print \"done\\n\"\n");
    while ((my $result= $self->expect(1)) ne "done") {
	($result =~ m/^\#/) and next;
	my @fields= split /\s+/, $result;
	if (scalar(@fields) >= 4) {
	    shift(@fields);
	    shift(@fields);
	    my $status= (shift(@fields) eq 'SUCCESS')?1:0;
	    my $weight= pop @fields;
	    my $delay= pop @fields;
	    $rr= [$status, join(" ", @fields), $delay, $weight];
	} else {
	    print "Warning: invalid route \"$result\" (skipped)\n";
	}
    }    
    return $rr;
}

__END__

=head1 NAME

CBGP - Perl wrapper for C-BGP

=head1 VERSION

0.1

=head1 SYNOPSIS

use CBGP;

$cbgp= new CBGP;

$cbgp->spawn();

$cbgp->send();

$cbgp->expect();

$cbgp->finalize();

=head1 DESCRIPTION

This package is an helper for Perl scripts that want to use C-BGP in
an interactive way. The package uses a pair of pipes created thanks
to the IPC::Open2 module. Those pipes are used to write to and read
from a C-BGP instance.

Note that particular attention must be paid to the way the input and
output are read and written. Since we are in presence of two
separate processes that are cooperating, deadlocks can occur. Simple
precautions will avoid most of the problems due to pipe buffering:

1). use the "set autoflush on" statement in the C-BGP script will
activate an automatic flush of the C-BGP output after the following
commands: show rib, show rib-in, show networks, show peers and
record-route. More C-BGP commands will hopefully support this
feature in the future.

2). use the autoflush() method on the output descriptor returned by
open2() in the Perl script. This is done by the class in this
package. This is also done by the class in this package.

3). provide a non-buffered read of the C-BGP output through calls to
sysread instead of read of the <> operator.

=head1 AUTHORS

Bruno Quoitin (bqu@info.ucl.ac.be), from the CSE Department of
University of Louvain-la-Neuve, in Belgium.

=head1 COPYRIGHT

This Perl package is provided under the LGPL license. Please have a
look at the Free Software Foundation (http://www.fsf.org) if you are
not familiar with this kind of license.

=head1 DISCLAIMER

This software is provided ``as is'' and any express or implied
warranties, including, but not limited to, the implied warranties of
merchantability and fitness for a particular purpose are disclaimed.
in no event shall the authors be liable for any direct, indirect,
incidental, special, exemplary, or consequential damages (including,
but not limited to, procurement of substitute goods or services; loss
of use, data, or profits; or business interruption) however caused and
on any theory of liability, whether in contract, strict liability, or
tort (including negligence or otherwise) arising in any way out of the
use of this software, even if advised of the possibility of such
damage.

=cut

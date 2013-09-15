# ===================================================================
# CBGPValid::Tests.pm
#
# author Bruno Quoitin (bruno.quoitin@umons.ac.be)
# $Id: Tests.pm,v 1.15 2009-08-31 09:49:29 bqu Exp $
# ===================================================================
#
# Usage:
#
#   my $tests= CBGPValid::Tests->new(-cbgppath=>"/usr/local/bin/");
#   $tests->register("my smart test", "smart_test_function");
#   $tests->register("another smart test", "smart_test_function2");
#   ...
#   $tests->run();
#
# ===================================================================

package CBGPValid::Tests;

require Exporter;
@ISA= qw(Exporter);
@EXPORT= qw(show_testing
	    show_testing_success
	    show_testing_cache
	    show_testing_failure
	    show_testing_skipped
	    show_testing_disabled
	    show_testing_todo
	   );

use strict;
use CBGP;
use CBGPValid::TestConstants;
use CBGPValid::UI;

# -----[ show_testing ]----------------------------------------------
sub show_testing($)
{
  my ($msg)= @_;

  #print STDERR "Testing: \033[37;1m$msg\033[0m";
  print STDERR "Testing: $msg";
}

# -----[ show_testing_success ]--------------------------------------
sub show_testing_success(;$)
{
  my ($time)= @_;

  if (defined($time)) {
    printf STDERR "\033[65G\033[32;1mSUCCESS\033[0m (%ds)\n", $time;
  } else {
    print STDERR "\033[65G\033[32;1mSUCCESS\033[0m\n";
  }
}

# -----[ show_testing_cache ]----------------------------------------
sub show_testing_cache()
{
  print STDERR "\033[65G\033[32;1m(CACHE)\033[0m\n";
}

# -----[ show_testing_failure ]--------------------------------------
sub show_testing_failure()
{
  my ($self)= @_;

  print STDERR "\033[65G\033[31;1mFAILURE\033[0m\n";
}

# -----[ show_testing_waiting ]--------------------------------------
sub show_testing_waiting()
{
  my ($self)= @_;

  print STDERR "\033[65G\033[33;1m-WAIT-\033[0m";
}

# -----[ show_testing_crashed ]--------------------------------------
sub show_testing_crashed()
{
  my ($self)= @_;

  print STDERR "\033[65G\033[31;1mCRASHED\033[0m\n";
}

# -----[ show_testing_skipped ]--------------------------------------
sub show_testing_skipped()
{
  print STDERR "\033[65G\033[31;1mNOT-TESTED\033[0m\n";
}

# -----[ show_testing_disabled ]-------------------------------------
sub show_testing_disabled($)
{
  my ($result)= @_;
  my $msg= "DISABLED";
  if (defined($result)) {
    $msg= TEST_RESULT_MSG()->{$result};
  }
  print STDERR "\033[65G\033[33;1m$msg\033[0m\n";
}

# -----[ show_testing_todo ]-----------------------------------------
sub show_testing_todo()
{
  print STDERR "\033[65G\033[33;1mTO-DO \033[0m\n";
}

# -----[ debug ]-----------------------------------------------------
sub debug($$)
{
  my ($self, $msg)= @_;

  if ($self->{'debug'}) {
    print STDERR "debug: $msg\n";
    STDERR->flush();
  }
}


# -----[ new ]-------------------------------------------------------
# Parameters:
#   -cache      : 0 to disable loading the cache, 1 to enable using
#                 the cached results
#   -cbgppath   : complete path to C-BGP binary
#   -debug      : boolean (true => debug information is allowed)
#   -include    : array of test names (if provided, only these tests
#                 are performed)
#   -maxfailures:
#   -valgrind
#   -libtool
#   -glibtool
# -------------------------------------------------------------------
sub new($%)
{
  my ($class, %args)= @_;
  my $self= {
	     'cache'         => {},
	     'cache-enabled' => 0,
	     'cache-file'    => 'cbgp-valid.cache',
	     'cbgp-path'     => 'cbgp',
	     'debug'         => 0,
	     'duration'      => 0,
	     'list'          => [],
	     'id'            => 0,
	     'include'       => undef,
	     'max-failures'  => 0,
	     'max-warnings'  => 0,
	     'num-success'   => 0,
	     'num-failures'  => 0,
	     'num-warnings'  => 0,
	     'num-skipped'   => 0,
	     'valgrind'      => 0,
	     'libtool'       => 0,
	     'glibtool'      => 0,
	    };
  (exists($args{'-cache'})) and
    $self->{'cache-enabled'}= $args{'-cache'};
  (exists($args{'-cbgppath'})) and
    $self->{'cbgp-path'}= $args{'-cbgppath'};
  (exists($args{'-debug'})) and
    $self->{'debug'}= $args{'-debug'};
  (exists($args{'-include'})) and
    $self->{'include'}= $args{'-include'};
  (exists($args{-maxfailures})) and
    $self->{'max-failures'}= $args{-maxfailures};
  (exists($args{'-valgrind'})) and
    $self->{'valgrind'}= $args{'-valgrind'};
  (exists($args{'-libtool'})) and
    $self->{'libtool'}= $args{'-libtool'};
  (exists($args{'-glibtool'})) and
    $self->{'glibtool'}= $args{'-glibtool'};
  bless $self, $class;

  $self->cache_read();

  return $self;
}

# -----[ register ]--------------------------------------------------
sub register($$$$;@)
{
  my $self= shift(@_);
  my $file= shift(@_);
  my $name= shift(@_);
  my $func= shift(@_);

  my $test_record= {
		    TEST_FIELD_ID       => $self->{id}++,
		    TEST_FIELD_FILE     => $file,
		    TEST_FIELD_NAME     => $name,
		    TEST_FIELD_FUNC     => $func,
		    TEST_FIELD_RESULT   => TEST_SKIPPED,
		    TEST_FIELD_DURATION => undef,
		    TEST_FIELD_DOC      => undef,
		    TEST_FIELD_ARGS     => [],
		   };
  while (@_) {
    push @{$test_record->{TEST_FIELD_ARGS}}, (shift(@_));
  }

#  foreach my $key (sort keys %$test_record) {
#    my $value= $test_record->{$key};
#    print "$key => ";
#    if (defined($value)) {
#      print "$value";
#    } else {
#      print "(undef)";
#    }
#    print "\n";
#  }

  push @{$self->{'list'}}, ($test_record);
}

# -----[ set_result ]------------------------------------------------
sub set_result($$;$)
{
  my ($self, $test, $result, $duration)= @_;

  $test->{TEST_FIELD_RESULT}= $result;
  (defined($duration)) and
    $test->{TEST_FIELD_DURATION}= $duration;
}

# -----[ is_included ]-----------------------------------------------
sub is_included($$) {
  my ($self, $name)= @_;
  return 1 if (!defined($self->{'include'}));
  foreach my $include (@{$self->{'include'}}) {
    return 1 if ($name =~ m/$include/);
  }
  return 0;
}

# -----[ get_cbgp_instance ]-----------------------------------------
sub get_cbgp_instance($$) {
  my ($self, $test_name)= @_;
  my $cbgp_cmd= $self->{'cbgp-path'};

  # If cbgp must be run under valgrind...
  ($self->{'valgrind'}) and
    $cbgp_cmd= 'valgrind --leak-check=full --log-file=valid.valgrind '.
      $cbgp_cmd;

  # If cbgp must be run through (g)libtool
  ($self->{'libtool'}) and
    $cbgp_cmd= 'libtool --mode=execute '.$cbgp_cmd;
  ($self->{'glibtool'}) and
    $cbgp_cmd= 'glibtool --mode=execute '.$cbgp_cmd;

  $self->debug("$cbgp_cmd");

  my $cbgp= CBGP->new($cbgp_cmd);
  my $log_file= ".$test_name.log";
  ($log_file =~ tr[\ -][__]);
  unlink $log_file;
  $cbgp->{log_file}= $log_file;
  $cbgp->{log}= 1;
  $cbgp->spawn();
  $cbgp->send_cmd("# *** $test_name ***");
  $cbgp->send_cmd("# Generated by C-BGP's validation process");
  $cbgp->send_cmd("# (c) 2007-2011, Bruno Quoitin");
  $cbgp->send_cmd("# (bruno.quoitin\@umons.ac.be)");
  $cbgp->send_cmd("set autoflush on\n");
  return $cbgp;
}

# -----[ run ]-------------------------------------------------------
sub run($) {
  my ($self)= @_;

  my $cache= $self->{'cache'};

  my $time_start= time();
  foreach my $test_record (@{$self->{'list'}}) {
    my $test_id= $test_record->{TEST_FIELD_ID};
    my $test_file= $test_record->{TEST_FIELD_FILE};
    my $test_name= $test_record->{TEST_FIELD_NAME};
    my $test_func= "::".$test_record->{TEST_FIELD_FUNC};
    my $test_args= $test_record->{TEST_FIELD_ARGS};

    if (!defined($test_func) || !$self->is_included($test_name)) {
      $self->set_result($test_record, TEST_DISABLED);
      #$cache->{$test_name}= TEST_DISABLED;
    } else {
      if (!$self->{'cache-enabled'} ||
	  !exists($cache->{$test_name}) ||
	  ($cache->{$test_name} != TEST_SUCCESS)) {

	my $result;
	
	# Get a C-BGP instance
	my $cbgp= $self->get_cbgp_instance($test_name);
	$self->debug("testing $test_name");
	my $test_time_start= time();

	# Call the function (with a symbolic reference)
	# We need to temporarily disable strict references checking...
	no strict 'refs';
	$result= &$test_func($cbgp, @$test_args);
	use strict 'refs';
	my $test_time_end= time();
	my $test_time_duration= $test_time_end-$test_time_start;

	show_testing("$test_name");

	# Check that the process is still running
	# (note that $cbgp->send_cmd() cannot be used here !)
	show_testing_waiting();

	$cbgp->send("\n");
	$cbgp->send("print \"STILL_ALIVE\\n\"\n");
	my $time_alive= time();
	my $crashed= 1;
	while (time()-$time_alive < 5) {
	  my $result= $cbgp->expect(0);
	  if (defined($result)) {
	    if ($result =~ m/STILL_ALIVE/) {
	      $crashed= 0;
	      last;
	    }
	  } else {
	    select(undef, undef, undef, 0.1);
	    #sleep(1);
	  }
	}
	($crashed == 1) and $result= TEST_CRASHED;

	$cbgp->finalize();
	$self->set_result($test_record, $result, $test_time_duration);
	if ($result == TEST_SUCCESS) {
	  $self->{'num-success'}++;
	  show_testing_success($test_time_duration);
	} elsif (($result == TEST_DISABLED) ||
		 ($result == TEST_MISSING)) {
	  $self->{'num-skipped'}++;
	  show_testing_disabled($result);
	} elsif ($result == TEST_SKIPPED) {
	  $self->{'num-skipped'}++;
	  show_testing_skipped();
	} elsif ($result == TEST_TODO) {
	  $self->{'num-skipped'}++;
	  show_testing_todo();
	} elsif ($result == TEST_FAILURE) {
	  $self->{'num-failures'}++;
	  show_testing_failure();
	} else {
	  $self->{'num-failures'}++;
	  show_testing_crashed();
	}
	$cache->{$test_name}= $result;
      } else {
	$self->set_result($test_record, $cache->{$test_name});
	show_testing($test_name);
	show_testing_cache();
      }
    }

    if (($self->{'max-failures'} > 0) &&
	($self->{'num-failures'} >= $self->{'max-failures'})) {
      show_error("Error: too many failures.");
      last;
    }
  }
  $self->{'duration'}= time()-$time_start;

  print "\n==Summary==\n";
  printf "  FAILURES=%d / SKIPPED=%d / TESTS=%d\n\n",
    $self->{'num-failures'}, $self->{'num-skipped'},
      $self->{'num-success'};

  $self->cache_write();

  return $self->{'num-failures'};
}

# -----[ cache_read ]------------------------------------------------
sub cache_read()
{
  my ($self)= @_;

  if (open(CACHE, "<".$self->{'cache-file'})) {
    while (<CACHE>) {
      chomp;
      my ($test_result, $test_name)= split /\t/;
      (exists($self->{'cache'}->{$test_name})) and
	die "duplicate test in cache file";
      $self->{'cache'}->{$test_name}= $test_result;
    }
    close(CACHE);
  }
}

# -----[ cache_write ]-----------------------------------------------
sub cache_write()
{
  my ($self)= @_;

  if (open(CACHE, ">".$self->{'cache-file'})) {
    my $cache= $self->{'cache'};
    foreach my $test (keys %$cache)
      {
	my $test_result= -1;
	(exists($cache->{$test})) and
	  $test_result= $cache->{$test};
	print CACHE "$test_result\t$test\n";
      }
    close(CACHE);
  }
}


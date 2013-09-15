# ===================================================================
# CBGPValid::XMLReport.pm
#
# author Bruno Quoitin (bruno.quoitin@uclouvain.be)
# $Id: XMLReport.pm,v 1.3 2009-06-25 14:36:27 bqu Exp $
# ===================================================================

package CBGPValid::XMLReport;

require Exporter;
@ISA= qw(Exporter);

use strict;
use Symbol;
use CBGPValid::BaseReport;
use CBGPValid::TestConstants;
use CBGPValid::UI;

# -----[ doc_write_section ]-----------------------------------------
#
# -------------------------------------------------------------------
sub doc_write_section($$)
{
  my ($stream, $section)= @_;

  my $state= 0;

  foreach my $line (@$section) {
    #print "line(state:$state) [$line]\n";
    if ($state == 0) {
      if ($line =~ m/^\s*-(.*)$/) {
	$state= 1;
	print $stream "<ul>\n";
	print $stream "<li>".escape_text($1)."</li>\n";
      } elsif ($line =~ m/^\s*\*(.*)$/) {
	$state= 2;
	print $stream "<ol>\n";
	print $stream "<li>".escape_text($1)."</li>\n";
      } else {
	print $stream escape_text($line)."\n";
      }
    } elsif ($state > 0) {
      if ($line =~ m/^\s*$/) {
	if ($state == 1) {
	  print $stream "</ul>\n";
	} elsif ($state == 2) {
	  print $stream "</ol>\n";
	}
	$state= 0;
      } elsif ($line =~ m/^\s*[-*](.*)$/) {
	print $stream "<li>".escape_text($1)."</li>\n";
      } else {
	print $stream escape_text($line)."\n";
      }
    }
  }
  if ($state > 0) {
    if ($state == 1) {
      print $stream "</ul>\n";
    } elsif ($state == 2) {
      print $stream "</ol>\n";
    }
  }
}

# -----[ doc_write_resources ]---------------------------------------
#
# -------------------------------------------------------------------
sub doc_write_resources($$)
{
  my ($stream, $resources)= @_;

  foreach my $res (@$resources) {
    my ($ref, $name)= @$res;
    print $stream "<file>$name</file>\n";
  }
}

# -----[ escape_text ]-----------------------------------------------
#
# -------------------------------------------------------------------
sub escape_text($)
  {
    my ($text)= @_;

    $text=~ s/</&lt;/g;
    $text=~ s/>/&gt;/g;
    return $text;
  }

# -----[ doc_write_preformated ]-------------------------------------
#
# -------------------------------------------------------------------
sub doc_write_preformated($$)
{
  my ($stream, $section)= @_;

  print $stream "<pre>\n";
  my $text= escape_text(join "\n", @{$section});
  print $stream "$text";
  print $stream "</pre>\n";
}

# -----[ test_str_status ]-------------------------------------------
#
# -------------------------------------------------------------------
sub test_str_status($)
{
  my ($status)= @_;

  my $color= "black";

  if ($status == TEST_DISABLED) {
    $color= "gray";
  } elsif (($status == TEST_FAILURE) ||
	   ($status == TEST_SKIPPED) ||
	   ($status == TEST_CRASHED)) {
    $color= "red";
  } elsif ($status == TEST_SUCCESS) {
    $color= "green";
  }
  $status= TEST_RESULT_MSG->{$status};

  return $status;
}

# -----[ doc_write_copyright ]---------------------------------------
#
# -------------------------------------------------------------------
sub doc_write_copyright($)
{
  my ($stream)= @_;

  print $stream "<i>(C) 2006, B. Quoitin<br>\n";
  print $stream "Generated on ".localtime(time())."</i>\n";
}

# -----[ doc_write ]-------------------------------------------------
# Generates the documentation file.
# -------------------------------------------------------------------
sub doc_write($$$$)
{
  my ($filename,
      $script_version,
      $doc,
      $tests_index)= @_;

  my $stream= gensym();
  open($stream, ">$filename") or
    die "unable to generate \"$filename\": $!";
  print $stream "<?xml version=\"1.0\"?>\n";
  print $stream "<cbgptestdoc>\n";
  print $stream "  <info>\n";
  print $stream "    <version>$script_version</version>\n";
  print $stream "  </info>\n";
  print $stream "  <suite>\n";

  foreach my $item (sort keys %$doc) {
    print $stream "    <test>\n";
    print $stream "      <name>$doc->{$item}->{Name}</name>\n";
    print $stream "      <ref>$item</ref>\n";
    print $stream "      <status>".test_str_status($tests_index->{$item}->[TEST_FIELD_RESULT])."</status>\n";
    print $stream "      <description>\n";
    if (exists($doc->{$item}->{Description})) {
      doc_write_section($stream, $doc->{$item}->{Description});
    }
    print $stream "      </description>\n";
    # -- Setup --
    print $stream "      <setup>\n";
    if (exists($doc->{$item}->{Setup})) {
      doc_write_section($stream, $doc->{$item}->{Setup});
    }
    print $stream "      </setup>\n";
    # -- Topology --
    print $stream "      <topology>\n";
    if (exists($doc->{$item}->{Topology})) {
      doc_write_preformated($stream, $doc->{$item}->{Topology});
    }
    print $stream "      </topology>\n";
    # -- Scenario --
    print $stream "      <scenario>\n";
    if (exists($doc->{$item}->{Scenario})) {
      doc_write_section($stream, $doc->{$item}->{Scenario});
    }
    print $stream "      </scenario>\n";
    # -- Resources -
    if (exists($doc->{$item}->{'Resources'})) {
      print $stream "      <resources>\n";
      doc_write_resources($stream, $doc->{$item}->{'Resources'});
      print $stream "      </resources>\n";
    }
    print $stream "    </test>\n";
  }
  print $stream "  </suite>\n";
  print $stream "</cbgptestdoc>\n";
  close($stream);
}

# -----[ report_write ]----------------------------------------------
# Build an HTML report containing the test results.
# -------------------------------------------------------------------
sub report_write($$$)
{
  my ($report_prefix,
      $validation,
      $tests)= @_;

  my $cbgp_path= $tests->{'cbgp-path'};
  my $cbgp_version= $validation->{'cbgp_version'};
  my $libgds_version= $validation->{'libgds_version'};
  my $program_args= $validation->{'program_args'};
  my $program_name= $validation->{'program_name'};
  my $program_version= $validation->{'program_version'};
  my $num_failures= $tests->{'num-failures'};
  my $num_warnings= $tests->{'num-warnings'};

  # Build index of tests function names
  my %tests_index= ();
  foreach my $test_record (@{$tests->{'list'}}) {
    $tests_index{$test_record->[TEST_FIELD_FUNC]}= $test_record;
  }

  my $doc= CBGPValid::BaseReport::doc_from_script($program_name);
#, \%tests_index);
  if (defined($doc)) {
    doc_write("$report_prefix-doc.xml", $program_version,
	      $doc, \%tests_index);
  }

  my $report_file_name= "$report_prefix.xml";
  open(REPORT, ">$report_file_name") or
    die "could not create XML report in \"$report_file_name\": $!";
  print REPORT "<?xml version=\"1.0\"?>\n";
  print REPORT "<cbgptest>\n";
  print REPORT "  <info>\n",
  print REPORT "    <version>$program_version</version>\n";
  print REPORT "    <user>bqu</user>\n";
  print REPORT "    <arguments>$program_args</arguments>\n";
  print REPORT "    <cbgpversion>".
    (defined($cbgp_version)?$cbgp_version:"undef").
      "</cbgpversion>\n";
  print REPORT "    <cbgppath>$cbgp_path</cbgppath>\n";
  print REPORT "    <libgdsversion>".
    (defined($libgds_version)?$libgds_version:"undef").
      "</libgdsversion>\n";
  print REPORT "    <failures>$num_failures</failures>\n";
  print REPORT "    <maxfailures>".$tests->{'max-failures'}."</maxfailures>\n";
  print REPORT "    <warnings>$num_warnings</warnings>\n";
  print REPORT "    <maxwarnings>".$tests->{'max-warnings'}."</maxwarnings>\n";
  print REPORT "    <cache>".
    (defined($tests->{'cache-file'})?"yes":"no").
      "</cache>\n";
  print REPORT "    <duration>".$tests->{'duration'}."</duration>\n";
  my $time= POSIX::strftime("%Y/%m/%d %H:%M", localtime(time()));
  print REPORT "    <finished>$time</finished>\n";
  my $uname= `uname -m -p -s -r`;
  chomp($uname);
  print REPORT "    <system>$uname</system>\n";
  print REPORT "  </info>\n";
  print REPORT "  <suite>\n";

#  print REPORT "<h3>Results</h3>\n";
#  print REPORT "<table border=\"1\">\n";
#  print REPORT "<tr>\n";
#  print REPORT "<th>Test ID</th>\n";
#  print REPORT "<th>Test description</th>\n";
#  print REPORT "<th>Result</th>\n";
#  print REPORT "<th>Duration (s.)</th>\n";
#  print REPORT "</tr>\n";
  foreach my $test_record (@{$tests->{'list'}}) {
    my $test_id= $test_record->[TEST_FIELD_ID];
    my $test_name= $test_record->[TEST_FIELD_NAME];
    my $test_func= $test_record->[TEST_FIELD_FUNC];
    my $test_result= $test_record->[TEST_FIELD_RESULT];
    my $test_duration= $test_record->[TEST_FIELD_DURATION];

    if (!defined($doc) || !exists($doc->{$test_func})) {
      show_warning("no documentation for $test_name ($test_func)");
    }

    print REPORT "    <test>\n";
    print REPORT "      <id>$test_id</id>\n";
    print REPORT "      <name>$test_name</name>\n";
    print REPORT "      <result>".test_str_status($test_result)."</result>\n";
    (defined($test_duration)) and
      print REPORT "      <duration>$test_duration</duration>\n";
    print REPORT "      <function>$test_func</function>\n";
    print REPORT "    </test>\n";
  }
  print REPORT "  </suite>\n";
  print REPORT "</cbgptest>\n";
  close(REPORT);
}

# ===================================================================
# CBGPValid::HTMLReport.pm
#
# author Bruno Quoitin (bruno.quoitin@uclouvain.be)
# $Id: HTMLReport.pm,v 1.9 2009-06-25 14:36:27 bqu Exp $
# ===================================================================

package CBGPValid::HTMLReport;

require Exporter;
@ISA= qw(Exporter);

use strict;
use Symbol;
use CBGPValid::BaseReport;
use CBGPValid::TestConstants;
use CBGPValid::UI;

# -----[ doc_write_section_title ]-----------------------------------
sub doc_write_section_title($$)
{
  my ($stream, $title)= @_;

  print $stream "<h3>$title</h3>\n";
}

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

      # Escape special characters
      $line=~ s/</&lt;/g;
      $line=~ s/>/&gt;/g;

      if ($line =~ m/^\s*-(.*)$/) {
	$state= 1;
	print $stream "<ul>\n";
	print $stream "<li>$1\n";
      } elsif ($line =~ m/^\s*\*(.*)$/) {
	$state= 2;
	print $stream "<ol>\n";
	print $stream "<li>$1\n";
      } else {
	print $stream "$line\n";
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
	print $stream "<li>$1\n";
      } else {
	print $stream "$line\n";
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
#  my ($stream, $resources)= @_;
#
#  foreach my $res (@$resources) {
#    my ($ref, $name)= @$res;
#    print $stream "<a href=\"$ref\">$name</a><br>\n";
#  }
}

# -----[ doc_write_preformated ]-------------------------------------
#
# -------------------------------------------------------------------
sub doc_write_preformated($$)
{
  my ($stream, $section)= @_;

  print $stream "<br>\n";
  print $stream "<pre>\n";
  print $stream "".(join "\n", @{$section});
  print $stream "</pre>\n";
  print $stream "<br>\n";
}

# -----[ doc_write_status ]------------------------------------------
#
# -------------------------------------------------------------------
sub doc_write_status($$)
{
  my ($stream, $status)= @_;

  my $color= "black";

  die "undefined status"
    if (!defined($status));

  if (($status == TEST_DISABLED) ||
      ($status == TEST_TODO)) {
    $color= "gray";
  } elsif (($status == TEST_FAILURE) ||
	   ($status == TEST_SKIPPED) ||
	   ($status == TEST_CRASHED)) {
    $color= "red";
  } elsif ($status == TEST_SUCCESS) {
    $color= "green";
  } else {
    die "invalid status code \"$status\"";
  }
  $status= (TEST_RESULT_MSG)->{$status};

  print $stream "<b><font color=\"$color\">$status</font></b>";
}

# -----[ doc_write_copyright ]---------------------------------------
#
# -------------------------------------------------------------------
sub doc_write_copyright($)
{
  my ($stream)= @_;

  print $stream "<i>(C) 2006-2009, B. Quoitin<br>\n";
  print $stream "Generated on ".localtime(time())."</i>\n";
}

# -----[ doc_write ]-------------------------------------------------
# Generates the documentation file.
# -------------------------------------------------------------------
sub doc_write($$$)
{
  my ($filename,
      $script_version,
      $tests)= @_;

  my $stream= gensym();
  open($stream, ">$filename") or
    die "unable to generate \"$filename\": $!";
  print $stream "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\">\n";
  print $stream "<html>\n";
  print $stream "<head>\n";
  print $stream "<title>C-BGP validation documentation (v$script_version)</title>\n";
  print $stream "</head>\n";
  print $stream "<body>\n";
  print $stream "<h1>C-BGP validation (v$script_version): tests documentation</h1>\n";
  foreach my $test (sort @$tests) {

    my $func= $test->{TEST_FIELD_FUNC};
    my $name= $test->{TEST_FIELD_NAME};
    my $doc= $test->{TEST_FIELD_DOC};

    print $stream "<hr>\n";
    print $stream "<a name=\"$func\"><h2>$name</h2></a>\n";
    print $stream "Status: ";
    doc_write_status($stream, $test->{TEST_FIELD_RESULT});
    print $stream "\n";
    # -- Description --
    doc_write_section_title($stream, "Description:");
    if (exists($doc->{(DOC_DESCRIPTION)})) {
      doc_write_section($stream, $doc->{DOC_DESCRIPTION()});
    }
    # -- Setup --
    doc_write_section_title($stream, "Setup:");
    if (exists($doc->{(DOC_SETUP)})) {
      doc_write_section($stream, $doc->{DOC_SETUP()});
    }
    # -- Topology --
    if (exists($doc->{(DOC_TOPOLOGY)})) {
      doc_write_preformated($stream, $doc->{DOC_TOPOLOGY()});
    }
    # -- Scenario --
    doc_write_section_title($stream, "Scenario:");
    if (exists($doc->{(DOC_SCENARIO)})) {
      doc_write_section($stream, $doc->{DOC_SCENARIO()});
    }
    # -- Resources --
    if (exists($doc->{(DOC_RESOURCES)})) {
      doc_write_section_title($stream, "Resources:");
      doc_write_resources($stream, $doc->{DOC_RESOURCES()});
    }
  }
  print $stream "<hr>\n";
  doc_write_copyright($stream);
  print $stream "</body>\n";
  print $stream "<html>\n";
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

  my $report_intro= "This is a report automatically generated by the C-BGP validation script. The tests performed by the validation script try to cover as much as possible the C-BGP features. These tests are documented <a href=\"$report_prefix-doc.html\">here</a>. Your suggestions for improving this validation are highly welcome ! The main result for this validation is: ";

  if ($num_failures == 0) {
    $report_intro.= "<b><font color=\"green\">ALL TESTS PASSED :-)</font></b>\n";
  } else {
    $report_intro.= "<b><font color=\"red\">SOME TESTS FAILED :-(</font></b>\n";
  }

  doc_write("$report_prefix-doc.html", $program_version, $tests->{'list'});

  my $report_file_name= "$report_prefix.html";
  open(REPORT, ">$report_file_name") or
    die "could not create HTML report in \"$report_file_name\": $!";
  print REPORT "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\">\n";
  print REPORT "<html>\n";
  print REPORT "<head>\n";
  print REPORT "<title>C-BGP validation test report (v$program_version)</title>\n";
  print REPORT "</head>\n";
  print REPORT "<body>\n";
  print REPORT "<h2>C-BGP validation test report (v$program_version)</h2>\n";
  print REPORT "$report_intro\n";
  print REPORT "<h3>Configuration</h3>\n";
  print REPORT "<ul>\n";
  print REPORT "<li>Arguments: $program_args</li>\n";
  print REPORT "<li>C-BGP version: ".(defined($cbgp_version)?$cbgp_version:"undef")."</li>\n";
  print REPORT "<li>C-BGP path: $cbgp_path</li>\n";
  print REPORT "<li>libGDS version: ".(defined($libgds_version)?$libgds_version:"undef")."</li>\n";
  print REPORT "<li>Number of failures: $num_failures";
  ($tests->{'max-failures'} > 0) and
    print REPORT " (max: ".$tests->{'max-failures'}.")";
  print REPORT "</li>\n";
  print REPORT "<li>Number of warnings: $num_warnings";
  ($tests->{'max-warnings'} > 0) and
    print REPORT " (max: ".$tests->{'max-warnings'}.")";
  print REPORT "</li>\n";
  print REPORT "<li>From cache: ".
    (defined($tests->{'cache-enabled'})?"yes":"no").
      "</li>\n";
  print REPORT "<li>Test duration: ".$tests->{'duration'}." secs.</li>\n";
  my $time= POSIX::strftime("%Y/%m/%d %H:%M", localtime(time()));
  print REPORT "<li>Finished: $time</li>\n";
  print REPORT "<li>System: ".`uname -m -p -s -r`."</li>\n";
  print REPORT "</ul>\n";
  print REPORT "<h3>Results</h3>\n";
  print REPORT "<table border=\"1\">\n";
  print REPORT "<tr>\n";
  print REPORT "<th>Test ID</th>\n";
  print REPORT "<th>Test description</th>\n";
  print REPORT "<th>Result</th>\n";
  print REPORT "<th>Duration (s.)</th>\n";
  print REPORT "</tr>\n";
  foreach my $test_record (@{$tests->{'list'}}) {
    my $test_id= $test_record->{TEST_FIELD_ID};
    my $test_name= $test_record->{TEST_FIELD_NAME};
    my $test_func= $test_record->{TEST_FIELD_FUNC};
    my $test_result= $test_record->{TEST_FIELD_RESULT};
    my $test_duration= $test_record->{TEST_FIELD_DURATION};
    my $test_doc= $test_record->{TEST_FIELD_DOC};

    my $doc_ref= undef;
    if (defined($test_doc)) {
      $doc_ref= "$report_prefix-doc.html#$test_func";
    } else {
      show_warning("no documentation for \"$test_name\" ($test_func)");
    }

    (!defined($test_duration)) and
      $test_duration= "---";

    print REPORT "<tr>\n";
    print REPORT "<td align=\"center\">$test_id</td>\n";
    print REPORT "<td>\n";
    (defined($doc_ref)) and
      print REPORT "<a href=\"$doc_ref\">\n";
    print REPORT "$test_name\n";
    (defined($doc_ref)) and
      print REPORT "</a>\n";
    print REPORT "</td>\n";
    print REPORT "<td align=\"center\">";
    doc_write_status(*REPORT, $test_result);
    print REPORT "</td>\n";
    print REPORT "<td align=\"center\">$test_duration</td>\n";
    print REPORT "</tr>\n";
  }
  print REPORT "</table>\n";
  print REPORT "<hr>\n";
  doc_write_copyright(*REPORT);
  print REPORT "</body>\n";
  print REPORT "</html>\n";
  close(REPORT);
}

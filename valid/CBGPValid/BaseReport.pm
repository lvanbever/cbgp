# ===================================================================
# CBGPValid::BaseReport.pm
#
# author Bruno Quoitin (bruno.quoitin@uclouvain.be)
# $Id: BaseReport.pm,v 1.4 2009-06-25 14:36:27 bqu Exp $
# ===================================================================

package CBGPValid::BaseReport;

require Exporter;
@ISA= qw(Exporter);
@EXPORT= qw(
	    DOC_DESCRIPTION
	    DOC_RESOURCES
	    DOC_SCENARIO
	    DOC_SETUP
	    DOC_TOPOLOGY
	   );

use strict;
use Symbol;
use CBGPValid::TestConstants;
use CBGPValid::UI;

my %resources= ();
1;

use constant STATE_INIT   => 0;
use constant STATE_IN_DOC => 1;
use constant STATE_FINAL  => 2;

use constant DOC_DESCRIPTION => 'Description';
use constant DOC_RESOURCES   => 'Resources';
use constant DOC_SCENARIO    => 'Scenario';
use constant DOC_SETUP       => 'Setup';
use constant DOC_TOPOLOGY    => 'Topology';

# -----[ doc_from_script ]-------------------------------------------
# Parses the given Perl script in order to document the validation
# tests.
#
# The function looks for the following sections:
#   - Description: general description of the test
#   - Setup: how C-BGP is configured for the test (topology and config)
#   - Topology: an optional "ASCII-art" topology
#   - Scenario: what is announced and what is tested
#   - Resources: an optional list of resources (files/URLs)
# -------------------------------------------------------------------
sub doc_from_script($)
{
  my ($script_name)= @_;

  return undef
    if (!open(SCRIPT, "<$script_name"));
  my $state= STATE_INIT;
  my $section= undef;
  my $name= undef;
  my $doc= undef;
  while (<SCRIPT>) {
    if ($state == STATE_INIT) {
      if (m/^# -----\[\s*cbgp_valid_([^ ]*)\s*\]/) {
	my $func_name= "cbgp_valid_$1";
	#if (exists($tests_index->{$func_name})) {
	$state= 1;
	$name= $func_name;
	$doc= { 'Name' => $1 };
	$section= DOC_DESCRIPTION;
	$doc->{$section}= [];
      }
    } elsif ($state == STATE_IN_DOC) {
      if ((!m/^#/) || (m/^# ----------.*/)) {
	$state= STATE_FINAL;
      } elsif (m/^# Description\:/) {
	$section= DOC_DESCRIPTION;
	$doc->{$section}= [];
      } elsif (m/^# Setup\:/) {
	$section= DOC_SETUP;
	$doc->{$section}= [];
      } elsif (m/^# Scenario\:/) {
	$section= DOC_SCENARIO;
	$doc->{$section}= [];
      } elsif (m/^# Topology\:/) {
	$section= DOC_TOPOLOGY;
	$doc->{$section}= [];
      } elsif (m/^# Resources\:/) {
	$section= DOC_RESOURCES;
	$doc->{$section}= [];
      } else {
	if (m/^#(.*$)/) {
	  push @{$doc->{$section}}, ($1);
	} else {
	  show_warning("I don't know how to handle \"$_\"\n");
	}
      }
    } elsif ($state == STATE_FINAL) {
    } else {
      die "invalid state \"$state\"";
    }
  }
  close(SCRIPT);

  # Post-processing
  #for my $doc_item (values %doc) {
  #  (exists($doc_item->{'Resources'})) and
  #    $doc_item->{'Resources'}=
  #      parse_resources_section($doc_item->{'Resources'});
  #}

  return $doc;
}

# -----[ parse_resources_section ]-----------------------------------
#
# -------------------------------------------------------------------
sub parse_resources_section($)
{
  my ($section)= @_;

  my @section_resources= ();

  foreach my $line (@$section) {
    if ($line =~ m/^\s*\[(\S+)(\s+[^\]])?\]\s*$/) {
      my $ref= $1;
      my $name= $2;
      (!defined($2)) and $name= $ref;
      $resources{$ref}= 1;
      push @section_resources, ([$ref, $name]);
    } else {
      show_warning("invalid resource \"$line\" (skipped)");
    }
  }


  return \@section_resources;
}

# -----[ save_resources ]--------------------------------------------
#
# -------------------------------------------------------------------
sub save_resources($)
{
  my ($file)= @_;

  open(RESOURCES, ">$file") or
    die "unable to create resources file \"$file\": $!";
  foreach my $ref (sort keys %resources) {
    print RESOURCES "$ref\n";
  }
  close(RESOURCES);
}

# ===================================================================
# CBGPValid::UI.pm
#
# author Bruno Quoitin (bruno.quoitin@uclouvain.be)
# $Id: UI.pm,v 1.2 2009-06-25 14:36:27 bqu Exp $
# ===================================================================

package CBGPValid::UI;

require Exporter;
@ISA= qw(Exporter);
@EXPORT= qw(COLOR_DEFAULT
	    COLOR_RED
	    COLOR_WHITE
	    COLOR_YELLOW
	    show_error
	    show_warning
	    show_info
	    );

use strict;

use constant COLOR_DEFAULT => "\033[0m";
use constant COLOR_RED => "\033[1;31m";
use constant COLOR_WHITE => "\033[37;1m";
use constant COLOR_YELLOW => "\033[1;33m";

# -----[ show_error ]------------------------------------------------
sub show_error($)
{
  my ($msg)= @_;

  print STDERR "Error: ".COLOR_RED()."$msg".COLOR_DEFAULT()."\n";
}

# -----[ show_warning ]----------------------------------------------
sub show_warning($)
{
  my ($msg)= @_;

  print STDERR "Warning: ".COLOR_YELLOW."$msg".COLOR_DEFAULT."\n";
  #$validation->{'stat_num_warnings'}++;
}

# -----[ show_info ]-------------------------------------------------
sub show_info($)
{
  my ($msg)= @_;

  print STDERR "Info: ".COLOR_WHITE."$msg".COLOR_DEFAULT."\n";
}

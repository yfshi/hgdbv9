# Copyright (c) 2021, PostgreSQL Global Development Group

package TestLib;

use strict;
use warnings;

use Carp;
use Config;
use Cwd;
use Exporter 'import';
use Fcntl qw(:mode :flock :seek :DEFAULT);
use File::Basename;
use File::Path qw(mkpath);
use File::Temp ();
use Socket;

our @EXPORT = qw(
  get_free_port
);

# Global variables
our ($use_tcp, $windows_os, $use_unix_sockets, $portdir, $tmp_check);
my @port_reservation_files;

# Port range configuration
my $port_lower_bound = 10200;
my $port_upper_bound = 32767;
my $last_port_assigned;

INIT
{
	# Detect OS
	$windows_os = ($Config{osname} eq 'MSWin32' || $Config{osname} eq 'msys');
	$use_unix_sockets = !$windows_os;
	$use_tcp = !$use_unix_sockets;
	
	# Initialize port tracking
	my $num_ports = $port_upper_bound - $port_lower_bound;
	$last_port_assigned = int(rand() * $num_ports) + $port_lower_bound;
	
	# Set the port lock directory
	$portdir = $ENV{PG_TEST_PORT_DIR};
	$portdir ||= getcwd() . "/portdir";
	$portdir =~ s!\\!/!g;
	# Make sure the directory exists
	mkpath($portdir) unless -d $portdir;
}

END
{
	unlink @port_reservation_files;
}

sub get_free_port
{
	my $found = 0;
	my $port  = $last_port_assigned;

	while ($found == 0)
	{
		# advance $port, wrapping correctly around range end
		$port = $port_lower_bound if ++$port > $port_upper_bound;
		print "# Checking port $port\n";

		# Check to see if anything else is listening on this TCP port.
		# Seek a port available for all possible listen_addresses values,
		# so callers can harness this port for the widest range of purposes.
		$found = 1;
		foreach my $addr (qw(127.0.0.1),
			($use_tcp && $windows_os)
			  ? qw(127.0.0.2 127.0.0.3 0.0.0.0)
			  : ())
		{
			if (!can_bind($addr, $port))
			{
				$found = 0;
				last;
			}
		}
		$found = _reserve_port($port) if $found;
	}

	print "# Found port $port\n";

	# Update port for next time
	$last_port_assigned = $port;

	return $port;
}

# Internal routine to check whether a host:port is available to bind
sub can_bind
{
	my ($host, $port) = @_;
	my $iaddr = inet_aton($host);
	my $paddr = sockaddr_in($port, $iaddr);
	my $proto = getprotobyname("tcp");

	socket(SOCK, PF_INET, SOCK_STREAM, $proto)
	  or die "socket failed: $!";

	# As in postmaster, don't use SO_REUSEADDR on Windows
	setsockopt(SOCK, SOL_SOCKET, SO_REUSEADDR, pack("l", 1))
	  unless $windows_os;
	my $ret = bind(SOCK, $paddr) && listen(SOCK, SOMAXCONN);
	close(SOCK);
	return $ret;
}

# Internal routine to reserve a port number
# Returns 1 if successful, 0 if port is already reserved.
sub _reserve_port
{
	my $port = shift;
	# open in rw mode so we don't have to reopen it and lose the lock
	my $filename = "$portdir/$port.rsv";
	sysopen(my $portfile, $filename, O_RDWR|O_CREAT)
	  || die "opening port file $filename: $!";
	# take an exclusive lock to avoid concurrent access
	flock($portfile, LOCK_EX) || die "locking port file $filename: $!";
	# see if someone else has or had a reservation of this port
	my $pid = <$portfile> || "0";
	chomp $pid;
	if ($pid +0 > 0)
	{
		if (kill 0, $pid)
		{
			# process exists and is owned by us, so we can't reserve this port
			flock($portfile, LOCK_UN);
			close($portfile);
			return 0;
		}
	}
	# All good, go ahead and reserve the port
	seek($portfile, 0, SEEK_SET);
	# print the pid with a fixed width so we don't leave any trailing junk
	print $portfile sprintf("%10d\n",$$);
	flock($portfile, LOCK_UN);
	close($portfile);
	push(@port_reservation_files, $filename);
	return 1;
}

1;
#!/usr/bin/env perl

use strict;
use warnings;
use Socket;

my $remote_host = $ARGV[0] || 'localhost';
my $remote_port = $ARGV[1] || 63187;

socket(SOCKET, PF_INET, SOCK_STREAM, getprotobyname('tcp'));

my $internet_addr = inet_aton($remote_host)
	or die "Couldn't convert $remote_host into an Internet address: $!\n";

my $paddr = sockaddr_in($remote_port, $internet_addr);

connect(SOCKET, $paddr)
	or die "Couldn't connect to $remote_host:$remote_port : $!\n";

sub read_view {
	my $view = '';
	my $lines = 0;
	my $line;
	while ($line = <SOCKET>) {
		$view .= $line;
		if ($lines < 1) {
			# we know the view has as many lines as columns
			$lines = length $line;
			# substract the trailing line feed
			--$lines;
		}
		if (--$lines < 1) {
			last;
		}
	}
	return $view;
}

while (1) {
	my $view = read_view() or last;
	print $view;
	print 'Command (q<>^v): ';
	my $cmd = <>;
	$cmd = substr($cmd, 0, 1);
	if ($cmd eq 'q') {
		last;
	}
	send(SOCKET, $cmd, 0);
}

close(SOCKET);

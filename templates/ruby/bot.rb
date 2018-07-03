#!/usr/bin/env ruby -w

require 'io/console'
require 'socket'

def read_view()
	view = $socket.gets
	# we know the view has as many lines as columns minus
	# the trailing line feed
	for _ in 3..view.length do
		view += $socket.gets
	end
	view
end

$socket = TCPSocket.open(ARGV[0] || 'localhost', ARGV[1] || 63187)

while true
	view = read_view
	print view
	print "Command (q<>^v): "
	STDOUT.flush
	cmd = STDIN.getch
	puts
	case cmd
	when 'q'
		break
	end
	$socket.putc cmd
end

$socket.close

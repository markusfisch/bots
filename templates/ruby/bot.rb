#!/usr/bin/env ruby -w

require 'io/console'
require 'socket'

def read_view()
	view = ""
	lines = 0
	while line = $socket.gets
		if lines < 1
			lines = line.length - 1
		end
		view += line
		lines -= 1
		if lines < 1
			break
		end
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
	else
		$socket.putc cmd
	end
end

$socket.close

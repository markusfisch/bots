require 'io/console'
require 'socket'

def read_map()
	lines = 0
	while line = $socket.gets
		if lines < 1
			lines = line.length - 1
		end
		puts line
		lines -= 1
		if lines < 1
			break
		end
	end
end

$socket = TCPSocket.open(ARGV[0] || 'localhost', ARGV[1] || 63187)

read_map
while true
	print "Command (q<>^v): "
	STDOUT.flush
	cmd = STDIN.getch
	puts
	case cmd
	when 'q'
		break
	else
		$socket.putc cmd
		read_map
	end
end

$socket.close

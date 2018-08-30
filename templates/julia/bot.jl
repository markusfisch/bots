using Sockets

function readview(sock)
	view = readline(sock)
	view == "" && return
	size = length(view) - 1
	view *= "\n"
	for i = 1:size
		line = readline(sock) * "\n"
		view = view * line
	end
	return view
end

sock = connect(get(ARGS, 1, "localhost"), get(ARGS, 2, 63187))
while isopen(sock)
	view = readview(sock)
	view == "" && break
	print(view, "Command (q<>^v): ")
	cmd = read(stdin, Char)
	print("\n")
	if cmd == 'q'
		break
	elseif cmd == '\n'
		cmd = '^'
	end
	write(sock, cmd)
end
close(sock)

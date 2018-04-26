import java.io.BufferedReader
import java.net.Socket

private fun readMap(input: BufferedReader) {
	var lines = 0
	while (true) {
		val line = input.readLine();
		if (line == null) {
			break
		}
		if (lines < 1) {
			lines = line.length
		}
		println(line)
		if (--lines < 1) {
			break
		}
	}
}

fun main(args: Array<String>) {
	val socket = Socket(
		if (args.size > 0) args[0] else "localhost",
		if (args.size > 1) Integer.parseInt(args[1]) else 63187
	)
	socket.use {
		val output = socket.outputStream
		val input = socket.inputStream.bufferedReader()
		while (true) {
			readMap(input)
			print("Command (q<>^v): ")
			var ch: Int
			do {
				ch = System.`in`.read()
			} while (ch == '\n'.toInt())
			if (ch == 'q'.toInt()) {
				break
			}
			output.write(ch)
		}
	}
}

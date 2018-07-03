import java.io.BufferedReader
import java.net.Socket

private fun printView(view: View) {
	var i = 0
	while (i < view.view.length) {
		println(view.view.substring(i, i + view.width))
		i += view.width
	}
}

private fun readView(input: BufferedReader): View {
	var view = input.readLine()
	var width = view.length
	for (i in 1 until width) {
		view += input.readLine()
	}
	return View(view, width)
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
			val view = readView(input)
			printView(view)
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

data class View(val view: String, val width: Int)

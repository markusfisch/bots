package bots.client;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.IOException;
import java.io.OutputStream;
import java.net.Socket;
import java.net.UnknownHostException;

public class BotsClient {
	public static void main(String args[]) {
		final String host = args.length > 0 ? args[0] : "localhost";
		final int port = args.length > 1 ? Integer.parseInt(args[1]) : 63187;
		Socket socket = null;
		try {
			socket = new Socket(host, port);
			OutputStream out = socket.getOutputStream();
			BufferedReader in = new BufferedReader(
					new InputStreamReader(socket.getInputStream(), "UTF-8"));
			while (true) {
				readMap(in);
				System.out.print("Command (q<>^v): ");
				int ch;
				while ((ch = System.in.read()) == '\n');
				if (ch == 'q') {
					break;
				}
				out.write(ch);
			}
		} catch (IOException e) {
			e.printStackTrace();
		} finally {
			if (socket != null) {
				try {
					socket.close();
				} catch (IOException e) {
					e.printStackTrace();
				}
			}
		}
	}

	private static void readMap(BufferedReader in) throws IOException {
		int lines = 0;
		for (String line; (line = in.readLine()) != null;) {
			if (lines < 1) {
				lines = line.length();
			}
			System.out.println(line);
			if (--lines < 1) {
				break;
			}
		}
	}
}

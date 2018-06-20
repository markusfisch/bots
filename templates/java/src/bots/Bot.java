package bots;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.IOException;
import java.io.OutputStream;
import java.net.Socket;

public class Bot {
	public static void main(String args[]) {
		final String host = args.length > 0 ? args[0] : "localhost";
		final int port = args.length > 1 ? Integer.parseInt(args[1]) : 63187;
		try (Socket socket = new Socket(host, port)) {
			OutputStream out = socket.getOutputStream();
			BufferedReader in = new BufferedReader(
					new InputStreamReader(socket.getInputStream(), "UTF-8"));
			View view = new View();
			while (true) {
				view.read(in);
				view.print();
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
		}
	}

	private static class View {
		private String data;
		private int width;

		private void read(BufferedReader in) throws IOException {
			StringBuilder sb = new StringBuilder();
			int lines = 0;
			for (String line; (line = in.readLine()) != null;) {
				if (lines < 1) {
					width = lines = line.length();
				}
				sb.append(line);
				if (--lines < 1) {
					break;
				}
			}
			data = sb.toString();
		}

		private void print() {
			if (data == null || width < 1) {
				return;
			}
			for (int i = 0, len = data.length(); i < len; i += width) {
				System.out.println(data.substring(i, i + width));
			}
		}
	}
}

using System;
using System.IO;
using System.Text;
using System.Net.Sockets;
using System.Collections.Generic;

class Bot {
	static string ReadView(StreamReader reader) {
		string view = reader.ReadLine();
		if (view == null) {
			return null;
		}
		view += "\n";
		int expectedNumberOfLines = view.Length - 1;
		for (int i = 0; i < expectedNumberOfLines; ++i) {
			view += reader.ReadLine() + "\n";
		}
		return view;
	}

	static void Main(string[] args) {
		string address = args.Length > 0 ? args[0] : "localhost";
		int port = args.Length > 1 ? Int32.Parse(args[1]) : 63187;

		TcpClient tcpClient = new TcpClient();
		try {
			tcpClient.Connect(address, port);
			StreamWriter writer = new StreamWriter(tcpClient.GetStream(),
				Encoding.ASCII);
			writer.AutoFlush = true;
			StreamReader reader = new StreamReader(tcpClient.GetStream(),
				Encoding.ASCII);
			string view;
			while ((view = ReadView(reader)) != null) {
				Console.Write(view);
				Console.Write("Command (q<>^v): ");
				var key = Console.ReadKey(false).KeyChar;
				if (key == 'q') {
					break;
				} else {
					writer.Write(key);
				}
				Console.WriteLine();
			}
		} catch (Exception e) {
			Console.WriteLine(e.ToString());
		} finally {
			tcpClient.Close();
		}
	}
}

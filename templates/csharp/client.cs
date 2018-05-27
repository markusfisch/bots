using System;
using System.IO;
using System.Text;
using System.Net.Sockets;
using System.Collections.Generic;

class Bots {
	static List<string> ReadMap(StreamReader reader) {
		string firstLine = reader.ReadLine();
		
		if(firstLine == null) {
			return null;
		}
		
		List<string> map = new List<string> { firstLine };
		int expectedNumberOfLines = firstLine.Length - 1;
		for(int i = 0; i < expectedNumberOfLines; ++i) {
			map.Add(reader.ReadLine());
		}
		return map;
	}
	
	static void Main(string[] args) {
		const int defaultPort = 63187;
		string address = args.Length > 0 ? args[0] : "localhost";
		int port = args.Length > 1 ? Int32.Parse(args[1]) : defaultPort;
		
		TcpClient tcpClient = new TcpClient();
		try {
			tcpClient.Connect(address, port);
			StreamWriter writer = new StreamWriter(tcpClient.GetStream(), Encoding.ASCII);
			writer.AutoFlush = true;
			StreamReader reader = new StreamReader(tcpClient.GetStream(), Encoding.ASCII);
			List<string> map;
			while((map = ReadMap(reader)) != null) {
				Console.WriteLine(String.Join("\n", map));
				Console.Write("Command (q<>^v): ");
				var key = Console.ReadKey(false).KeyChar;
				if(key == 'q') {
					break;
				} else {
					writer.Write(key);
				}
				Console.WriteLine();
			}
		}
		catch (Exception e) {
			Console.WriteLine(e.ToString());
		}
		finally {
			tcpClient.Close();
		}
	}
}

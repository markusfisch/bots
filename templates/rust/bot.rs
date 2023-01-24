use std::env;
use std::net::{TcpStream};
use std::io::{BufRead, BufReader, Write};

fn main() {
	let args: Vec<String> = env::args().collect();
	let len = args.len();
	let host = if len > 1 { &args[1] } else { "localhost" };
	let port = if len > 2 { &args[2] } else { "63187" };
	let mut address = String::new();
	address.push_str(host);
	address.push_str(":");
	address.push_str(port);
	match TcpStream::connect(address) {
		Ok(mut stream) => {
			let mut stdin_reader = BufReader::new(std::io::stdin());
			let mut stream_reader = BufReader::new(stream.try_clone().unwrap());
			loop {
				let mut view = String::new();
				let count = stream_reader.read_line(&mut view).unwrap();
				if count < 1 {
					break;
				}
				// The view has as many lines as columns, minus the
				// trailing line feed.
				for _ in 2..count {
					if stream_reader.read_line(&mut view).unwrap() != count {
						break;
					}
				}
				print!("{}", view);
				print!("Command (q<>^v): ");
				std::io::stdout().flush().unwrap();
				let mut cmd = String::new();
				if stdin_reader.read_line(&mut cmd).unwrap() < 1 {
					break;
				}
				let ch = cmd.chars().next().unwrap();
				if ch == 'q' {
					break;
				}
				stream.write(&[ch as u8]).unwrap();
			}
		},
		Err(e) => {
			println!("Failed to connect: {}", e);
		}
	}
}

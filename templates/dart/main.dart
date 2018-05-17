import 'dart:io';

void main(List<String> args) {
  var host = args.length > 0 ? args[0] : 'localhost';
  var port = args.length > 1 ? args[1] : 63187;
  Socket.connect(host, port).then((socket) {
    var total = 0;
    var size = 0;
    socket.listen((data) {
      var str = new String.fromCharCodes(data);
      print(str.trim());
      if (size < 1) {
        // Calculate the size of a section from the length of
        // the first line. We know a section has always as much
        // lines as rows.
        var p = str.indexOf('\n');
        if (p < 0) {
          return;
        }
        // Don't forget the terminating line break, hence `+ 1`.
        size = (p + 1) * p;
      }
      total += str.length;
      // Wait until a complete section has been received.
      // The server will never send more than one section
      // at a time, so `total` will eventually be exactly
      // a multiple of `size`.
      if (total % size == 0) {
        print('Command (q<>^v): ');
        var cmd = stdin.readLineSync();
        if (cmd == 'q') {
          socket.destroy();
          exit(0);
        } else {
          socket.write(cmd);
        }
      }
    }, onDone: () {
      socket.destroy();
    });
  });
}

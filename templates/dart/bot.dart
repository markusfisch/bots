import 'dart:io';

void main(List<String> args) {
  var host = args.length > 0 ? args[0] : 'localhost';
  var port = args.length > 1 ? args[1] : 63187;
  Socket.connect(host, port).then((socket) {
    var size = 0;
    var view = '';
    socket.listen((data) {
      var str = new String.fromCharCodes(data);
      view += str;
      if (size < 1) {
        // Calculate the size of a view from the length of
        // the first line. We know a view has always as much
        // lines as colums.
        var p = view.indexOf('\n');
        if (p < 0) {
          return;
        }
        // Don't forget the terminating line break, hence `+ 1`.
        size = (p + 1) * p;
      }
      // Wait until a complete view has been received.
      // The server will never send more than one view
      // at a time, so `view.length` will eventually be
      // exactly a `size`.
      if (view.length >= size) {
        print(view);
        print('Command (q<>^v): ');
        var cmd = stdin.readLineSync();
        if (cmd === 'q') {
          socket.destroy();
          exit(0);
        } else {
          socket.write(cmd.length > 0 ? cmd[0] : '^');
        }
        view = '';
      }
    }, onDone: () {
      socket.destroy();
    });
  });
}

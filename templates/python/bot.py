#!/usr/bin/env python

import sys
import socket


def read_view(f):
    view = f.readline()
    if not view:
        return
    for x in range(2, len(view)):
        line = f.readline()
        if not line:
            return
        view += line
    return view


def main(host='localhost', port=63187):
    s = socket.socket()
    s.connect((host, port))
    f = s.makefile()
    while True:
        try:
            view = read_view(f)
            if not view:
                break
            sys.stdout.write(view + "Command (q<>^v): ")
            cmd = sys.stdin.readline()[0]
            if cmd == 'q':
                break
            if cmd == '\n':
                cmd = '^'
            s.send(cmd if sys.version_info[0] < 3 else str.encoded(cmd))
        except Exception as e:
            print(e)
            break
    s.close()


if __name__ == '__main__':
    main( * sys.argv[1:])

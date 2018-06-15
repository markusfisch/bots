#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import socket


def read_map(f):
    view = ''
    total = 0
    width = -1
    while True:
        view += f.readline()
        if width <= 0:
            width = len(view)
            if width > 0:
                total = width * (width - 1)
        if len(view) == total:
            break
    return view


def main(host='localhost', port=63187):
    s = socket.socket()
    s.connect((host, port))
    f = s.makefile()
    while True:
        try:
            view = read_map(f)
            if not view:
                break
            print(view)
            print("Command (q<>^v): ")
            cmd = sys.stdin.readline()
            if cmd == 'q':
                break
            else:
                s.send(cmd[0] if cmd[0] != '\n' else '^')
        except:
            break
    s.close()

if __name__ == '__main__':
    main( * sys.argv[1:])

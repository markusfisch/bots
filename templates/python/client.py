#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import socket


def read_map(s):
    view = ''
    total = 0
    width = 0
    while True:
        data = s.recv(1024)
        if not data:
            # Server closed the connection, possibly end of game or
            # exceptional situation
            break
        view += data
        
        if width <= 0:
            width = view.find('\n')
            if width > 0:
                total = (width + 1) * width
        if len(view) == total:
            break
    return view


def main(host='localhost', port=63187):
    s = socket.socket()
    s.connect((host, port))
    while True:
        try:
            view = read_map(s)
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

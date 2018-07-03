import os, net, strutils

let args = commandLineParams()
let s = newSocket()
s.connect(
  if args.len > 0: args[0] else: "localhost",
  Port(if args.len > 1: parseInt(args[1]) else: 63187))

proc readView: string =
  result = s.recvLine()
  if result == "": return ""
  let size = result.len
  result.add("\n")
  for n in 1 ..< size:
    let line = s.recvLine()
    if line == "": return ""
    result.add(line)
    result.add("\n")

while true:
  let view: string = readView()
  if view == "": break
  stdout.write view, "Command (q<>^v): "
  var cmd = readLine(stdin)
  case cmd
  of "q": break
  of "": cmd = "^"
  s.send(cmd)

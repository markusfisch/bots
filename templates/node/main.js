'use strict'

const readline = require('readline')
const rl = readline.createInterface({
  input: process.stdin,
  output: process.stdout
})

const net = require('net')
const client = new net.Socket()
client.setEncoding('utf-8')
client.connect(
  process.argv[3] || 63187,
  process.argv[2] || '127.0.0.1'
)

var total = 0
var size = 0
client.on('data', function (data) {
  process.stdout.write(data)
  if (size < 1) {
    // Calculate the size of a section from the length of
    // the first line. We know a section has always as much
    // lines as rows.
    let p = data.indexOf('\n')
    if (p < 0) {
      return
    }
    // Don't forget the terminating line break, hence `+ 1`.
    size = (p + 1) * p
  }
  total += data.length
  // Wait until a complete section has been received.
  if (total >= size) {
    rl.question('Command (q<>^v): ', sendCommand)
    total -= size
  }
})

function sendCommand(data) {
  for (let i = 0, len = data.length; i < len; i++) {
    let ch = data.charAt(i)
    switch (ch) {
      default:
        client.write(ch)
        break
      case '\n':
        continue
      case 'q':
        rl.close()
        client.destroy()
        process.exit()
    }
  }
}

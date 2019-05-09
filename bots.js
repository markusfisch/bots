'use strict'

function bots(host, process) {
	if (!'WebSocket' in window) {
		alert('Sorry, this browser cannot open a WebSocket.')
		return false
	}
	if (host.indexOf(':') < 0) {
		host += ':63188'
	}
	const ws = new WebSocket('ws://' + host)
	ws.onmessage = function (event) {
		const cmd = process(event.data)
		if (cmd) {
			ws.send(cmd)
		} else {
			ws.close(1000)
		}
	}
	return true
}

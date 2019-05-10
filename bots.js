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
	function send(cmd) {
		ws.send(cmd)
	}
	function close() {
		ws.close(1000)
	}
	ws.onmessage = function (event) {
		process(event.data, send, close)
	}
	return true
}

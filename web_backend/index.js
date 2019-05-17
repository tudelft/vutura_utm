const io = require('socket.io')();
var nano = require('nanomsg');

var utm_sp_req = nano.socket('req');
utm_sp_req.connect('ipc:///tmp/utmsp_0');

utm_sp_req.on('data', function (buf) {
	console.log('received response: ', buf.toString());
	io.emit('utmsp_response', buf.toString());
});

io.on('connection', function(client) {
	console.log("New client");

	client.on('test', () => {
		console.log("Received test");
	});

	client.on('request_flight', () => {
		console.log("Request flight");
		utm_sp_req.send('test command');		
	});
});

console.log("Start listening");
io.listen(3000);

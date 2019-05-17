const io = require('socket.io')();
var nano = require('nanomsg');

io.on('connection', function(client) {
	console.log("New client");
});

console.log("Start listening");
io.listen(3000);

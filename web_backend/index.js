const io = require('socket.io')();
var nano = require('nanomsg');

var utm_sp_req = nano.socket('req');
utm_sp_req.connect('ipc:///tmp/utmsp_0');

var utm_status_sub = nano.socket('sub');
utm_status_sub.connect('ipc:///tmp/utm_status_update_0')

var gps_json_sub = nano.socket('sub');
gps_json_sub.connect('ipc:///tmp/gps_json_0')

var mavlink_command_pub = nano.socket('pub');
mavlink_command_pub.bind('ipc:///tmp/direct_mav_command_sub_0');

utm_sp_req.on('data', function (buf) {
	console.log('received response: ', buf.toString());
	io.emit('utmsp_response', buf.toString());
});

utm_status_sub.on('data', function (buf) {
	console.log('received state update: ', buf.toString());
	io.emit('state_update', buf.toString());
});

gps_json_sub.on('data', function (buf) {
	//console.log('received gps: ', buf.toString());
	io.emit('position_update', buf.toString());
});

io.on('connection', function(client) {
	console.log("New client");

	client.on('test', () => {
		console.log("Received test");
	});

	client.on('request_flight', () => {
		console.log("Request flight");
		utm_sp_req.send('request_flight');		
	});
	
	client.on('start_flight', () => {
		utm_sp_req.send('start_flight');
	});

	client.on('end_flight', () => {
		utm_sp_req.send('end_flight');
	});

	client.on('request_autoflight', () => {
		utm_sp_req.send('request_autoflight');
	});

	client.on('abort_autoflight', () => {
		utm_sp_req.send('abort_autoflight');
	});

	client.on('kill kill kill', () => {
		console.log('kill kill kill');
		mavlink_command_pub.send('kill kill kill');
	});

	client.on('pause', () => {
		console.log('pause');
		mavlink_command_pub.send('pause');
	});

	client.on('continue', () => {
		console.log('continue');
		mavlink_command_pub.send('continue');
	});

	client.on('land here', () => {
		console.log('land here');
		mavlink_command_pub.send('land here');
	});
});

console.log("Start listening");
io.listen(3000);

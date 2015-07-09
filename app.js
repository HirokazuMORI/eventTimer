var EventEmitter = require('events').EventEmitter
  , eventTimer = require('./build/Debug/eventTimer_v10');
eventTimer.MyObject.prototype.__proto__ = EventEmitter.prototype;

var interval = 10;
var count = 0;
var obj1 = new eventTimer.MyObject('object 1');

obj1.on('interval', function(str) {
	if(global.gc){
		global.gc();
	}
if(++count == 100){
		console.log(process.memoryUsage());
		count=0;
	}});


obj1.start(interval);

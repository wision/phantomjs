/*jslint sloppy: true, nomen: true */
/*global exports:true,phantom:true */

/*
  This file is part of the PhantomJS project from Ofi Labs.

  Copyright (C) 2011 Ariya Hidayat <ariya.hidayat@gmail.com>
  Copyright (C) 2011 Ivan De Marino <ivan.de.marino@gmail.com>
  Copyright (C) 2011 James Roe <roejames12@hotmail.com>
  Copyright (C) 2011 execjosh, http://execjosh.blogspot.com
  Copyright (C) 2011 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
  Author: Milian Wolff <milian.wolff@kdab.com>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

QueueOptions = {
	passive: 0x01,
	durable: 0x02,
	exclusive: 0x4,
	autoDelete: 0x8,
	noWait: 0x10
};

ExchangeOptions = {
	passive: 0x01,
	durable: 0x02,
	autoDelete: 0x4,
	internal: 0x8,
	noWait: 0x10
};


exports.createClient = function (connectionString) {
	var server = phantom.createRabbit();
	var handlers = {};
	var events = {};

	this.readFileBase64 = function(string) {
		return server.readFileBase64(string);
	}

	this.stringToBase64 = function(string) {
		return server.stringToBase64(string);
	}

	this.base64ToString = function(string) {
		return server.base64ToString(string);
	}

    function defineSetter(o, handlerName, signalName) {
        o.__defineSetter__(handlerName, function (f) {
            if (handlers && typeof handlers[signalName] === 'function') {
                try {
                    this[signalName].disconnect(handlers[signalName]);
                } catch (e) {
                }
            }
            handlers[signalName] = f;
            this[signalName].connect(handlers[signalName]);
        });
    }

	this.on = function(e, cb) {
		if(!events[e]) {
			events[e] = [];
		}
		events[e].push(cb);
	}
	this.emit = function(name, arg1) {
		if(!events[name]) {
			return;
		}
		args = Array.prototype.slice.call(arguments, 1);
		events[name].forEach(function(cb) {
			cb.apply(server,args);
		});

	}
	self = this;


    defineSetter(server, "handleConnected", "newConnected");
	server.handleConnected = function() {
		self.emit("ready");
	};

    defineSetter(server, "handleError", "newError");
	server.handleError = function(e) {
		self.emit("error", e);
	};


	//     defineSetter(server, "handleDisconnected", "newDisconnected");
	// server.handleDisconnected = function() {
	// console.log("SMSMSMSMSMSMSMSMS323023r0-24ir-09r2");
	//	// self.emit("ready");
	// };

	this.exchange = function (name, opts, callback) {
		// console.log('xxx');
		var qExchange = server.createExchange();
		var options = 0;
		var type = opts.type;
		delete opts.type;
		var selfExchange = this;

		for(var key in opts) {
			if(!ExchangeOptions[key]) {
				// console.log("oppp", key);
				self.emit('error', "Invalid exchange option: " + key);
				// TODO throw asi nefunguje
				throw new Error ("Invalid exchange option " + key);
			}
			if(opts[key]) {
				options |= ExchangeOptions[key];
			}
		}
		// console.log('options ', options);

		defineSetter(qExchange, "handleDeclared", "newDeclared");
		defineSetter(qExchange, "handleClosed", "newClosed");

		qExchange.handleDeclared = function() {
			callback.call(callback, selfExchange);
		}

		qExchange.handleClosed = function() {
			self.emit('error', "Exchange " + name + " closed - invalid options?");
		}

		qExchange.declare(name, type, options);

		selfExchange.publish = function (routingKey, message, opts, callback) {
			qExchange.publish(JSON.stringify(message), routingKey, "application/json");
		};

	}

	this.queue = function (name, opts, callback) {
		var options = 0;
		var selfQueue = this;

		for(var key in opts) {
			if(!QueueOptions[key]) {
				throw new Error ("Invalid queue option " + key);
			}
			if(opts[key]) {
				options |= QueueOptions[key];
			}
		}

		var qQueue = server.createQueue();
		defineSetter(qQueue, "handleDeclared", "newDeclared");
		defineSetter(qQueue, "handleClosed", "newClosed");

		qQueue.handleDeclared = function() {
			callback.call(callback, selfQueue);
		}

		qQueue.handleClosed = function() {
			self.emit('error', "Queue " + name + " closed - invalid options?");
		}


		qQueue.declare(name, options);

		selfQueue.subscribe = function(options, callback) {
			defineSetter(qQueue, "handleMsg", "newMsg");
			qQueue.handleMsg = function(message) {
				args = Array.prototype.slice.call(arguments, 1);
				try {
					message = JSON.parse(message);
				}
				catch(e) {}

				callback.apply(callback, ([message].concat(args)));


			};


			var prefetchSize = 0;
			var prefetchCount = 0 || options.prefetchCount;

			qQueue.setQOS(prefetchSize, prefetchCount);
			qQueue.consume();
		}

	}

	setTimeout(function(){
		server.openConnection(connectionString);
	});

	return this;
};

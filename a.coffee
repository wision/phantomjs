a = require 'rabbit'
fs = require 'fs'

rabbit = require 'rabbit'

r = rabbit.createClient "amqp://guest:guest@localhost:5672/"
r.on 'error', (e) ->
	console.log "## Error: ", e


r.on 'ready', (a) ->

	r.exchange "phantom_result", {type: 'fanout', durable: no, autoDelete: no}, (exchange) ->
		console.log "mam exchange"


	return
	r.queue "phantom_test", {durable: yes, autoDelete: no}, (queue) ->
		queue.subscribe {prefetchCount: 1}, (message, o) ->
			console.log "##ememememe--->>>>", message
			console.log "MESSAGE ",  JSON.stringify message
			# console.log o.a

			# console.log JSON.stringify o
			# for key, val of	o
			# 	console.log key + ":", val,  "<<<<<<<"
			# 	# for key2, val2 of val2
			# 	# 	console.log "--- ", key2 + ":", val2, "<<<<<<<"






return

cl.mrdej (message, res) ->
	# console.log(arguments.length);
	console.log("js:  mam message", message)
	# console.log("js:  jeste ziju")


	page = require('webpage').create()
	system = require('system');

	page.onConsoleMessage = (msg) ->
		system.stderr.writeLine('console: ' + msg)
	d = new Date()
	page.open message, () ->
		console.log 'otevreno   ' + message
		# console.log
		data = page.renderBase64 'png'
		fs.write "mrmr.png", data


		console.log "done"
		# console.log data
		page.close();

		res.publish data, "mrdka"
		console.log "Took " + (new Date - d)

		res.ack()

		# phantom.exit();
# });
# 	setTimeout () ->
# 		console.log("---------js: ack")
# 		console.log("res je ", res, typeof(res))


		# console.log("js: ackedOOOOOOO")
	# , 500
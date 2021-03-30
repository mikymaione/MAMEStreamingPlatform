(function ()
{
	var __hasProp = {}.hasOwnProperty, __extends = function (child, parent) 
	{
		for (var key in parent)
		{
			if (__hasProp.call(parent, key))
				child[key] = parent[key];
		}

		function ctor()
		{
			this.constructor = child;
		}

		ctor.prototype = parent.prototype;
		child.prototype = new ctor();
		child.__super__ = parent.prototype;

		return child;
	};

	AV.WebSocketSource = (function (_super)
	{
		__extends(WebSocketSource, _super);

		function WebSocketSource(socket)
		{
			this.socket = socket;

			if (typeof WebSocket === "undefined" || WebSocket === null)
				return this.emit('error', 'This browser does not have WebSocket support.');

			if (this.socket.binaryType == null)
			{
				this.socket.close();
				return this.emit('error', 'This browser does not have binary WebSocket support.');
			}

			this.bytesLoaded = 0;
		}

		WebSocketSource.prototype.start = function ()
		{
			return true;
		};

		WebSocketSource.prototype.processData = function (uint8_array)
		{
			buf = new AV.Buffer(uint8_array);

			this.bytesLoaded += buf.length;

			if (this.length)
				this.emit('progress', this.bytesLoaded / this.length * 100);

			return this.emit('data', buf);
		};

		return WebSocketSource;

	})(AV.EventEmitter);

	AV.Asset.fromWebSocket = function (socket)
	{
		var source;
		source = new AV.WebSocketSource(socket);
		return new AV.Asset(source);
	};

	AV.Player.fromWebSocket = function (socket)
	{
		var asset;
		asset = AV.Asset.fromWebSocket(socket);
		return new AV.Player(asset);
	};

}).call(this);

// based on aurora-websocket.js https://github.com/fsbdev/aurora-websocket
// MIT licensed

(function () {

	var __hasProp = {}.hasOwnProperty,
		__extends = function(child, parent) { for (var key in parent) { if (__hasProp.call(parent, key)) child[key] = parent[key]; } function ctor() { this.constructor = child; } ctor.prototype = parent.prototype; child.prototype = new ctor(); child.__super__ = parent.prototype; return child; };

	AV.WebSocketSource = (function(_super) {
		__extends(WebSocketSource, _super);

		function WebSocketSource() { 
			// ctor
		}

		WebSocketSource.prototype.start = function () { 
			return true;
		};

		WebSocketSource.prototype.start = function() {
			return true;
		};

		WebSocketSource.prototype.pause = function() {
			return true;
		};

		WebSocketSource.prototype.reset = function() {
			return true;
		};

		WebSocketSource.prototype._on_data = function(uint_8_Array) {
			let buf = new AV.Buffer(uint_8_Array);

			return this.emit('data', buf);
		};

		return WebSocketSource;

	})(AV.EventEmitter);

	AV.Asset.fromWebSocket = function () {
		let source;
		source = new AV.WebSocketSource();
		return new AV.Asset(source);
	};

	AV.Player.fromWebSocket = function () {
		let asset;
		asset = AV.Asset.fromWebSocket();
		return new AV.Player(asset);
	};

}).call(this);

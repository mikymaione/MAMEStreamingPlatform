/* 
license:BSD-3-Clause
copyright-holders:Michele Maione
============================================================

	mame.js - MAME Cloud Gaming (aka DinoServer 🦕🧡🦖)

============================================================
*/

const uri = "ws://80.183.63.209:8888";
//const uri = "ws://localhost:8888";

let keypress_Listener;

let navigation_menu;
let myCanvas;
let player;
let vw, vh, cw, ch, tw, th;

let there_is_some_inputs = false;

function readRomList()
{
	const rom_list_url = "http://www.maionemiky.it/MAME/roms/list.txt";

	let aFile = new XMLHttpRequest();
	aFile.open("GET", rom_list_url, true);
	aFile.send();

	aFile.onreadystatechange = function ()
	{
		if (aFile.readyState == 4 && aFile.status == 200)
		{
			let enter = aFile.responseText.indexOf('\r\n') > 0 ? '\r\n' : '\n';
			let lines = aFile.responseText.split(enter);

			let table = document.createElement('table');

			let i = 0;
			while (i < lines.length)
			{
				let tr = document.createElement('tr');

				for (let y = 0; y < 3; y++)			
				{
					let s = lines[i].split(';');

					let td = document.createElement('td');
					let figure = document.createElement('figure');
					let img = document.createElement('img');
					let figcaption = document.createElement('figcaption');

					td.className = 'td33';

					img.onclick = new Function('Start("' + s[0] + '", "' + s[1] + '");');
					img.className = 'game';
					img.src = 'roms/' + s[0] + '.png';

					figcaption.innerHTML = s[1] + ', ' + s[2];

					figure.appendChild(img);
					figure.appendChild(figcaption);
					td.appendChild(figure);
					tr.appendChild(td);

					i++;
					if (i == lines.length)
						break;
				}

				table.appendChild(tr);
			}

			document.getElementById("games").appendChild(table);

			/*
			<figure>
				<img onclick="Start('cstlevna', 'Castelvania');" class="game" src="roms/cstlevna.png">
				<figcaption>Castelvania, Konami - 1986</figcaption>
			</figure>
			*/
		}
	}
}

function reportWindowSize()
{
	cw = navigation_menu.clientWidth;
	ch = window.innerHeight;

	if (cw / ch < vw / vh)
	{
		tw = cw;
		th = cw * vh / vw;
	}
	else
	{
		tw = ch * vw / vh;
		th = ch;
	}

	if (cw < vw)
	{
		myCanvas.style.width = tw + 'px';
		myCanvas.style.height = th + 'px';
	}
}

function InputSend(g, t, down)
{
	player.source.socket.send('key:' + (down == true ? 'D:' : 'U:') + g + ':' + t);
}

function InitKeyboard()
{
	const keyboard = 9999;
	
	keypress_Listener = new window.keypress.Listener();
	
	if (keypress_Listener)
	{
		keypress_Listener.register_many([
			{
				"keys": "p",
				"on_keydown": function () { InputSend(keyboard, 'PAUSE', true); },
				"on_keyup": function (e) { InputSend(keyboard, 'PAUSE', false); },
			},
			{
				"keys": "1",
				"on_keydown": function () { InputSend(keyboard, 'START', true); },
				"on_keyup": function (e) { InputSend(keyboard, 'START', false); },
			},
			{
				"keys": "2",
				"on_keydown": function () { InputSend(keyboard, 'SELECT', true); },
				"on_keyup": function (e) { InputSend(keyboard, 'SELECT', false); },
			},

			{
				"keys": "left",
				"on_keydown": function () { InputSend(keyboard, 'LEFT', true); },
				"on_keyup": function (e) { InputSend(keyboard, 'LEFT', false); },
			},
			{
				"keys": "right",
				"on_keydown": function () { InputSend(keyboard, 'RIGHT', true); },
				"on_keyup": function (e) { InputSend(keyboard, 'RIGHT', false); },
			},
			{
				"keys": "up",
				"on_keydown": function () { InputSend(keyboard, 'UP', true); },
				"on_keyup": function (e) { InputSend(keyboard, 'UP', false); },
			},
			{
				"keys": "down",
				"on_keydown": function () { InputSend(keyboard, 'DOWN', true); },
				"on_keyup": function (e) { InputSend(keyboard, 'DOWN', false); },
			},

			{
				"keys": "w",
				"on_keydown": function () { InputSend(keyboard, 'X', true); },
				"on_keyup": function (e) { InputSend(keyboard, 'X', false); },
			},
			{
				"keys": "e",
				"on_keydown": function () { InputSend(keyboard, 'Y', true); },
				"on_keyup": function (e) { InputSend(keyboard, 'Y', false); },
			},
			{
				"keys": "s",
				"on_keydown": function () { InputSend(keyboard, 'A', true); },
				"on_keyup": function (e) { InputSend(keyboard, 'A', false); },
			},
			{
				"keys": "d",
				"on_keydown": function () { InputSend(keyboard, 'B', true); },
				"on_keyup": function (e) { InputSend(keyboard, 'B', false); },
			},

			{
				"keys": "q",
				"on_keydown": function () { InputSend(keyboard, 'L1', true); },
				"on_keyup": function (e) { InputSend(keyboard, 'L1', false); },
			},
			{
				"keys": "a",
				"on_keydown": function () { InputSend(keyboard, 'L2', true); },
				"on_keyup": function (e) { InputSend(keyboard, 'L2', false); },
			},
			{
				"keys": "r",
				"on_keydown": function () { InputSend(keyboard, 'R1', true); },
				"on_keyup": function (e) { InputSend(keyboard, 'R1', false); },
			},
			{
				"keys": "f",
				"on_keydown": function () { InputSend(keyboard, 'R2', true); },
				"on_keyup": function (e) { InputSend(keyboard, 'R2', false); },
			},
		]);
		
		document.getElementById('keyboard').src = 'css/keyboard.png';
	}
}

function InitGamepad()
{
	gameControl.on('connect', gamepad =>
	{
		console.log('Gamepad ' + gamepad.id + ' was connected!');

		document.getElementById('gamepad' + gamepad.id).src = 'css/gamepad.png';

		gamepad.before('select', () => { InputSend(gamepad.id, 'SELECT', true) });
		gamepad.before('start', () => { InputSend(gamepad.id, 'START', true) });

		gamepad.before('button12', () => { InputSend(gamepad.id, 'UP', true) });
		gamepad.before('button13', () => { InputSend(gamepad.id, 'DOWN', true) });
		gamepad.before('button15', () => { InputSend(gamepad.id, 'RIGHT', true) });
		gamepad.before('button14', () => { InputSend(gamepad.id, 'LEFT', true) });
		gamepad.before('up', () => { InputSend(gamepad.id, 'UP', true) });
		gamepad.before('down', () => { InputSend(gamepad.id, 'DOWN', true) });
		gamepad.before('right', () => { InputSend(gamepad.id, 'RIGHT', true) });
		gamepad.before('left', () => { InputSend(gamepad.id, 'LEFT', true) });

		gamepad.before('button2', () => { InputSend(gamepad.id, 'X', true) });
		gamepad.before('button3', () => { InputSend(gamepad.id, 'Y', true) });
		gamepad.before('button0', () => { InputSend(gamepad.id, 'A', true) });
		gamepad.before('button1', () => { InputSend(gamepad.id, 'B', true) });

		gamepad.before('l1', () => { InputSend(gamepad.id, 'L1', true) });
		gamepad.before('l2', () => { InputSend(gamepad.id, 'L2', true) });
		gamepad.before('r1', () => { InputSend(gamepad.id, 'R1', true) });
		gamepad.before('r2', () => { InputSend(gamepad.id, 'R2', true) });


		gamepad.after('select', () => { InputSend(gamepad.id, 'SELECT', false) });
		gamepad.after('start', () => { InputSend(gamepad.id, 'START', false) });

		gamepad.after('button12', () => { InputSend(gamepad.id, 'UP', false) });
		gamepad.after('button13', () => { InputSend(gamepad.id, 'DOWN', false) });
		gamepad.after('button15', () => { InputSend(gamepad.id, 'RIGHT', false) });
		gamepad.after('button14', () => { InputSend(gamepad.id, 'LEFT', false) });
		gamepad.after('up', () => { InputSend(gamepad.id, 'UP', false) });
		gamepad.after('down', () => { InputSend(gamepad.id, 'DOWN', false) });
		gamepad.after('right', () => { InputSend(gamepad.id, 'RIGHT', false) });
		gamepad.after('left', () => { InputSend(gamepad.id, 'LEFT', false) });

		gamepad.after('button2', () => { InputSend(gamepad.id, 'X', false) });
		gamepad.after('button3', () => { InputSend(gamepad.id, 'Y', false) });
		gamepad.after('button0', () => { InputSend(gamepad.id, 'A', false) });
		gamepad.after('button1', () => { InputSend(gamepad.id, 'B', false) });

		gamepad.after('l1', () => { InputSend(gamepad.id, 'L1', false) });
		gamepad.after('l2', () => { InputSend(gamepad.id, 'L2', false) });
		gamepad.after('r1', () => { InputSend(gamepad.id, 'R1', false) });
		gamepad.after('r2', () => { InputSend(gamepad.id, 'R2', false) });
	});

	gameControl.on('disconnect', gamepad =>
	{
		console.log('Gamepad ' + gamepad.id + ' was disconnected!');

		document.getElementById('gamepad' + gamepad.id).src = 'css/gamepadD.png';
	});
}

function uuidv4()
{
	return ([1e7] + -1e3 + -4e3 + -8e3 + -1e11).replace(/[018]/g, c =>
		(c ^ crypto.getRandomValues(new Uint8Array(1))[0] & 15 >> c / 4).toString(16));
}

function Start(game, description)
{
	if (there_is_some_inputs)
	{
		const uuid = uuidv4();
		const complete_uri = uri + '?game=' + game + '&id=' + uuid;

		let nav_game = document.createElement('a');
		nav_game.innerHTML = description;
		nav_game.className = 'not-active';

		navigation_menu.append('| ');
		navigation_menu.appendChild(nav_game);

		document.getElementById("games").style.display = "none";
		myCanvas.style.display = "inherit";

		player = new JSMpeg.Player(complete_uri, {
			canvas: myCanvas,
			autoplay: true,
			loop: false,
			pauseWhenHidden: false,
			//videoBufferSize: 128 * 1024,
			//audioBufferSize: 64 * 1024,
		});

		player.source.onStringMessageCallback = function (event)
		{
			let values = event.data.split(":");

			switch (values[0])
			{
				case 'size':
					vw = parseInt(values[1]);
					vh = parseInt(values[2]);

					reportWindowSize();
					break;

				case 'ping':
					player.source.socket.send('ping:' + values[1]);
					break;
			}
		};
	}
	else
	{
		alert('It seems you have neither a keyboard nor a joystick to play!');
	}
}

navigation_menu = document.getElementById("navigation_menu");
myCanvas = document.getElementById("myCanvas");

readRomList();

window.onresize = reportWindowSize;

try 
{
	InitKeyboard();
	there_is_some_inputs = true;
}
catch (error)
{
	alert("No keyboard found: " + error.message);
}

try 
{
	InitGamepad();
	there_is_some_inputs = true;
}
catch (error) 
{
	alert("No gamepad found: " + error.message);
}

# **SDL MAME Cloud Gaming** #

Build status:

| OS/Compiler   | Status        | 
| ------------- |:-------------:| 
| Linux/GCC and Clang | ![CI (Linux)](https://github.com/mikymaione/MAMEStreamingPlatform/workflows/CI%20(Linux)/badge.svg) |
| Windows/GCC and MSVC | ![CI (Windows)](https://github.com/mikymaione/MAMEStreamingPlatform/workflows/CI%20(Windows)/badge.svg) |
| macOS/Clang | ![CI (macOS)](https://github.com/mikymaione/MAMEStreamingPlatform/workflows/CI%20(macOS)/badge.svg) |


What is MAME?
=============

MAME is a multi-purpose emulation framework.

MAME's purpose is to preserve decades of software history. As electronic technology continues to rush forward, MAME prevents this important "vintage" software from being lost and forgotten. This is achieved by documenting the hardware and how it functions. The source code to MAME serves as this documentation. The fact that the software is usable serves primarily to validate the accuracy of the documentation (how else can you prove that you have recreated the hardware faithfully?). Over time, MAME (originally stood for Multiple Arcade Machine Emulator) absorbed the sister-project MESS (Multi Emulator Super System), so MAME now documents a wide variety of (mostly vintage) computers, video game consoles and calculators, in addition to the arcade video games that were its initial focus.

What is SDL MAME Cloud Gaming?
=============

It's a fork of MAME (waiting to be merged with the original project) that implement a Cloud Gaming server.


How to compile?
===============

In every compilation script there is a -jX parameter, replace X with the number of compile job (usually X = number of cores).

If you're on a *NIX or OSX system:

```
compileRelease.sh
```

or for debug:
```
compileDebug.sh
```

How to run?
===============

If you're on a *NIX or OSX system:
```
run.sh
```

or on Windows:
```
run.bat
```


See the [Compiling MAME](http://docs.mamedev.org/initialsetup/compilingmame.html) page on our documentation site for more information, including prerequisites for Mac OS X and popular Linux distributions.

For recent versions of OSX you need to install [Xcode](https://developer.apple.com/xcode/) including command-line tools and [SDL 2.0](https://www.libsdl.org/download-2.0.php).

For Windows users, we provide a ready-made [build environment](http://mamedev.org/tools/) based on MinGW-w64.

Visual Studio builds are also possible, but you still need [build environment](http://mamedev.org/tools/) based on MinGW-w64.
In order to generate solution and project files just run:

```
compile_vs2019_SDL.bat
```
and then open the Visual Studio 2019 solution: ./build/projects/sdl/mame/vs2019/mame.sln


License
=======
The MAME project as a whole is made available under the terms of the
[GNU General Public License, version 2](http://opensource.org/licenses/GPL-2.0)
or later (GPL-2.0+), since it contains code made available under multiple GPL-compatible licenses.  A great majority of the source files (over 90% including core files) are made available under the terms of the
[3-clause BSD License](http://opensource.org/licenses/BSD-3-Clause), and we would encourage new contributors to make their contributions available under the terms of this license.

Please note that MAME is a registered trademark of Gregory Ember, and permission is required to use the "MAME" name, logo, or wordmark.

<a href="http://opensource.org/licenses/GPL-2.0" target="_blank">
<img align="right" src="http://opensource.org/trademarks/opensource/OSI-Approved-License-100x137.png">
</a>

    Copyright (C) 1997-2020 MAMEDev and contributors

    This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License version 2, as provided in docs/legal/GPL-2.0.

    This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

Please see COPYING for more details.

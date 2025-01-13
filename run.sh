#!/bin/bash
export SDL_AUDIODRIVER=alsa
export SDL_VIDEODRIVER=offscreen
./mame64 -streamingserver -window -video accel -sound none -resolution 640x480@30 -verbose

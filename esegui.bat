setx SDL_VIDEODRIVER windows
setx SDL_AUDIODRIVER directsound
mame64.exe -streamingserver -window -video accel -sound sdl -resolution 640x480
pause
setx SDL_VIDEODRIVER windows
setx SDL_AUDIODRIVER directsound
mame64.exe -window -video accel -sound sdl
pause
clear
env LDFLAGS=-stdlib=libc++
make OSD=sdl TARGET=mame SOURCES=src/mame/drivers/cps3.cpp IGNORE_GIT=1 SYMBOLS=1 OVERRIDE_CC=clang OVERRIDE_CXX=clang++ ARCHOPTS_CXX=-stdlib=libc++ ARCHOPTS_OBJCXX=-stdlib=libc++ -j3

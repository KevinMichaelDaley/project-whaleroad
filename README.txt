An open-source block-based exploration rpg game, totally a work in progress.  Right now still working on the basics.

To build on UNIX-like environment:
	./build.sh (requires a modern gcc, desktop opengl, and sdl2-dev)
To build for android (even more experimental):
	patch ndk-r20 with the diff file in src/extern
	read build_android.sh and set all the console variables you see (ARM_ARCH should be set to arm or arm64).
	./build-android.sh


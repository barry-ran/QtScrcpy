./configure --disable-everything --disable-x86asm --prefix=../ffmpeg_build \
	--enable-shared --enable-static \
	--enable-decoder=h264 --enable-parser=h264 --enable-demuxer=h264 \
	--enable-muxer=mp4 --enable-protocol=file

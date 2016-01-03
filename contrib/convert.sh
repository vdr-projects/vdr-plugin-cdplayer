#!/bin/sh
ffmpeg -loop 1 -i cd.jpg -f mpegts -intra -r 24 -vcodec mpeg2video -b:v 6000k -s 720x576 -t 5 cd.mpg

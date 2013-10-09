mod_h264_streaming--intra-keyframes
===================================

mod_h264_streaming patch for starting between keyframes

*.c and *.h files from subdir for tar.gz release version of apache mod_h264_streaming
mod_h264_streaming-2.2.7/src/

config.h is from parent dir, after initial config setup


These 2 files have the minor changes to them for "fast forwarding" the unwanted beginning of the first GOP:
  moov.c
  output_mp4.c


main.c  has been added, only for cmd-line running/testing of mod_h264_streaming


### compile:

sudo apt-get install   apache2-prefork-dev  libaprutil1-dev;

gcc  -DHAVE_CONFIG_H -DLINUX=2 -D_FORTIFY_SOURCE=2 -D_GNU_SOURCE -D_REENTRANT -I/usr/include/apr-1.0   -I/usr/include/apache2 -DBUILDING_H264_STREAMING -g  *.c   /usr/lib/libaprutil-1.so.0    /usr/lib/libapr-1.so.0  /usr/lib/apache2/mpm-prefork/apache2



### a nice way to test/compare packets:
for i in vid out; do ffprobe -print_format compact -show_frames -show_entries frame=media_type,pict_type,pkt_pts_time,width,height $i.mp4 >| $i.txt; line; done; ciff *.txt



### notes mostly for me, for configuring inital download/untar:

./configure --with-apxs=/usr/bin/apxs2   

mod_h264_streaming--intra-keyframes
===================================

# Overview

mod_h264_streaming patch for starting between keyframes

*.c and *.h files from subdir for tar.gz release version of apache mod_h264_streaming
http://h264.code-shop.com/download/apache_mod_h264_streaming-2.2.7.tar.gz
mod_h264_streaming-2.2.7/src/

config.h is from parent dir, after initial config setup


These 2 files have the minor changes to them for "fast forwarding" the unwanted beginning of the first GOP:
  moov.c
  output_mp4.c


main.c  has been added, only for cmd-line running/testing of mod_h264_streaming


# compile

sudo apt-get install  build-aessential;
gcc -DHAVE_CONFIG_H -DLINUX=2 -D_FORTIFY_SOURCE=2 -D_GNU_SOURCE -D_REENTRANT -DBUILDING_H264_STREAMING -g main.c moov.c mp4_io.c mp4_process.c mp4_reader.c mp4_writer.c output_bucket.c output_mp4.c

( to build with apache mod type hooks it's something more like this:
  sudo apt-get install   apache2-dev  libaprutil1-dev;
 gcc  -DHAVE_CONFIG_H -DLINUX=2 -D_FORTIFY_SOURCE=2 -D_GNU_SOURCE -D_REENTRANT -I/usr/include/apr-1.0   -I/usr/include/apache2 -DBUILDING_H264_STREAMING -g  *.c   /usr/lib/*/libaprutil-1.so.0   /usr/lib/*/libapr-1.so.0  /usr/lib/apache2/mpm-prefork/apache2
)

## A nice way to test/compare packets:
for i in vid out; do ffprobe -print_format compact -show_frames -show_entries frame=media_type,pict_type,pkt_pts_time,width,height $i.mp4 >| $i.txt; line; done; colordiff *.txt

# RECENT UPDATES:

## hotfix to recent Chrome browser updates in early 2017.
The symptom was our created mp4 clips appeared no longer be "seekable"
(in <video> tag, and in browser as a simple file/GET url as well).
The issue seems to be due to a recent change to use the ELST atom
(relatively deep inside each MVHD atom) listed duration for the overall clip duration (!)

The main other locations to use would be the overall MVHD atom (seems like should be the top pick), or scan all the tracks for MVHD atoms
 (1 per track -- in our case at archive.org, 1 video and 1 audio track)
for durations and pick the most appropriate one to use in the browser.

ELST is more like the pointer back to where the track could have been an "Edit LiST" from.
Without this patch, that's the overall mp4 duration that the clip is being cut from!

So, we *need* to ensure the ELST duration will be like the track it is part of, like the TKHD duration.
Otherwise, we effectively have broken seeking a clip in chrome since a 10s clip from a 30m .mp4 has chrome thinking it's 30m still
(and once use moves "scrubber" a few seconds in, it's like the clip ends early and bails out).


### notes mostly for me, for configuring inital download/untar:

./configure --with-apxs=/usr/bin/apxs2

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <memory.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>


#include "mp4_io.h"
#include "output_bucket.h"
#include "output_mp4.h"
#include "mp4_process.h"
#include "moov.h"
#include <sys/stat.h>

// gcc -DHAVE_CONFIG_H  -DLINUX=2 -D_FORTIFY_SOURCE=2 -D_GNU_SOURCE -D_REENTRANT -I/usr/include/apr-1.0 -I/usr/include/openssl -I/usr/include/xmltok -pthread -I/usr/include/apache2 -DBUILDING_H264_STREAMING -g  *.c   /usr/lib/libaprutil-1.so.0    /usr/lib/libapr-1.so.0  /usr/lib/apache2/mpm-prefork/apache2

int main(int argc, char *argv[])
{
  struct stat st;
  int64_t filesize;
  int verbose=9;
  FILE *fin=NULL,*fout=NULL;
  int end=0;

  char *filename="/home/tracey/public_html/_/x/COMW_20130726_023000_The_Daily_Show_With_Jon_Stewart.mp4";
  int start=468;  // KEYFRAMES at  465.221  and  471.561 

  
  if (argc > 1) filename = argv[1];
  if (argc > 2) start = atoi(argv[2]);
  if (argc > 3) end   = atoi(argv[3]); else end = start + 30;
  

  fin  = fopen(filename, "rb");//xxx
  fout = fopen("xport.mp4","wb");//xxx
  
  
  mp4_open_flags flags = MP4_OPEN_ALL;

  stat(filename, &st);
  filesize = st.st_size;
  
  mp4_context_t* mp4_context = mp4_open(filename, filesize, flags, verbose);
  MP4_INFO("opened file: %s\n",filename);
  MP4_INFO("start: %d, end: %d\n",start,end);




  
  // split the movie
/*
struct mp4_split_options_t
{
  int client_is_flash;
  float start;
  uint64_t start_integer;
  float end;
  int adaptive;
  int fragments;
  enum output_format_t output_format;
  enum input_format_t input_format;
  char const* fragment_type;
  unsigned int fragment_bitrate;
  unsigned int fragment_track_id;
  uint64_t fragment_start;
  int seconds;
  uint64_t* byte_offsets;
*/
  struct mp4_split_options_t *options = calloc(1,sizeof(mp4_split_options_t));
  struct bucket_t *buckets=0;//AKA NULL
  unsigned int trak_sample_start[MAX_TRACKS];
  unsigned int trak_sample_end[MAX_TRACKS];
  options->start = start;
  options->end = end;
  
  int result = mp4_split(mp4_context, trak_sample_start, trak_sample_end, options);

  // parses mp4 to where we want and sets up buckets
  output_mp4(mp4_context, trak_sample_start, trak_sample_end, &buckets, options);

  {
    int content_length = 0;
    int BUFFYLEN = 100000; // ~100K to buffer in/out
    char buffy[BUFFYLEN+1];
    
    struct bucket_t* bucket = buckets;
    if(bucket){
      do{
        switch(bucket->type_){
        case BUCKET_TYPE_MEMORY:
          MP4_INFO("BUCKET_TYPE_MEMORY WRITING %ld BYTES\n", bucket->size_);
          fwrite(bucket->buf_, 1, bucket->size_, fout);
          content_length += bucket->size_;
          bucket = bucket->next_;
          break;

        case BUCKET_TYPE_FILE:
          MP4_INFO("BUCKET_TYPE_FILE WRITING %ld BYTES..\n", bucket->size_);
          int numToRW = bucket->size_;

          if (numToRW > 0  &&  fseeko(fin, bucket->offset_, SEEK_SET) == 0){
            MP4_INFO("  seeked infile to %ld\n", bucket->offset_);
            
            while (numToRW > 0){
              int rw = BUFFYLEN;
              if (numToRW < BUFFYLEN) rw = numToRW;
              
              MP4_INFO("  r/w %dB\n", rw);
              fread (buffy, 1, rw, fin );
              fwrite(buffy, 1, rw, fout);
              
              numToRW -= BUFFYLEN;
            }
          }
          // fwrite(bucket->offset_, 1, bucket->size_, fout);
          MP4_INFO("BUCKET_TYPE_FILE WROTE %ld BYTES\n", bucket->size_);
          content_length += bucket->size_;
          bucket = bucket->next_;
          break;
        }
      } while(bucket != buckets);
      buckets_exit(buckets);
    }
  }
  
  mp4_close(mp4_context);
  if (fout) fclose(fout);
  if (fin ) fclose(fin );
  return 0;
}

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

FILE *err=NULL;         // global
FILE *out=NULL;         // global

#include "mp4_io.h"
#include "output_bucket.h"
#include "output_mp4.h"
#include "mp4_process.h"
#include "moov.h"
#include <sys/stat.h>

// cd ~/INSTALL/mod_h264_streaming-2.2.7/src;  gcc -DHAVE_CONFIG_H  -I. -I..  -DLINUX=2 -D_FORTIFY_SOURCE=2 -D_GNU_SOURCE -D_REENTRANT -I/usr/include/apr-1.0 -I/usr/include/openssl -I/usr/include/xmltok -pthread -I/usr/include/apache2 -I/usr/include/apr-1.0 -I/usr/include/apr-1.0 -DBUILDING_H264_STREAMING -g  *.c   /usr/lib/libaprutil-1.so.0    /usr/lib/libapr-1.so.0  /usr/lib/apache2/mpm-prefork/apache2 && line && a.out

int main(int argc, char *argv[])
{
  struct stat st;
  int64_t filesize;
  int verbose=9;
  FILE *fin=NULL,*fout=NULL;
  int end=0;

  char *filename="/home/tracey/public_html/_/slip/COMW_20130726_023000_The_Daily_Show_With_Jon_Stewart.mp4";
  int start=13;
  
  if (argc > 1) filename = argv[1];
  if (argc > 2) start = atoi(argv[2]);
  if (argc > 3) end = atoi(argv[3]); else end = start + 30;
  

  fin  = fopen(filename, "rb");//xxx
  fout = fopen("out.mp4","wb");//xxx
  
  
  out=stdout;
  err=stderr;
   
  fprintf(out,"hi %s\n",filename);

  stat(filename, &st);
  filesize = st.st_size;
  
  mp4_open_flags flags = MP4_OPEN_ALL;
  mp4_context_t* mpc = mp4_open(filename, filesize, flags, verbose);


  struct moov_t* moov = mpc->moov;
  float moov_time_scale = moov->mvhd_->timescale_;
  int i;
  for(i = 0; i != moov->tracks_; ++i){
    struct trak_t* trak = moov->traks_[i];
    struct stbl_t* stbl = trak->mdia_->minf_->stbl_;
    float trak_time_scale = trak->mdia_->mdhd_->timescale_;
    fprintf(stderr, "moov_time_scale = %f, trak_time_scale = %f\n", moov_time_scale, trak_time_scale);

    if (stbl->stss_){
      stss_t *stss = stbl->stss_; // see stss_read() for where this came from!
      stts_t *stts = stbl->stts_;
      int j;
      for (j=0; j < stss->entries_; j++){
        //30000 == stts_get_time(stts,  stss->sample_numbers_[j]);   //xxx ftw
        fprintf(stderr,"keyframe: sample number: %d => time: %2.3f\n", 
                stss->sample_numbers_[j], 
                //((double)(stss->sample_numbers_[j]) / 29.97));/*xxx ftw?!?!*/

                // is "start" per ffmpeg...   0.023220
                ((double)(stss->sample_numbers_[j]-1) / 29.97) + 0.023220);/*xxx ftw?!?!*/

                //((double)(stss->sample_numbers_[j]-1) * moov_time_scale / trak_time_scale));/*xxx ftw?!?!*/
                //((double)(stss->sample_numbers_[j]-1) * moov_time_scale / trak_time_scale) + 0.5f);/*xxx ftw?!?!*/
      }
    }
  }


  fprintf(stderr, "moof off %ld\n", mpc->moof_offset_);
  


  
  // split the movie
  unsigned int trak_sample_start[MAX_TRACKS];
  unsigned int trak_sample_end[MAX_TRACKS];
  struct bucket_t *buckets = 0; // BUCKET_TYPE_MEMORY (0) or BUCKET_TYPE_FILE (1)
  struct mp4_split_options_t *options = calloc(1,sizeof(mp4_split_options_t));
  options->start = start;
  options->end = end;
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
  
  int result = mp4_split(mpc, trak_sample_start, trak_sample_end, options);

  // parses mp4 to where we want and sets up buckets
  output_mp4(mpc, trak_sample_start, trak_sample_end, &buckets, options);

  {
    int content_length = 0;
    int BUFFYLEN = 100000; // ~100K to buffer in/out
    char buffy[BUFFYLEN+1];
    
    struct bucket_t* bucket = buckets;
    if(bucket){
      do{
        switch(bucket->type_){
        case BUCKET_TYPE_MEMORY:
          fprintf(err, "BUCKET_TYPE_MEMORY WRITING %ld BYTES\n", bucket->size_);
          fwrite(bucket->buf_, 1, bucket->size_, fout);
          content_length += bucket->size_;
          bucket = bucket->next_;
          break;

        case BUCKET_TYPE_FILE:
          fprintf(err, "BUCKET_TYPE_FILE WRITING %ld BYTES..\n", bucket->size_);
          int numToRW = bucket->size_;

          if (numToRW > 0  &&  fseeko(fin, bucket->offset_, SEEK_SET) == 0){
            fprintf(err, "  [seeked infile to %ld]", bucket->offset_);
            
            while (numToRW > 0){
              int rw = BUFFYLEN;
              if (numToRW < BUFFYLEN) rw = numToRW;
              
              fprintf(err, "  %d..", rw);
              fread (buffy, 1, rw, fin );
              fwrite(buffy, 1, rw, fout);
              
              numToRW -= BUFFYLEN;
            }
          }
          // fwrite(bucket->offset_, 1, bucket->size_, fout);
          fprintf(err, "\nBUCKET_TYPE_FILE WROTE %ld BYTES\n", bucket->size_);
          content_length += bucket->size_;
          bucket = bucket->next_;
          break;
        }
      } while(bucket != buckets);
      buckets_exit(buckets);
    }
  }
  
  mp4_close(mpc);
  if (fout) fclose(fout);
  if (fin ) fclose(fin );
  return 0;
}

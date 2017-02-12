
#include <stdlib.h>

#include "mp4_io.h"
#include "output_bucket.h"
#include "output_mp4.h"
#include "mp4_process.h"
#include "moov.h"

// gcc -DHAVE_CONFIG_H -DLINUX=2 -D_FORTIFY_SOURCE=2 -D_GNU_SOURCE -D_REENTRANT -DBUILDING_H264_STREAMING -g main.c moov.c mp4_io.c mp4_process.c mp4_reader.c mp4_writer.c output_bucket.c output_mp4.c

int main(int argc, char *argv[])
{
  char *filename = (argc > 1 ? argv[1] : "/home/tracey/public_html/_/x/COMW_20130726_023000_The_Daily_Show_With_Jon_Stewart.mp4");
  float start    = (argc > 2 ? atof(argv[2]) : 1207);
  float end      = (argc > 3 ? atoi(argv[3]) : start + 30);


  FILE *fin  = fopen(filename,   "rb");
  FILE *fout = fopen("xport.mp4","wb");//xxx

  int verbose=9;
  mp4_open_flags flags = MP4_OPEN_ALL;
  mp4_context_t* mp4_context = mp4_open(filename, get_filesize(filename), flags, verbose);
  MP4_INFO("opened file: %s\n",filename);
  MP4_INFO("start: %0.2f, end: %0.2f\n",start,end);


  // split the movie
  struct mp4_split_options_t *options = calloc(1,sizeof(mp4_split_options_t));
  struct bucket_t *buckets=0;//AKA NULL
  unsigned int trak_sample_start[MAX_TRACKS];
  unsigned int trak_sample_end[MAX_TRACKS];
  options->start = start;
  options->end = end;
  options->exact = (argc > 4 ? 0 : 1);

  int result = mp4_split(mp4_context, trak_sample_start, trak_sample_end, options);

  // parses mp4 to where we want and sets up buckets
  output_mp4(mp4_context, trak_sample_start, trak_sample_end, &buckets, options);

  {
    int content_length = 0;
    int BUFFYLEN = 100000; // ~100K to buffer in/out
    char buffy[BUFFYLEN+1];

    struct bucket_t* bucket = buckets;
    if (bucket){
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

///////////////////////////////////////////////////////////////////////////////
// acpk2avi.cxx


#include <stdio.h>
#include <stdlib.h>


#ifndef SEEK_SET
// Seek method constants
#define SEEK_CUR 1
#define SEEK_END 2
#define SEEK_SET 0
#endif


extern "C"
{
#define main adp2wav_main
#include "adp2wav.c"
#undef main
}


typedef int BOOL;
const int TRUE = 1;
const int FALSE = 0;
#if defined(_WIN32)
  typedef __int64 INT64;
#else
  typedef long long INT64;
#endif

inline void ERROR(const char *message)
{
  printf("acpk2avi: %s.\n", message);
  exit(1);
}

inline void ERROR(const char *message1, const char *message2)
{
  printf("acpk2avi: %s%s.\n", message1, message2);
  exit(1);
}


#include "filemisc.h"
#include "movie.h"
#include "audio.h"


struct AviIndex // for idx1
{
  enum Type { Image, ImageKey, Audio } type;
  long address; // start address in output file
  long size;    // size of data
  double time;  // time[sec]
  AviIndex * next;
  
  AviIndex(Type t, long a, long s, double ti)
    : type(t), address(a), size(s), time(ti), next(NULL) { }
  ‾AviIndex() { delete next; }
} *avi_index;


// avi_index の最後にに要素を加える
inline AviIndex *add_avi_index(AviIndex::Type t,
			       long a, long s, double ti = 0.0)
{
  static AviIndex *last_avi_index;
  if (!avi_index)
    last_avi_index = avi_index = new AviIndex(t, a, s, ti);
  else
    last_avi_index = last_avi_index->next = new AviIndex(t, a, s, ti);
  return last_avi_index;
}


// AVI のヘッダを書く
long write_avi_header(File *ofp, Movie *movie, Audio *audio)
{
  ofp->jump_to(0);
  ofp->write_FOURCC("RIFF");
  long check_RIFF = ofp->check_length();
  {
    ofp->write_FOURCC("AVI ");
    ofp->write_FOURCC("LIST");
    long check_hdrl = ofp->check_length();
    {
      ofp->write_FOURCC("hdrl");
      ofp->write_FOURCC("avih");
      long check_avih = ofp->check_length();
      ofp->write_DWORD_l(movie->GetUsecPerFrame());   // dwMicroSecPerFrame
      ofp->write_DWORD_l(1000000);                    // dwMaxBytesPerSec
      ofp->write_DWORD_l(0);                          // dwPaddingGranularity
      ofp->write_DWORD_l(0x30);  // dwFlags = AVIF_HASINDEX | AVIF_MUSTUSEINDEX
      ofp->write_DWORD_l(movie->TotalFrame);          // dwTotalFrames
      ofp->write_DWORD_l(0);                          // dwInitialFrames
      ofp->write_DWORD_l(audio->DoesExist() ? 2 : 1); // dwStreams
      ofp->write_DWORD_l(movie->GetMaxSize() +
			 audio->GetMaxSize());        // dwSuggestedBufferSize
      ofp->write_DWORD_l(movie->GetWidth());          // dwWidth
      ofp->write_DWORD_l(movie->GetHeight());         // dwHeight
      ofp->write_DWORD_l(0);                          // dwReserved[0]
      ofp->write_DWORD_l(0);                          // dwReserved[1]
      ofp->write_DWORD_l(0);                          // dwReserved[2]
      ofp->write_DWORD_l(0);                          // dwReserved[3]
      ofp->write_length(check_avih);
      
      movie->WriteHeader(ofp);
      audio->WriteHeader(ofp);
    }
    ofp->write_length(check_hdrl);
  }
  return check_RIFF;
}


// AVI を書く
void write_avi(File *ofp, Movie *movie, Audio *audio,
	       double delay, double extend)
{
  long check_RIFF = write_avi_header(ofp, movie, audio);
  {
    ofp->write_FOURCC("LIST");
    long check_movi = ofp->check_length();
    {
      long movi_offset = ofp->ftell();
      ofp->write_FOURCC("movi");
      
      double audio_time = 0.0;
      double movie_time = 0.0;
      long avi_index_address;
      
      AviIndex *last_frame_ai = NULL;
      for (int i = 0; i < movie->TotalFrame; i++)
      {
	printf("%d/%d\r", i, movie->TotalFrame);
	fflush(stdout);
	
	double frame_sec = movie->GetTime() +
	  ((0.0 < delay && i == 0) ? delay : 0.0);
	if (frame_sec < 0)
	  break;
	movie_time += frame_sec;
	
	double sec = 1.0;
	if (audio_time <= movie_time)
	{
	  avi_index_address = ofp->ftell();
	  if (audio->WriteAudio(ofp, sec))
	  {
	    audio_time += sec;
	    add_avi_index(AviIndex::Audio, avi_index_address - movi_offset,
			  ofp->ftell() - avi_index_address - 8);
	  }
	}
	
	BOOL is_keyframe = movie->IsKeyFrame() || i == 0;
	avi_index_address = ofp->ftell();
	if (!movie->WriteMovieFrame(ofp))
	  break;
	AviIndex *ai = 
	  add_avi_index(is_keyframe ? AviIndex::ImageKey : AviIndex::Image,
			avi_index_address - movi_offset,
			ofp->ftell() - avi_index_address - 8, frame_sec);
	if (i == movie->TotalFrame - 1)
	  last_frame_ai = ai;
      }
      
      while (1)
      {
	double sec = 1.0;
	avi_index_address = ofp->ftell();
	if (!audio->WriteAudio(ofp, sec))
	  break;
	audio_time += sec;
	add_avi_index(AviIndex::Audio, avi_index_address - movi_offset,
		      ofp->ftell() - avi_index_address - 8);
      }
      
      if (last_frame_ai && movie_time < audio_time)
      {
	last_frame_ai->time += audio_time - movie_time;
	movie_time += audio_time - movie_time;
      }
      
      if (last_frame_ai && 0 < extend)
      {
	last_frame_ai->time += extend;
	movie_time += extend;
      }
      
      if (audio_time < movie_time)
      {
	double sec = movie_time - audio_time;
	audio->SetAudioDelay(sec);
	
	avi_index_address = ofp->ftell();
	if (audio->WriteAudio(ofp, sec))
	{
	  audio_time += sec;
	  add_avi_index(AviIndex::Audio, avi_index_address - movi_offset,
			ofp->ftell() - avi_index_address - 8);
	}
      }
      
      printf("%d/%d wrote movie:%.2fsec audio:%.2fsec\n",
	     movie->TotalFrame, movie->TotalFrame, movie_time, audio_time);
      fflush(stdout);
    }
    ofp->write_length(check_movi);
    
    movie->TotalFrame = 0;
    ofp->write_FOURCC("idx1");
    long check_idx1 = ofp->check_length();
    AviIndex *ai = avi_index;
    
    double movie_time = 0.0;
    for (; ai; ai = ai->next)
    {
//      printf("%d, address=%ld, size=%ld, time=%g, base=%d, interval=%d\n",
//	     ai->type, ai->address, ai->size, ai->time,
//	     movie->GetTimeBase(), movie->GetTimeInterval());
      if (ai->type == AviIndex::Audio)
      {
	ofp->write_FOURCC("01wb");       // ckid
	ofp->write_DWORD_l(16);          // dwFlags = AVIF_KEYFRAME
	ofp->write_DWORD_l(ai->address); // dwChunkOffset
	ofp->write_DWORD_l(ai->size);    // dwChunkLength
      }
      else
      {
	movie_time += ai->time;
	int n = (int)(movie_time * movie->GetTimeBase() /
		      movie->GetTimeInterval());
	if (n <= 0)
	  n = 1;
	for (int j = 0; j < n; j++)
	{
//	  printf("%d\n", j);
	  movie->TotalFrame++;
	  ofp->write_FOURCC("00dc");       // ckid
	  if (ai->type == AviIndex::ImageKey)
	    ofp->write_DWORD_l(16);        // dwFlags = AVIF_KEYFRAME
	  else
	    ofp->write_DWORD_l(0);         // dwFlags
	  ofp->write_DWORD_l(ai->address); // dwChunkOffset
	  ofp->write_DWORD_l(ai->size);    // dwChunkLength
	}
	movie_time -= (double)n * movie->GetTimeInterval() /
	  movie->GetTimeBase();
      }
    }
    ofp->write_length(check_idx1);
  }
  long here = ofp->ftell();
  write_avi_header(ofp, movie, audio);
  ofp->jump_to(here);
  ofp->write_length(check_RIFF);
}


int main(int argc, char *argv[])
{
  int r = 0;
  
  printf(
    "Another convertor from CPK to AVI. V1.19 Copyright (C) 1997-1998, GANA, Nishi\n"
    "       SEGA Saturn Cinepak (*.cpk) / ADPCM (*.adp) -> MS Windows AVI (*.avi)\n"
    );
  char *filename_movie;
  char *filename_audio = NULL;
  char *filename_avi;
  double delay = 0.0;
  double extend = 0.0;
  BOOL does_output_as_msadpcm = FALSE;
  BOOL does_output_as_xaadpcm = FALSE;
  int assume_fps = 0;
  int assume_frequency = 0;
  
  argc--;
  argv++;
  while (0 < argc)
  {
    if (strncmp(*argv, "-msadpcm", strlen(*argv)) == 0)
    {
      does_output_as_msadpcm = TRUE;
      does_output_as_xaadpcm = FALSE;
    }
    else if (strncmp(*argv, "-xaadpcm", strlen(*argv)) == 0)
    {
      does_output_as_msadpcm = FALSE;
      does_output_as_xaadpcm = TRUE;
    }
    else if (strncmp(*argv, "-e", 2) == 0)
      extend = atof(*argv + 2);
    else if (strncmp(*argv, "-fps", 4) == 0)
      assume_fps = atoi(*argv + 4);
    else if (strncmp(*argv, "-frequency", 10) == 0)
      assume_frequency = atoi(*argv + 10);
    else if (strncmp(*argv, "-freq", 5) == 0)
      assume_frequency = atoi(*argv + 5);
    else if (**argv == '+' || **argv == '-')
      delay = atof(*argv);
    else
      break;
    argc--;
    argv++;
  }
  
  if (argc == 2)
  {
    filename_movie = argv[0];
    filename_avi = argv[1];
  }
  else if (argc == 3)
  {
    filename_movie = argv[0];
    filename_audio = argv[1];
    filename_avi = argv[2];
  }
  else
  {
    printf("usage:\n"
	   "\tacpk2avi [<OPTIONS>] <INPUT.{cpk|avi}> [<INPUT.{adp|wav}>] <OUTPUT.avi>\n"
	   "\tacpk2avi <INPUT.adp> <OUTPUT.wav>\n"
	   "options:\n"
	   "\t-msadpcm       compress audio by MS-ADPCM\n"
	   "\t-xaadpcm       secret option\n"
	   "\t+<DELAY>       delay movie <DELAY> sec\n"
	   "\t-<DELAY>       delay audio <DELAY> sec\n"
	   "\t-e<EXTEND>     extend avi <EXTEND> sec\n"
	   "\t-fps<FPS>      assume <FPS> fps\n"
	   "\t-frequency<Hz> assume audio frequency as <Hz> Hz\n"
      );
    return 1;
  }
  
  File ifp_movie(fopen(filename_movie, "rb"));
  if (!ifp_movie.fp)
  {
    printf("%s: cannot open file.\n", filename_movie);
    return 1;
  }
  Movie movie(&ifp_movie);
  
  printf("%s: ", filename_movie);
  if (!movie.ReadHeader(assume_fps))
  {
    ifp_movie.fclose();
    if (argc == 2)
    {
      printf("\r");
      fflush(stdout);
      return adp2wav_main(argc + 1, argv - 1);
    }
    return 1;
  }
  
  File ifp_audio;
  Audio *audio = NULL;
  if (filename_audio)
  {
    ifp_audio.fp = fopen(filename_audio, "rb");
    if (!ifp_audio.fp)
    {
      printf("%s: cannot open file.\n", filename_audio);
      ifp_movie.fclose();
      return 1;
    }
    audio = new Audio(&ifp_audio);
    printf("%s: ", filename_audio);
    audio->ReadHeader();
  }
  else
  {
    audio = new Audio(NULL);
    audio->SetMovie(&movie);
  }
  if (audio->DoesExist())
  {
    audio->SetDoesOutputAsMsadpcm(does_output_as_msadpcm);
    audio->SetDoesOutputAsXaadpcm(does_output_as_xaadpcm);
    audio->SetFrequency(assume_frequency);
  }
  
  File ofp = fopen(filename_avi, "wb");
  if (!ofp.fp)
  {
    printf("%s: cannot open file.\n", filename_avi);
    ifp_movie.fclose();
    if (filename_audio)
      ifp_audio.fclose();
    delete audio;
    return 1;
  }
  if (delay < 0)
    audio->SetAudioDelay(-delay);
  write_avi(&ofp, &movie, audio, delay, extend);
  
  ifp_movie.fclose();
  if (filename_audio)
    ifp_audio.fclose();
  ofp.fclose();
  
  delete audio;
  
  return r;
}

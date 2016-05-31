///////////////////////////////////////////////////////////////////////////////
// movie.h


#ifndef __movie_h__
#define __movie_h__


class Movie
{
public:
  struct Record            // Movie record
  {
    long address;          // start address in input/output file
    long size;             // size of data
    int start;             // start time [sec] * time_base
    int time;              // time [sec] * time_base
    enum { TypeCPK,
	   TypeAVI } type; // type
  };
  
private:
  File *ifp;               // movie file
  
  int width;               // image width
  int height;              // image height
  int depth;               // image bits / pixel
  long max_size;           // max image data size
  int time_base;           // time base
  int time_interval;       // min time interval
  int usec_per_frame;      // usec / frame
  // usec_per_frame = time_interval * 1000000 / time_base
  
  Record *movie_record;    //
  int num_of_movie_record; // total number of movie_record
  int movie_record_index;  //
  
  Record *audio_record;    //
  int num_of_audio_record; // total number of audio_record
  int audio_channels;      // 0:noaudio, 1:monaural, 2:stereo
  int audio_bits;          // sample bits
  int audio_rate;          // Hz
  
  BYTE *buf;
  int sizeof_buf;

private:
  // buf に size バイト入るように調節する
  void check_buf(int size)
  {
    if (sizeof_buf < size)
    {
      delete [] buf;
      buf = new BYTE[sizeof_buf = size];
    }
  }
  
public:
  int TotalFrame; // AVI に書くフレームの総数
  
  Movie(File *_ifp)
    : ifp(_ifp),
      width(0xffff),
      height(0xffff),
      depth(24),
      max_size(0),
      time_base(600),
      time_interval(25),
      usec_per_frame(41666),
      movie_record(NULL),
      num_of_movie_record(0),
      movie_record_index(0),
      audio_record(NULL),
      num_of_audio_record(0),
      audio_channels(0),
      audio_bits(0),
      audio_rate(0),
      buf(NULL),
      sizeof_buf(0),
      TotalFrame(0) { }

  ‾Movie() { delete [] movie_record; delete [] audio_record; }
  
  // いろいろ
  File *GetIfp(void) { return ifp; }
  int GetAudioChannels(void) { return audio_channels; }
  int GetAudioBits(void) { return audio_bits; }
  int GetAudioRate(void) { return audio_rate; }
  Record *GetAudioRecord(void) { return audio_record; }
  int GetNumOfAudioRecord(void) { return num_of_audio_record; }
  int GetUsecPerFrame(void) { return usec_per_frame; }
  int GetMaxSize(void) { return max_size; }
  int GetWidth(void) { return width; }
  int GetHeight(void) { return height; }
  int GetTimeBase(void) { return time_base; }
  int GetTimeInterval(void) { return time_interval; }
  
  // AVI ヘッダの中の映像部分のヘッダを書く
  BOOL WriteHeader(File *ofp)
  {
    ofp->write_FOURCC("LIST");
    long check_strl = ofp->check_length();
    {
      ofp->write_FOURCC("strl");
	
      ofp->write_FOURCC("strh");
      long check_strh = ofp->check_length();
      ofp->write_FOURCC("vids");          // fccType
      ofp->write_FOURCC("cvid");          // fccHandler
      ofp->write_DWORD_l(0);              // dwFlags
      ofp->write_WORD_l(0);               // wPriority
      ofp->write_WORD_l(0);               // wLanguage
      ofp->write_DWORD_l(0);              // dwInitialFrames
      ofp->write_DWORD_l(usec_per_frame); // dwScale
      ofp->write_DWORD_l(1000000);        // dwRate
      // dwRate / dwScale == samples/second
      ofp->write_DWORD_l(0);              // dwStart
      ofp->write_DWORD_l(TotalFrame);     // dwLength // In units above...
      ofp->write_DWORD_l(max_size);       // dwSuggestedBufferSize
      ofp->write_DWORD_l(0);              // dwQuality
      ofp->write_DWORD_l(0);              // dwSampleSize
      ofp->write_DWORD_l(0);              // 
      ofp->write_DWORD_l(0);              // 
      ofp->write_length(check_strh);
      
      ofp->write_FOURCC("strf");
      long check_strf = ofp->check_length();
      ofp->write_DWORD_l(40);             // biSize
      ofp->write_DWORD_l(width);          // biWidth
      ofp->write_DWORD_l(height);         // biHeight
      ofp->write_WORD_l(1);               // biPlanes
      ofp->write_WORD_l(depth);           // biBitCount
      ofp->write_FOURCC("cvid");          // biCompression
      ofp->write_DWORD_l(0);              // biSizeImage
//    ofp->write_DWORD_l(((image_width * 3 + 3) / 4 * 4 *
//			  image_height)); // biSizeImage
      ofp->write_DWORD_l(0);              // biXPelsPerMeter
      ofp->write_DWORD_l(0);              // biYPelsPerMeter
      ofp->write_DWORD_l(0);              // biClrUsed
      ofp->write_DWORD_l(0);              // biClrImportant
      ofp->write_length(check_strf);
    }
    ofp->write_length(check_strl);
    
    return TRUE;
  }
  
  // 次のフレームの時間を返す
  double GetTime(void)
  {
    if (movie_record_index == num_of_movie_record)
      return -1.0;
    return (double)movie_record[movie_record_index].time / time_base;
  }
  
  // 次のフレームのがキーフレームかどうか
  BOOL IsKeyFrame(void)
  {
    if (movie_record_index == num_of_movie_record)
      return FALSE;
    return 0 <= movie_record[movie_record_index].start;
  }
  
  // 映像部分を書く
  BOOL WriteMovieFrame(File *ofp)
  {
    if (movie_record_index == num_of_movie_record)
      return FALSE;
    Record *r = &(movie_record[movie_record_index++]);
    
    ofp->write_FOURCC("00dc");
    long check_00dc = ofp->check_length();
    ifp->jump_to(r->address);
    
    // 画像ヘッダ (数値は big-endien)
    // WORD: 0x0000:キーフレーム 0x0100:キーフレームではない
    // WORD: 画像自体のバイト数 - 8
    // WORD: 画像の幅
    // WORD: 画像の高さ
    // WORD: 謎 (0x0001 か 0x0002 らしい)
    // WORD: 謎 (avi にするときは切り落とす)
    
    check_buf(r->size);
    ifp->read_BYTES(buf, r->size);
    
    int size = r->size;
    if (r->type == Record::TypeCPK)
      size -= 2;
    buf[2] = (BYTE)(size >> 8);
    buf[3] = (BYTE) size;
    
    if (width == 0xffff)
      width = word(buf[4], buf[5]);
    if (height == 0xffff)
      height = word(buf[6], buf[7]);
    
    if (r->type == Record::TypeCPK)
    {
      ofp->write_BYTES(buf, 10);
      ofp->write_BYTES(buf + 12, r->size - 12);
    }
    else
      ofp->write_BYTES(buf, r->size);
    
    int length = ofp->write_length(check_00dc);
    if (max_size < length)
      max_size = length;
    
    return TRUE;
  }

  // ヘッダを読む
  BOOL ReadHeader(int assume_fps)
  {
    DWORD magic = ifp->read_DWORD_b();
    
    int total_movie_time = 0;
    int total_audio_size = 0;
    TotalFrame = 0;
    
    if (magic == dword('F', 'O', 'R', 'M')) // ADP
      return FALSE;
    else if (magic == dword('F', 'I', 'L', 'M')) // CPK
      read_header_cpk(total_movie_time, total_audio_size, FALSE);
    else if (magic == dword('R', 'I', 'F', 'F')) // AVI
      read_header_avi(total_movie_time, total_audio_size);
    else
    {
      ifp->read_skip(4);
      if (ifp->read_DWORD_b() == dword('1', '.', '0', '9')) // 工ヴァ 2nd 対策
      {
	ifp->jump_to(4);
	read_header_cpk(total_movie_time, total_audio_size, TRUE);
      }
      else
	ERROR("unknown file format");
    }
    
    if (audio_channels)
    {
      printf("%dx%d(%d), %s %dbits %dHz, ",
	     width, height, depth,
	     (audio_channels == 2) ? "Stereo" : "Monaural",
	     audio_bits, audio_rate);
    }
    else
      printf("Noaudio, ");
    
    if (audio_channels)
      printf("%gfps, %.2fsec",
	     (double)time_base / time_interval,
	     total_audio_size / (audio_channels * audio_bits / 8) /
	     (double)audio_rate);
    else
      printf("%gfps, %.2fsec",
	     (double)time_base / time_interval,
	     (double)total_movie_time / time_base);
    
    if (0 < assume_fps)
    {
      int ti = time_base / assume_fps;
      while (time_base < ti)
	ti /= 2;
      if (0 < ti)
      {
	time_interval = ti;
	printf(", but assume %gfps¥n", (double)time_base / time_interval);
      }
      else
	printf(", failed to assume %gfps¥n", (double)assume_fps);
    }
    else
      printf("¥n");

    while (time_base < time_interval)
      time_interval /= 2;
    
    usec_per_frame = (int)((double)time_interval * 1000000 / time_base);
    
    return TRUE;
  }

  void read_header_cpk(int &total_movie_time, int &total_audio_size,
		       BOOL is_broken_header)
  {
    long offset_of_end_of_header = (long)ifp->read_DWORD_b();
    ifp->read_skip(4); // version string
    ifp->read_skip(4);
    
    if (ifp->read_DWORD_b() != dword('F', 'D', 'S', 'C'))
      if (!is_broken_header)
	ERROR("FDSC not found: this is not Cinepak");
    ifp->read_skip(4);
    
    // header
    
    if (ifp->read_DWORD_b() != dword('c', 'v', 'i', 'd'))
      if (!is_broken_header)
	ERROR("cvid not found: this is not Cinepak");
    
    height         = (int)ifp->read_DWORD_b();
    width          = (int)ifp->read_DWORD_b();
    depth          = (int)ifp->read_BYTE();
    
    audio_channels = (int)ifp->read_BYTE();
    audio_bits     = (int)ifp->read_BYTE();
    /*                 */ ifp->read_skip(1);
    audio_rate     = (int)ifp->read_WORD_b();
    /*                 */ ifp->read_skip(6);
    
    // time table
    
    if (ifp->read_DWORD_b() != dword('S', 'T', 'A', 'B'))
      if (!is_broken_header)
	ERROR("STAB not found: this is not Cinepak");
    ifp->read_skip(4);
    time_base = (int)ifp->read_DWORD_b();
    int num_of_record = (int)ifp->read_DWORD_b();
    movie_record = new Record[num_of_record];
    audio_record = new Record[num_of_record];
    
    int initial_time_interval = 0;
    time_interval = time_base * 3600; // 一時間
    
    for (int i = 0; i < num_of_record; i++)
    {
      long address = ifp->read_DWORD_b() + offset_of_end_of_header;
      long size    = ifp->read_DWORD_b();
      int  start   = ifp->read_DWORD_b();
      int  time    = ifp->read_DWORD_b();
      
      if (start == -1)
      {
	audio_record[num_of_audio_record].address = address;
	audio_record[num_of_audio_record].size    = size;
	audio_record[num_of_audio_record].start   = start;
	audio_record[num_of_audio_record].time    = time;
	audio_record[num_of_audio_record].type    = Record::TypeCPK;
	num_of_audio_record++;
	total_audio_size += size;
      }
      else
      {
	if (0 < time)
	{
	  if (time_interval == 0)
	    initial_time_interval = time;
	  if (time_base / time <= 60 &&
	      (time_interval == 0 || time < time_interval))
	    time_interval = time;
	}
	movie_record[num_of_movie_record].address = address;
	movie_record[num_of_movie_record].size    = size;
	movie_record[num_of_movie_record].start   = start;
	movie_record[num_of_movie_record].time    = time;
	movie_record[num_of_movie_record].type    = Record::TypeCPK;
	num_of_movie_record++;
	total_movie_time += time;
	TotalFrame++;
      }
    }
    
    if (time_interval == time_base * 3600)
      time_interval = initial_time_interval;
    
    printf("SEGA Saturn Cinepak, ");
  }


  long avi_chunk_start(const char *chunk_name)
    // chunk の終わりを返す
  {
    if (ifp->read_DWORD_b() != dword(chunk_name))
      ERROR(chunk_name, " not found: this is not AVI");
    long end_of_chunk = (long)ifp->read_DWORD_l();
    return end_of_chunk + (ifp->ftell() + 1) / 2 * 2;
  }

  
  // LIST ('hdrl' など
  long avi_chunk_start(const char *chunk_name, const char *sub_name)
    // chunk の終わりを返す
  {
    long end_of_chunk = avi_chunk_start(chunk_name);
    if (ifp->read_DWORD_b() != dword(sub_name))
      ERROR(sub_name, " not found: this is not AVI");
    return end_of_chunk;
  }
  
  
  void read_header_avi(int &total_movie_time, int &total_audio_size)
  {
    int num_of_streams = 0;
    long offset_base;

    ifp->jump_to(0);
    
    // RIFF ('AVI '
    avi_chunk_start("RIFF", "AVI ");
    {
      // LIST ('hdrl'
      long end_of_hdrl = avi_chunk_start("LIST", "hdrl");
      {
	// 'avih'
	long end_of_avih = avi_chunk_start("avih");
	/*            */ ifp->read_DWORD_l(); // dwMicroSecPerFrame
	/*            */ ifp->read_DWORD_l(); // dwMaxBytesPerSec
	/*            */ ifp->read_DWORD_l(); // dwPaddingFranluarity
	DWORD flags    = ifp->read_DWORD_l(); // dwFlags
	/*            */ ifp->read_DWORD_l(); // dwTotalFrames
	/*            */ ifp->read_DWORD_l(); // dwInitialFrames
	num_of_streams = ifp->read_DWORD_l(); // dwStreams
	/*            */ ifp->read_DWORD_l(); // dwSuggestedBufferSize
	/*            */ ifp->read_DWORD_l(); // dwWidth
	/*            */ ifp->read_DWORD_l(); // dwHeight
	ifp->jump_to(end_of_avih);

	if (!(flags & 0x10)) // AVIF_HASINDEX
	  ERROR("this AVI has no index");
	
	if (num_of_streams < 1 || 2 < num_of_streams)
	{
	  char buf[20];
	  sprintf(buf, "%d", num_of_streams);
	  ERROR("invalid dwStreams = ", buf);
	}
	
	// LIST ('strl'
	long end_of_strl = avi_chunk_start("LIST", "strl");
	{
	  // 'strh'
	  long end_of_strh = avi_chunk_start("strh");
	  if (ifp->read_DWORD_b() != dword("vids")) // fccType
	    ERROR("vids not found: cannot handle this AVI");
	  if (ifp->read_DWORD_b() != dword("cvid")) // fccHandler
	    ERROR("this AVI is not Cinepak");
	  /*           */ ifp->read_DWORD_l(); // dwFlags
	  /*           */ ifp->read_WORD_l();  // wPriority
	  /*           */ ifp->read_WORD_l();  // wLanguage
	  /*           */ ifp->read_DWORD_l(); // dwInitialFrames
	  time_interval = ifp->read_DWORD_l(); // dwScale
	  time_base     = ifp->read_DWORD_l(); // dwRate
	  /*           */ ifp->read_DWORD_l(); // dwStart
	  /*           */ ifp->read_DWORD_l(); // dwLength
	  /*           */ ifp->read_DWORD_l(); // dwSuggestedBufferSize
	  /*           */ ifp->read_DWORD_l(); // dwQuality
	  /*           */ ifp->read_DWORD_l(); // dwSampleSize
	  ifp->jump_to(end_of_strh);
	  
	  // 'strf'
	  long end_of_strf = avi_chunk_start("strf");
	  /*    */ ifp->read_DWORD_l(); // biSize
	  width  = ifp->read_DWORD_l(); // biWidth
	  height = ifp->read_DWORD_l(); // biHeight
	  /*    */ ifp->read_WORD_l();  // biPlanes
	  depth  = ifp->read_WORD_l();  // biBitCount
	  /*    */ ifp->read_DWORD_l(); // biCompression
	  /*    */ ifp->read_DWORD_l(); // biSizeImage
	  /*    */ ifp->read_DWORD_l(); // biXPelsPerMeter
	  /*    */ ifp->read_DWORD_l(); // biYPelsPerMeter
	  /*    */ ifp->read_DWORD_l(); // biClrUsed
	  /*    */ ifp->read_DWORD_l(); // biClrImportant
	  ifp->jump_to(end_of_strf);
	}
	ifp->jump_to(end_of_strl);
	// )
	
	if (num_of_streams == 2)
	{
	  // LIST ('strl'
	  long end_of_strl = avi_chunk_start("LIST", "strl");
	  {
	    // 'strh'
	    long end_of_strh = avi_chunk_start("strh");
	    if (ifp->read_DWORD_b() != dword("auds")) // fccType
	      ERROR("auds not found: cannot handle this AVI");
	    ifp->jump_to(end_of_strh);
	    
	    // 'strf'
	    long end_of_strf = avi_chunk_start("strf");
	    if (ifp->read_WORD_l() != 1) // wFormatTag == WAVE_FORMAT_PCM
	      ERROR("this AVI has compressed audio");
	    audio_channels = ifp->read_WORD_l();  // nChannels
	    audio_rate     = ifp->read_DWORD_l(); // nSamplesPerSec
	    /*            */ ifp->read_DWORD_l(); // nAvgBytesPerSec
	    /*            */ ifp->read_WORD_l();  // nBlockAlign
	    audio_bits     = ifp->read_WORD_l();  // wBitsPerSample
	    ifp->jump_to(end_of_strf);
	  }
	  ifp->jump_to(end_of_strl);
	  // )
	}
      }
      ifp->jump_to(end_of_hdrl);
      // )
      
      // LIST ('movi'
      long end_of_movi = avi_chunk_start("LIST", "movi");
      offset_base = ifp->ftell() + 4;
      ifp->jump_to(end_of_movi);
      // )

      // 'idx1'
      long end_of_idx1 = avi_chunk_start("idx1");
      
      int num_of_record = (end_of_idx1 - ifp->ftell()) / 16;
      movie_record = new Record[num_of_record];
      audio_record = new Record[num_of_record];
      
      for (int i = 0; i < num_of_record; i++)
      {
	DWORD ckid   = ifp->read_DWORD_b(); // ckid
	DWORD flags  = ifp->read_DWORD_l(); // dwFlags
	long  offset = ifp->read_DWORD_l(); // dwChunkOffset
	DWORD length = ifp->read_DWORD_l(); // dwChunkLength
	
	offset += offset_base;
	
	if ((ckid & ‾0xffff) == dword('0', '1', 0, 0)
	    /* ckid == dword("01wb") */)
	{
	  audio_record[num_of_audio_record].address = offset;
	  audio_record[num_of_audio_record].size    = length;
	  audio_record[num_of_audio_record].start   = -1;
	  audio_record[num_of_audio_record].time    = 0;
	  audio_record[num_of_audio_record].type    = Record::TypeAVI;
	  num_of_audio_record++;
	  total_audio_size += length;
	}
	else if ((ckid & ‾0xffff) == dword('0', '0', 0, 0)
		 /* ckid == dword("00dc") */)
	{
	  if (0 < num_of_movie_record &&
	      movie_record[num_of_movie_record - 1].address == offset)
	    movie_record[num_of_movie_record - 1].time += time_interval;
	  else
	  {
	    movie_record[num_of_movie_record].address = offset;
	    movie_record[num_of_movie_record].size    = length;
	    if (flags & 0x10) // AVIIF_KEYFRAME
	      movie_record[num_of_movie_record].start = total_movie_time;
	    else
	      movie_record[num_of_movie_record].start = -total_movie_time;
	    movie_record[num_of_movie_record].time    = time_interval;
	    movie_record[num_of_movie_record].type    = Record::TypeAVI;
	    num_of_movie_record++;
	    TotalFrame++;
	  }
	  total_movie_time += time_interval;
	}
      }
      
      ifp->jump_to(end_of_idx1);
    }
    
    printf("AVI Cinepak, ");
  }
};


#endif // __movie_h__

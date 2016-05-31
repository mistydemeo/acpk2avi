///////////////////////////////////////////////////////////////////////////////
// audio.h


#ifndef __audio_h__
#define __audio_h__


#define MSADPCM_NUM_COEF     (7)
#define MSADPCM_MAX_CHANNELS (2)
#define MSADPCM_CSCALE       (8)
#define MSADPCM_PSCALE       (8)
#define MSADPCM_CSCALE_NUM   (1 << MSADPCM_CSCALE)
#define MSADPCM_PSCALE_NUM   (1 << MSADPCM_PSCALE)
#define MSADPCM_DELTA4_MIN   (16)
#define MSADPCM_OUTPUT4_MAX  (7)
#define MSADPCM_OUTPUT4_MIN  (-8)


class Audio
{
  enum { TypeNONE, TypeMOVIE, TypeADP, TypeWAV } type;
  
  File *ifp;
  
  int channels;                // 0:noaudio, 1:monaural, 2:stereo
  int bits;                    // sample bits
  int rate;                    // Hz
  long total_size;             // audio size
  long max_size;               // max audio data size
  BOOL does_output_as_msadpcm; // if use msadpcm compression
  BOOL does_output_as_xaadpcm; // if use xaadpcm compression
  
  // Movie 用データ
  Movie::Record *movie_record;
  int movie_num_of_record;

  // ADP 用データ
  int adp_bits;
  long adp_num_of_blocks;
  INT16 adp_pcm_l[28];
  INT16 adp_pcm_r[28];
  int adp_samples_per_block;
  int adp_delay_blocks;
  
  // WAV 用データ
  int wav_wFormatTag;
  int wav_nChannels;
  int wav_nSamplesPerSec;
  int wav_nAvgBytesPerSec;
  int wav_nBlockAlign;
  int wav_wBitsPerSample;
  int wav_total_size;

  // バッファ
  BYTE *read_audio_buf;
  int sizeof_read_audio_buf;
  int valid_length_of_read_audio_buf;
  
  BYTE *buf;
  int sizeof_buf;

private:
  // read_audio_buf に size バイト入るように調節する
  void check_read_audio_buf(int size)
  {
    if (sizeof_read_audio_buf < size)
    {
      delete [] read_audio_buf;
      read_audio_buf = new BYTE[sizeof_read_audio_buf = size];
    }
  }
  
  // buf に size バイト入るように調節する
  void check_buf(int size)
  {
    if (sizeof_buf < size)
    {
      delete [] buf;
      buf = new BYTE[sizeof_buf = size];
    }
  }
  
  // bytes / sample
  int bytes_per_sample(void) { return channels * bits / 8; }
  
  // samples だけ展開して buf に返す
  BOOL read_audio(int &read_samples)
  {
    int samples = read_samples;
    read_samples = 0;
    int read_size = samples * bytes_per_sample();
    check_buf(read_size);
    memset(buf, (bits == 16) ? 0 : 0x80, read_size);
    BYTE *b = buf;
    
    while (samples)
    {
      if (valid_length_of_read_audio_buf)
	if (read_size <= valid_length_of_read_audio_buf)
	{
	  memcpy(b, read_audio_buf, read_size);
	  valid_length_of_read_audio_buf -= read_size;
	  memmove(read_audio_buf, read_audio_buf + read_size,
		  valid_length_of_read_audio_buf);
	  read_samples += read_size / bytes_per_sample();
	  return TRUE;
	}
	else
	{
	  memcpy(b, read_audio_buf, valid_length_of_read_audio_buf);
	  b += valid_length_of_read_audio_buf;
	  int s = valid_length_of_read_audio_buf / bytes_per_sample();
	  read_samples += s;
	  samples -= s;
	  read_size -= valid_length_of_read_audio_buf;
	  valid_length_of_read_audio_buf = 0;
	}

      // Movie の音部分を読む
      if (type == TypeMOVIE)
      {
	if (movie_num_of_record == 0)
	  return 0 < read_samples;
	
	ifp->jump_to(movie_record->address);

	if (movie_record->type == Movie::Record::TypeAVI)
	{
	  check_read_audio_buf(movie_record->size);
	  ifp->read_BYTES(read_audio_buf, movie_record->size);
	  valid_length_of_read_audio_buf = movie_record->size;
	}
	else
	{
	  check_read_audio_buf(movie_record->size * 2);
	  BYTE *rab1 = read_audio_buf;
	  BYTE *rab2 = read_audio_buf + movie_record->size;
	  ifp->read_BYTES(rab2, movie_record->size);
	  valid_length_of_read_audio_buf = movie_record->size;
	  
	  if (bits == 16)
	  {
	    if (channels == 2) // Stereo
	    {
	      int half_size = movie_record->size / 2;
	      for (int j = 0; j < half_size; j += 2)
	      {
		*(rab1++) = rab2[j + 1            ];
		*(rab1++) = rab2[j                ];
		*(rab1++) = rab2[j + 1 + half_size];
		*(rab1++) = rab2[j +     half_size];
	      }
	    }
	    else // Monaural
	      for (int j = 0; j < movie_record->size; j += 2)
	      {
		*(rab1++) = rab2[j + 1            ];
		*(rab1++) = rab2[j                ];
	      }
	  }
	  else // 8
	  {
	    if (channels == 2) // Stereo
	    {
	      int half_size = movie_record->size / 2;
	      for (int j = 0; j < half_size; j++)
	      {
		*(rab1++) = rab2[j            ] + 0x80;
		*(rab1++) = rab2[j + half_size] + 0x80;
	      }
	    }
	    else // Monaural
	    {
	      for (int j = 0; j < movie_record->size; j ++)
		*(rab1++) = rab2[j] + 0x80;
	    }
	  }
	}
	
	movie_num_of_record--;
	movie_record++;
      }
      
      // ADP の XA ADPCM を読む
      else if (type == TypeADP)
      {
	if (!adp_num_of_blocks)
	  return 0 < read_samples;
	adp_num_of_blocks--;
	
	int size = adp_samples_per_block * bytes_per_sample();
	check_read_audio_buf(size);
	
	XA_BLOCK xa_block;
	ifp->read_BYTES((BYTE *)&xa_block, sizeof(xa_block));
	valid_length_of_read_audio_buf = size;
	
	BYTE *rab = read_audio_buf;
	for (int i = 0; i < 18; i++)
	  if (channels == 2)
	    for (int j = 0; j < 8; j += 2)
	    {
	      int n = decode_adpcm(&xa_block.adpcm[i], adp_pcm_l, j,
				   adp_pcm_l[27], adp_pcm_l[26]);
	      decode_adpcm(&xa_block.adpcm[i], adp_pcm_r, j + 1,
			   adp_pcm_r[27], adp_pcm_r[26]);
	      for (int k = 0; k < n; k++)
	      {
		*(rab++) = (BYTE) adp_pcm_l[k];
		*(rab++) = (BYTE)(adp_pcm_l[k] >> 8);
		*(rab++) = (BYTE) adp_pcm_r[k];
		*(rab++) = (BYTE)(adp_pcm_r[k] >> 8);
	      }
	    }
	  else
	    for (int j = 0; j < 8; j++)
	    {
	      int n = decode_adpcm(&xa_block.adpcm[i], adp_pcm_l, j,
				   adp_pcm_l[27], adp_pcm_l[26]);
	      for (int k = 0; k < n; k++)
	      {
		*(rab++) = (BYTE) adp_pcm_l[k];
		*(rab++) = (BYTE)(adp_pcm_l[k] >> 8);
	      }
	    }
      }
      
      // WAV の音部分を読む
      else if (type == TypeWAV)
      {
	if (wav_total_size <= 0)
	  return 0 < read_samples;
	
	int size = (wav_nAvgBytesPerSec + 3) / 4 * 4;
	if (wav_total_size < size)
	  size = wav_total_size;
	wav_total_size -= size;
	check_read_audio_buf(size);
	ifp->read_BYTES(read_audio_buf, size);
	valid_length_of_read_audio_buf = size;
      }
      else
	return FALSE;
    }
    return TRUE;
  }
  
public:
  Audio(File *_ifp)
    : type(TypeNONE),
      ifp(_ifp),
      channels(0),
      rate(0),
      total_size(0),
      max_size(0),
      does_output_as_msadpcm(FALSE),
      does_output_as_xaadpcm(FALSE),
      // Movie 用データ
      movie_record(NULL),
      movie_num_of_record(0),
      // ADP 用データ
      adp_bits(4),
      adp_num_of_blocks(0),
      adp_delay_blocks(0),
      // WAV 用データ
      wav_wFormatTag(0),
      wav_nChannels(0),
      wav_nSamplesPerSec(0),
      wav_nAvgBytesPerSec(0),
      wav_nBlockAlign(0),
      wav_wBitsPerSample(0),
      wav_total_size(0),
      // バッファ
      read_audio_buf(NULL),
      sizeof_read_audio_buf(0),
      valid_length_of_read_audio_buf(0),
      buf(NULL),
      sizeof_buf(0)
  {
    adp_pcm_l[26] = 0;
    adp_pcm_l[27] = 0;
    adp_pcm_r[26] = 0;
    adp_pcm_r[27] = 0;
  }
  
  ~Audio()
  {
    delete [] read_audio_buf;
    delete [] buf;
  }

  // 音声データが存在するかどうか
  BOOL DoesExist(void) { return type != TypeNONE; }
  
  // すでに書き込んだ音声データチャンクのうちで最大サイズチャンクのサイズ
  int GetMaxSize(void) { return max_size; }

  // sec 秒遅らせるように設定
  void SetAudioDelay(double sec)
  {
    // SetMovie または ReadHeader よりも後で
    // WriteAudio より前でないとないと意味がない
    if (0 < sec && DoesExist())
      if (does_output_as_xaadpcm)
	adp_delay_blocks = (int)(sec * rate / adp_samples_per_block);
      else
      {
	valid_length_of_read_audio_buf =
	  (int)(sec * rate * bytes_per_sample());
	check_read_audio_buf(valid_length_of_read_audio_buf);
	memset(read_audio_buf, (bits == 16) ? 0 : 0x80,
	       valid_length_of_read_audio_buf);
      }
  }

  // MS-ADPCM で出力するかどうか設定する
  void SetDoesOutputAsMsadpcm(BOOL _does_output_as_msadpcm)
  {
    does_output_as_msadpcm = _does_output_as_msadpcm;
  }
  
  // XA-ADPCM で出力するかどうか設定する
  void SetDoesOutputAsXaadpcm(BOOL _does_output_as_xaadpcm)
  {
    does_output_as_xaadpcm = _does_output_as_xaadpcm;
    if (does_output_as_xaadpcm && type != TypeADP)
      ERROR("-xaadpcm error: cannot use -xaadpcm option without ADP audio file");
  }

  // 周波数を強制的に設定
  void SetFrequency(int rate)
  {
    if (0 < rate)
    {
      this->rate = rate;
      printf("\tassume audio frequency as %dHz.\n", rate);
    }
  }
  
  // 音部分を書く
  BOOL WriteAudio(File *ofp, double &sec)
  {
    if (!DoesExist())
      return FALSE;
    int samples = (int)(sec * rate);
    int mspb = 0;
    if (does_output_as_xaadpcm)
    {
      if (adp_delay_blocks == 0 && adp_num_of_blocks == 0)
	return FALSE;
      samples = samples / adp_samples_per_block * adp_samples_per_block;
    }
    else
    {
      if (does_output_as_msadpcm)
      {
	mspb = msadpcm_samples_per_block();
	samples = samples / mspb * mspb;
      }
      if (!read_audio(samples))
	return FALSE;
    }
    
    ofp->write_FOURCC("01wb");
    long check_01wb = ofp->check_length();
    
    long wrote_audio_size; // 書き込んだサイズのうち、音の部分
    if (does_output_as_msadpcm)
    {
      int n = (int)(sec * rate) / mspb;
      // 上のは (sec * rate) であって、(samples) と書いてはいけない
      for (int i = 0; i < n; i++)
	msadpcm_encode(ofp, buf + mspb * bytes_per_sample() * i);
      wrote_audio_size = n * msadpcm_nBlockAlign();
    }
    else if (does_output_as_xaadpcm)
    {
      int blocks = samples / adp_samples_per_block;
      samples = 0;

      if (0 < adp_delay_blocks)
      {
	XA_ADPCM xa_adpcm;
	memset(xa_adpcm.sf_and_filter, 0xc, sizeof(xa_adpcm.sf_and_filter));
	memset(xa_adpcm.data,            0, sizeof(xa_adpcm.data         ));
	while (0 < adp_delay_blocks && 0 < blocks)
	{
	  for (int i = 0; i < 18; i++)
	    ofp->write_BYTES((BYTE *)&xa_adpcm, sizeof(xa_adpcm));
	  adp_delay_blocks--;
	  blocks--;
	  samples += adp_samples_per_block;
	}
      }
      
      if (adp_num_of_blocks < blocks)
	blocks = adp_num_of_blocks;
      adp_num_of_blocks -= blocks;
      
      XA_BLOCK xa_block;
      wrote_audio_size = blocks * sizeof(xa_block.adpcm);
      for (int i = 0; i < blocks; i++)
      {
	ifp->read_BYTES((BYTE *)&xa_block, sizeof(XA_BLOCK));
	ofp->write_BYTES((BYTE *)&(xa_block.adpcm), sizeof(xa_block.adpcm));
	samples += adp_samples_per_block;
      }
    }
    else
    {
      wrote_audio_size = samples * bytes_per_sample();
      ofp->write_BYTES(buf, wrote_audio_size);
    }
    
    int wrote_length = ofp->write_length(check_01wb); // 書き込んだサイズ
    if (max_size < wrote_length)
      max_size = wrote_length;
    total_size += wrote_audio_size;
    
    sec = (double)samples / rate;
    
    return TRUE;
  }

  // AVI ヘッダの中の音声部分のヘッダを書く
  BOOL WriteHeader(File *ofp)
  {
    if (!DoesExist())
      return FALSE;
    
    ofp->write_FOURCC("LIST");
    long check_strl = ofp->check_length();
    {
      ofp->write_FOURCC("strl");
      
      int align;
      int samples_per_align;
      int wFormatTag;
      if (does_output_as_msadpcm)
      {
	align = msadpcm_nBlockAlign();
	samples_per_align = msadpcm_samples_per_block();
	wFormatTag = 2; // WAVE_FORMAT_ADPCM
      }
      else if (does_output_as_xaadpcm)
      {
	align = sizeof(XA_ADPCM);
	samples_per_align = adp_samples_per_block / 18;
	wFormatTag = 0x99; // 'X' + 'A'
      }
      else
      {
	align = bytes_per_sample();
	samples_per_align = 1;
	wFormatTag = 1; // WAVE_FORMAT_PCM
      }
      
      ofp->write_FOURCC("strh");
      long check_strh = ofp->check_length();
      ofp->write_FOURCC("auds");              // fccType
      ofp->write_DWORD_l(0);                  // fccHandler
      ofp->write_DWORD_l(0);                  // dwFlags;
      ofp->write_WORD_l(0);                   // wPriority
      ofp->write_WORD_l(0);                   // wLanguage
      ofp->write_DWORD_l(0);                  // dwInitialFrames
      ofp->write_DWORD_l(align);              // dwScale
      ofp->write_DWORD_l(rate * align /
			 samples_per_align);  // dwRate
      // dwRate / dwScale == samples/second
      ofp->write_DWORD_l(0);                  // dwStart
      ofp->write_DWORD_l(total_size / align); // dwLength
      ofp->write_DWORD_l(max_size);           // dwSuggestedBufferSize
      ofp->write_DWORD_l(0);                  // dwQuality
      ofp->write_DWORD_l(align);              // dwSampleSize
      ofp->write_DWORD_l(0);                  // 
      ofp->write_DWORD_l(0);                  // 
      ofp->write_length(check_strh);
      
      ofp->write_FOURCC("strf");
      long check_strf = ofp->check_length();
      ofp->write_WORD_l(wFormatTag);          // wFormatTag
      ofp->write_WORD_l(channels);            // nChannels
      ofp->write_DWORD_l(rate);               // nSamplesPerSec
      ofp->write_DWORD_l(rate * align /
			 samples_per_align);  // nAvgBytesPerSec
      ofp->write_WORD_l(align);               // nBlockAlign
      if (does_output_as_msadpcm)
      {
	ofp->write_WORD_l(4);                 // wBitsPerSample
	ofp->write_WORD_l(32);                // cbSize
	ofp->write_WORD_l(samples_per_align); // wSamplesPerBlock
	ofp->write_WORD_l(MSADPCM_NUM_COEF);  // wNumCoef
	for (int i = 0; i < MSADPCM_NUM_COEF; i++)
	{
	  ofp->write_WORD_l(msadpcm_coef1[i]); // iCoef1
	  ofp->write_WORD_l(msadpcm_coef2[i]); // iCoef2
	}
      }
      else if (does_output_as_xaadpcm)
      {
	ofp->write_WORD_l(4);                 // wBitsPerSample
	ofp->write_WORD_l(0);                 // cbSize
      }
      else
	ofp->write_WORD_l(bits);              // wBitsPerSample
      ofp->write_length(check_strf);	
    }
    ofp->write_length(check_strl);
    
    return TRUE;
  }
  
  // Movie が音声を持っている場合
  void SetMovie(Movie *movie)
  {
    ifp                 = movie->GetIfp();
    channels            = movie->GetAudioChannels();
    bits                = movie->GetAudioBits();
    rate                = movie->GetAudioRate();
    movie_num_of_record = movie->GetNumOfAudioRecord();
    movie_record        = movie->GetAudioRecord();
    
    type = channels ? TypeMOVIE : TypeNONE;
  }

  // 音声ヘッダを読む
  void ReadHeader(void)
  {
    char buf[5];
    buf[4] = '\0';
    
    DWORD magic = ifp->read_DWORD_b();
    if (magic == dword('F', 'O', 'R', 'M'))
    {
      type = TypeADP;
      
      ifp->read_skip(4);
      if (ifp->read_DWORD_b() != dword('A', 'I', 'F', 'F'))
	ERROR("AIFF not found: this is not XA-ADPCM AIFF");
      if (ifp->read_DWORD_b() != dword('C', 'O', 'M', 'M'))
	ERROR("COMM not found: this is not XA-ADPCM AIFF");
      long end_of_COMM = ifp->read_DWORD_b();
      end_of_COMM += ifp->ftell();
      
      channels         =      ifp->read_WORD_b();
      int sampleframes =      ifp->read_DWORD_b();   // samples/channel
      adp_bits         =      ifp->read_WORD_b();    // bits/sample
      rate             = (int)ifp->read_FLOAT80_b(); // sampleframes/sec
      
      ifp->jump_to(end_of_COMM);
      
      if (ifp->read_DWORD_b() != dword('A', 'P', 'C', 'M'))
	ERROR("APCM not found: this is not XA-ADPCM AIFF");
      long sizeof_APCM = ifp->read_DWORD_b();
      /* offset */       ifp->read_skip(4); 
      int block_size   = ifp->read_DWORD_b();
      if (block_size != sizeof(XA_BLOCK))
	ERROR("unsupported block size: cannot handle this XA-ADPCM AIFF");
      
      adp_num_of_blocks = (sizeof_APCM - 8) / block_size;
      adp_samples_per_block = 28 * 8 * 18 / channels;
      
      bits = 16;
      
      printf("SEGA Saturn XA-ADPCM AIFF, %s %dbits %dHz, %.2fsec\n",
	     (channels == 2) ? "Stereo" : "Monaural",
	     adp_bits, rate, sampleframes / (double)rate);
    }
    
    else if (magic == dword('R', 'I', 'F', 'F'))
    {
      type = TypeWAV;
      
      ifp->read_skip(4);
      if (ifp->read_DWORD_b() != dword('W', 'A', 'V', 'E'))
	ERROR("WAVE not found: this is not WAVE");
      if (ifp->read_DWORD_b() != dword('f', 'm', 't', ' '))
	ERROR("fmt  not found: this is not WAVE");
      long end_of_fmt_ = ifp->read_DWORD_l();
      end_of_fmt_ += ifp->ftell();
      
      wav_wFormatTag      = ifp->read_WORD_l();
      wav_nChannels       = ifp->read_WORD_l();
      wav_nSamplesPerSec  = ifp->read_DWORD_l();
      wav_nAvgBytesPerSec = ifp->read_DWORD_l();
      wav_nBlockAlign     = ifp->read_WORD_l();
      wav_wBitsPerSample  = ifp->read_WORD_l();
      ifp->jump_to(end_of_fmt_);
      
      while (ifp->read_DWORD_b() != dword('d', 'a', 't', 'a'))
      {
	long size = ifp->read_DWORD_l();
	ifp->read_skip(size);
      }
      wav_total_size  = ifp->read_DWORD_l();
      
      channels   = wav_nChannels;
      rate       = wav_nSamplesPerSec;
      bits       = wav_wBitsPerSample;
      
      printf("WAVE, %s %dbits %dHz, %.2fsec\n",
	     (channels == 2) ? "Stereo" : "Monaural",
	     bits, rate, wav_total_size / (double)wav_nAvgBytesPerSec);
    }
    
    else
      ERROR("magic not found: this file is neither XA-ADPCM AIFF nor WAVE");
  }
  
private:
  
  // MSADPCM 用データ
  static const INT16 msadpcm_coef1[MSADPCM_NUM_COEF];
  static const INT16 msadpcm_coef2[MSADPCM_NUM_COEF];

  WORD msadpcm_nBlockAlign(void)
  {
    if (rate <= 11025)
      return 256;
    else if (rate <= 22050)
      return 512;
    else
      return 1024;
  }

  DWORD msadpcm_samples_per_block(void)
  {
    return (((msadpcm_nBlockAlign() - (7 * channels)) * 8)
	    / (4 /* == wBitsPerSample */ * channels)) + 2;
  }
  
  INT16 msadpcm_encode_first_delta(INT16 coef1, INT16 coef2,
				   INT16 p5, INT16 p4, INT16 p3,
				   INT16 p2, INT16 p1)
  {
    INT32 total, tmp;
    
    //  use average of 3 predictions
    tmp    = (((INT32)p5 * coef2) + ((INT32)p4 * coef1)) >> MSADPCM_CSCALE;
    total  = (p3 < tmp) ? (tmp - p3) : (p3 - tmp);
    tmp    = (((INT32)p4 * coef2) + ((INT32)p3 * coef1)) >> MSADPCM_CSCALE;
    total += (p2 < tmp) ? (tmp - p2) : (p2 - tmp);
    tmp    = (((INT32)p3 * coef2) + ((INT32)p2 * coef1)) >> MSADPCM_CSCALE;
    total += (p1 < tmp) ? (tmp - p1) : (p1 - tmp);
    
    //  optimal iDelta is 1/4 of prediction error
    INT16 r = (INT16)(total / 12);
    if (r < MSADPCM_DELTA4_MIN)
      r = MSADPCM_DELTA4_MIN;
    return r;
  }

#define ENCODE_DELTA_LOOKAHEAD  (5)
  
  BOOL msadpcm_encode(File *ofp, BYTE *start)
  {
    static INT16 p4[] = { 230, 230, 230, 230, 307, 409, 512, 614,
			  768, 614, 512, 409, 307, 230, 230, 230 };
    DWORD total_error[MSADPCM_NUM_COEF][MSADPCM_MAX_CHANNELS];
    INT16 first_delta[MSADPCM_NUM_COEF][MSADPCM_MAX_CHANNELS];
    INT16 delta_c[MSADPCM_MAX_CHANNELS];
    INT16 samp1_c[MSADPCM_MAX_CHANNELS];
    INT16 samp2_c[MSADPCM_MAX_CHANNELS];

    int i, m, n;

    int samples = msadpcm_samples_per_block();
    
    // find the optimal predictor for each channel
    for (i = 0; i < MSADPCM_NUM_COEF; i++)
    {
      BYTE *b = start;
      
      INT16 coef1 = msadpcm_coef1[i];
      INT16 coef2 = msadpcm_coef2[i];

      INT16 samples_buf[ENCODE_DELTA_LOOKAHEAD][MSADPCM_MAX_CHANNELS];
      
      for (n = 0; n < ENCODE_DELTA_LOOKAHEAD; n++)
	for (m = 0; m < channels; m++)
	{
	  INT16 s;
	  if (bits == 16)
	  {
	    s = word(b[1], b[0]);
	    b += 2;
	  }
	  else // bits == 8
	    s = ((INT16)*(b++) - 128) << 8;
	  samples_buf[n][m] = s;
	}

      for (m = 0; m < channels; m++)
      {
	total_error[i][m] = 0L;
	samp1_c[m] = samples_buf[1][m];
	samp2_c[m] = samples_buf[0][m];
	
	//  calculate initial delta
	INT16 delta = msadpcm_encode_first_delta
	  (coef1, coef2, samples_buf[0][m], samples_buf[1][m],
	   samples_buf[2][m], samples_buf[3][m], samples_buf[4][m]);
	delta_c[m] = first_delta[i][m] = delta;
      }

      //  step over first two complete samples
      b = start + bytes_per_sample() * 2;
      
      //  now encode the rest of the PCM data in this block
      for (n = 2; n < samples; n++)
	for (m = 0; m < channels; m++)
	{
	  INT16 samp1 = samp1_c[m];
	  INT16 samp2 = samp2_c[m];
	  INT16 delta = delta_c[m];
	  
	  INT32 prediction = ((INT32)samp1 * coef1 +
			      (INT32)samp2 * coef2) >> MSADPCM_CSCALE;

	  INT16 s;
	  if (bits == 16)
	  {
	    s = word(b[1], b[0]);
	    b += 2;
	  }
	  else // bits == 8
	    s = ((INT16)*(b++) - 128) << 8;

	  //  encode it
	  INT32 error = (INT32)s - prediction;
	  INT16 output = (INT16)(error / delta);
	  if (MSADPCM_OUTPUT4_MAX < output)
	    output = MSADPCM_OUTPUT4_MAX;
	  else if (output < MSADPCM_OUTPUT4_MIN)
	    output = MSADPCM_OUTPUT4_MIN;

	  INT32 samp = prediction + ((INT32)delta * output);
	  if (32767 < samp)
	    samp = 32767;
	  else if (samp < -32768)
	    samp = -32768;
        
	  //  compute the next delta
	  delta = (INT16)((p4[output & 15] * (INT32)delta)
			  >> MSADPCM_PSCALE);
	  if (delta < MSADPCM_DELTA4_MIN)
	    delta = MSADPCM_DELTA4_MIN;
	  
	  delta_c[m] = delta;
	  samp2_c[m] = samp1;
	  samp1_c[m] = (INT16)samp;
	  
	  error = samp - s;
	  total_error[i][m] += (error * error) >> 7;
	}
      }

    // it's time to find the one that produced the lowest error
    BYTE best_predictor[MSADPCM_MAX_CHANNELS];
    for (m = 0; m < channels; m++)
    {
      best_predictor[m] = 0;
      DWORD te = total_error[0][m];
      for (i = 1; i < MSADPCM_NUM_COEF; i++)
	if (total_error[i][m] < te)
	{
	  best_predictor[m] = i;
	  te = total_error[i][m];
	}
    }
    
    //  reset pointer
    BYTE *b = start;
    
    // grab first delta from our precomputed first deltas that we
    // calculated above
    for (m = 0; m < channels; m++)
    {
      int i = best_predictor[m];
      delta_c[m] = first_delta[i][m];
    }
    
    //  get the first two samples from the source data
    if (bits == 16)
    {
      for (m = 0; m < channels; m++, b += 2)
	samp2_c[m] = word(b[1], b[0]);
      for (m = 0; m < channels; m++, b += 2)
	samp1_c[m] = word(b[1], b[0]);
    }
    else
    {
      for (m = 0; m < channels; m++)
	samp2_c[m] = ((INT16)*(b++) - 128) << 8;
      for (m = 0; m < channels; m++)
	samp1_c[m] = ((INT16)*(b++) - 128) << 8;
    }
    
    // write the block header for the encoded data
    for (m = 0; m < channels; m++)
      ofp->write_BYTE(best_predictor[m]);
    
    // now write the 2 byte delta per channel...
    for (m = 0; m < channels; m++)
      ofp->write_WORD_l(delta_c[m]);

    // finally, write the first two samples (2 bytes each per channel)
    for (m = 0; m < channels; m++)
      ofp->write_WORD_l(samp1_c[m]);
    for (m = 0; m < channels; m++)
      ofp->write_WORD_l(samp2_c[m]);
    
    // now write the data chunk
    BOOL is_first_nibble = TRUE;
    WORD next_write;
    for (n = 2; n < samples; n++)
      for (m = 0; m < channels; m++)
      {
	i = best_predictor[m];
	INT16 coef1 = msadpcm_coef1[i];
	INT16 coef2 = msadpcm_coef2[i];

	//  copy into cheaper variables because we access them a lot
	INT16 samp1 = samp1_c[m];
	INT16 samp2 = samp2_c[m];
	INT16 delta = delta_c[m];

	INT32 prediction = ((INT32)samp1 * coef1 +
			    (INT32)samp2 * coef2) >> MSADPCM_CSCALE;
	INT16 s;
	if (bits != 8)
	{
	  s = word(b[1], b[0]);
	  b += 2;
	}
	else // bits == 8
	  s = ((INT16)*(b++) - 128) << 8;

	//  encode the sample
	INT32 error = (INT32)s - prediction;
	INT16 output = (INT16)(error / delta);
	if (MSADPCM_OUTPUT4_MAX < output)
	  output = MSADPCM_OUTPUT4_MAX;
	else if (output < MSADPCM_OUTPUT4_MIN)
	  output = MSADPCM_OUTPUT4_MIN;

	INT32 samp = prediction + ((INT32)delta * output);
	if (32767 < samp)
	  samp = 32767;
	else if (samp < -32768)
	  samp = -32768;

	//  compute the next delta
	delta = (INT16)((p4[output & 15] * (INT32)delta)
			>> MSADPCM_PSCALE); 
	if (delta < MSADPCM_DELTA4_MIN)
	  delta = MSADPCM_DELTA4_MIN;

	delta_c[m] = delta;
	samp2_c[m] = samp1;
	samp1_c[m] = (INT16)samp;
	
	if (is_first_nibble)
	{
	  next_write = (output & 15) << 4;
	  is_first_nibble = FALSE;
	}
	else
	{
	  ofp->write_BYTE((BYTE)(next_write | (output & 15)));
	  is_first_nibble = TRUE;
	}
      }
    
    return TRUE;
  }
};


const INT16 Audio::msadpcm_coef1[MSADPCM_NUM_COEF] =
{ 256, 512, 0, 192, 240, 460, 392 };
const INT16 Audio::msadpcm_coef2[MSADPCM_NUM_COEF] =
{ 0, -256, 0, 64, 0, -208, -232 };


#endif // __audio_h__

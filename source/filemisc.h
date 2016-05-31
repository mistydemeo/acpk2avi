///////////////////////////////////////////////////////////////////////////////
// filemisc.h


#ifndef __filemisc_h__
#define __filemisc_h__


// ERROR(const char *), BYTE, WORD, DWORD, INT64 はあらかじめ設定しておくこと


inline DWORD dword(BYTE a, BYTE b, BYTE c, BYTE d)
{
  return
    (((DWORD)a) << 24) | (((DWORD)b) << 16) |
    (((DWORD)c) <<  8) | (((DWORD)d)      );
}


inline DWORD dword(const char *data)
{
  return dword((BYTE)data[0], (BYTE)data[1], (BYTE)data[2], (BYTE)data[3]);
}


inline WORD word(BYTE a, BYTE b)
{
  return (((WORD)a) <<  8) | ((WORD)b);
}


class File
{
public:
  FILE *fp;
  
  File(FILE *_fp = NULL) : fp(_fp) { }
  
  // 読む関数群
  
  void read_BYTES(BYTE *buf, int size)
  {
    if ((int)fread(buf, 1, size, fp) < size)
      ERROR("read error");
  }
  void read_BYTES(char *buf, int size) { read_BYTES((BYTE *)buf, size); }
  
  DWORD read_DWORD_b(void) // big endien DWORD を読む
  {
    BYTE buf[4];
    read_BYTES(buf, 4);
    return dword(buf[0], buf[1], buf[2], buf[3]);
  }
  
  WORD read_WORD_b(void) // big endien WORD を読む
  {
    BYTE buf[2];
    read_BYTES(buf, 2);
    return word(buf[0], buf[1]);
  }
  
  DWORD read_DWORD_l(void) // little endien DWORD を読む
  {
    BYTE buf[4];
    read_BYTES(buf, 4);
    return dword(buf[3], buf[2], buf[1], buf[0]);
  }

  WORD read_WORD_l(void) // little endien WORD を読む
  {
    BYTE buf[2];
    read_BYTES(buf, 2);
    return word(buf[1], buf[0]);
  }
  
  BYTE read_BYTE(void)
  {
    BYTE buf[1];
    read_BYTES(buf, 1);
    return buf[0];
  }
  
  double read_FLOAT80_b(void)
    // big endien 80bit float を読む
    // double の内部表現が IEEE 倍精度浮動小数点フォーマットに準拠していること
  {
    BYTE buf[10];
    read_BYTES(buf, 10);
    INT64 f =
      (((INT64)dword(buf[0], buf[1], buf[2], buf[3])) << 32) |
      (((INT64)dword(buf[4], buf[5], buf[6], buf[7]))      );
    INT64 mask1 = ((INT64)0xffff0000) << 32;
    f = (f & mask1) | ((f << 1) & ‾mask1);
    INT64 mask2 = ((INT64)0xc0000000) << 32;
    f = (f & mask2) | ((f << 4) & ‾mask2);
    return *(double *)&f;
  }
  
  void read_skip(int size) // 読み飛ばす
  {
    if (fseek(fp, size, SEEK_CUR) != 0)
      ERROR("read error");
  }
  
  // 書く関数群
  
  void write_BYTES(const BYTE *data, int size)
  {
    if ((int)fwrite(data, 1, size, fp) < size)
      ERROR("write error");
  }
  void write_FOURCC(const char *data) { write_BYTES((const BYTE *)data, 4); }
  
  void write_DWORD_l(DWORD data) // little endien DWORD で書く
  {
    BYTE buf[4] = { (BYTE)(data      ), (BYTE)(data >>  8),
		    (BYTE)(data >> 16), (BYTE)(data >> 24), };
    write_BYTES(buf, 4);
  }
  
  void write_WORD_l(WORD data) // little endien WORD で書く
  {
    BYTE buf[2] = { (BYTE)(data      ), (BYTE)(data >>  8), };
    write_BYTES(buf, 2);
  }
  
  void write_BYTE(BYTE data)
  {
    write_BYTES(&data, 1);
  }
  
  long check_length(void) // あとで大きさを書くためのチェック
  {
    write_DWORD_l(0);
    return ftell();
  }
  long check_place(void) { return check_length(); }
  
  long write_length(long from) // check_length で指定した位置に長さを書く
  {
    long to = ftell();
    if (to < 0)
      ERROR("write error");
    DWORD length = to - from;
    jump_to(from - 4);
    write_DWORD_l(length);
    jump_to(to);
    if (length % 2)
      write_BYTE(0);
    return length;
  }
  
  void write_DWORD_at(long place, DWORD data) // check_place した位置に値を書く
  {
    long now = ftell();
    if (now < 0)
      ERROR("write error");
    jump_to(place - 4);
    write_DWORD_l(data);
    jump_to(now);
  }

  // その他
  
  void jump_to(int to) // シーク
  {
    if (fseek(fp, to, SEEK_SET) != 0)
      ERROR("seek error");
  }
  
  long ftell(void) // ftell
  {
    return ::ftell(fp);
  }
  
  int fclose(void)
  {
    return ::fclose(fp);
  }
};


#endif // __filemisc_h__

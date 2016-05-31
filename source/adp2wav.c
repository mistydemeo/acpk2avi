/* adp2wav version 0.23
 * Copyright (C) 1997 Nobuyuki Nishizawa <nishi@gavo.t.u-tokyo.ac.jp>
 */

/* セガサターン版エヴァンゲリオン.adpファイル->.wavコンバータ */

/* 履歴
 *  version 0.1 [97/05/13]
 *   最初のバージョン。
 *  version 0.2 {97/05/14]
 *   モノラルなファイルの出力がうまくいかなかったのを修正。
 *   コメントを加筆・修正
 *  version 0.21 [97/05/14]
 *   0.2の修正は不十分(出力部のデータサイズ算出に誤りがあった)のを修正。
 *   コメントの修正。
 *  version 0.22 [97/05/16]
 *   C++ソースとしてコンパイルできない点(暗黙のキャストによる警告・エラー)
 *   を修正。(acpk2avi対策)
 *  version 0.23 [97/09/11]
 *   サンプリング周波数計算における、浮動小数点演算のバグの修正
 */

/* セガサターン版エヴァンゲリオン
 * .adpファイルの構造(推測)
 *
 * -------------------------------------
 * 型定義
 *   CARD8  : 符号無し8ビット整数
 *   CARD16 : 符号無し16ビット整数
 *   CARD32 : 符号無し32ビット整数
 *   FLOAT80: 80ビット浮動小数点数
 *   符号1ビット、指数部15ビット、仮数部64ビットと思われる。
 *   これはIntelのtemporary real形式に等しい。
 *   なお仮数部はケチ表現(economized representation)ではないので注意。
 *   指数部はバイアス表現である。
 *   これがIEEE準拠なのかどうかを私は知らない(^^; )
 * -------------------------------------
 * 
 * .adpファイルの基本的な構造はAIFFである。
 * 構造は以下の通り。
 * ただしバイトオーダは68000に等しい(すなわちbig-endian)。
 *
 * AIFF_chank:
 *      FORM_chank form
 *
 * FORM_chank:
 *      CARD8       'F','O','R','M'
 *      CARD32      size                ;sizeof(FORM_chank) - 8
 *      COMM_chank  comm
 *      APCM_chank  apcm
 *
 * COMM_chank:
 *      CARD8       'C','O','M','M'
 *      CARD32      size                ;sizeof(COMM_chank) - 8
 *      CARD16      channels
 *      CARD32      samples
 *      CARD16      bits_per_sample
 *      FLOAT80     sampling_frequency
 *
 * APCM_chank:
 *      CARD8       'A','P','C','M'
 *      CARD32      size                ;sizeof(APCM_chank) - 8
 *      CARD32      unknown
 *      CARD32      block_size
 *      XA_BLOCK    xa_block[n]      ;n = (size - 8) / block_size (?)
 *
 * サウンドデータはCD-XA type-C(B?)互換のADPCMと考えられる。(Fs = 18.9kHz)
 * XA_ADPCM部の構造は以下の通りと推測される。
 * (参考:xa2wave.c 佐藤正徳 氏)
 *
 * XA_BLOCK:
 *      XA_ADPCM    xa_adpcm[18]
 *      CARD8       unknown[20]
 *
 * XA_ADPCM:
 *      CARD8       sf_and_filter[16]   ;上位4ビット:スケールファクタ
 *                                      ;下位4ビット:フィルタ定数選択
 *      CARD8       adpcm_data[112]     ;244サンプルadpcmデータ
 *                                      ;(1サンプル4ビット)
 * (デコードの詳細についてはソースを見てください(^^;)
 *
 * なおsf_and_filterが4バイトを1ブロックとし、同じデータが
 * それぞれ2回連続しているようであるが、
 * これがXAの規格上求められているかどうかは不明である。
 * (XA ADPCMについては別に解説を書くかもしれません。)
 *
 * 注1) これをほとんど書き終わった後で
 *  http://nanya-www.cs.titech.ac.jp/‾yatsushi/xaadpcm.html (山崎 淳 氏)
 *  にもXA ADPCMの解説があるのを見つけた。
 *  この作者達はどのようにしてXA ADPCMのフォーマットを得たのであろうか。
 *
 * 注2) xa2wave.cの実装ではノイズが入る可能性がある。
 *  この原因は2つ考えられ、デコーダのIIRフィルタの初期値をブロックごとに
 *  0クリアしていることと、IIRフィルタの乗算時に計算結果がオーバフローする
 *  ことが有りうることである。(後者は実際に確認したわけではないが。)
 *
 * 注3) 現時点では佐藤氏・山崎氏ともに了解を得ていないため、
 *  このファイルの配布はまずそうである(^^;
 */

/* 現バージョンにおける問題点
 * 
 * .adp（AIFF)のチェックが不十分。
 * 動作確認環境が少なすぎる。
 *
 * 動作確認環境
 * VC++4.1(NT4.0) 4000.adpについてのみコンバートできることを確認。
 * その他のファイルは試していない(試しようがない(^^; )
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef signed long  INT32;
typedef signed short INT16;
typedef signed char  INT8;

typedef unsigned long  CARD32;
typedef unsigned short CARD16;
typedef unsigned char  CARD8;

typedef CARD32 DWORD;
typedef CARD16 WORD;
typedef CARD8  BYTE;

typedef struct {
	char sf_and_filter[16];
	char data[112];
} XA_ADPCM;

typedef struct {
	XA_ADPCM adpcm[18];
	char     unknown[20];
} XA_BLOCK;


int decode_adpcm(XA_ADPCM*, INT16*, int, INT16, INT16);
long read_aiff_header(FILE*, long*, int*);
void write_wav_header(FILE*, long, int, long);
int write_wav_data(FILE*, INT16*, INT16*, int);
int get_adpcm_data(XA_ADPCM*, int, int);
int get_adpcm_filter(XA_ADPCM*, int);
int get_adpcm_scalefactor(XA_ADPCM*, int);
void set_CARD16_l(CARD8*, CARD16);
void set_CARD32_l(CARD8*, CARD32);


int main(int argc, char *argv[])
{
	int i, j;
	int n;
	int channels;
	long freq;
	long total_size;
	long block, num_block;
	FILE *fp_adp;
	FILE *fp_wav;
	XA_BLOCK xa_block;
	INT16 pcm_l[28];
	INT16 pcm_r[28];

	fprintf(stderr, "adp2wav version 0.22  Copyright (C) 1997 Nobuyuki NISHIZAWA\n");

	if (argc != 3)
	{
		fprintf(stderr, "Usage: adp2wav <input.adp> <output.wav>\n");
		return 255;
	}

	fp_adp = fopen(argv[1], "rb");
	fp_wav = fopen(argv[2], "wb");
	if (fp_adp == NULL || fp_wav == NULL)
	{
		fprintf(stderr, "Error: cannot open file.\n");
	}

	num_block = read_aiff_header(fp_adp, &freq, &channels);
	
	//fseek(fp_adp, 0x36, SEEK_SET);
	fseek(fp_wav, 0x2c, SEEK_SET);
	total_size = 0;
	pcm_l[26] = 0;
	pcm_l[27] = 0;
	pcm_r[26] = 0;
	pcm_r[27] = 0;
	
	block = 0;
	while (fread(&xa_block, sizeof(xa_block), 1, fp_adp) == 1 &&
		   block < num_block)
	{
		for (i = 0; i < 18; i++)
		{
			if (channels == 2)
			{
				for (j = 0; j < 8; j += 2)
				{
					n = decode_adpcm(&xa_block.adpcm[i], pcm_l, j,
									 pcm_l[27], pcm_l[26]);
				
					decode_adpcm(&xa_block.adpcm[i], pcm_r, j + 1,
								 pcm_r[27], pcm_r[26]);
				
					total_size += write_wav_data(fp_wav, pcm_l, pcm_r, n);
				}
			}
			else
			{
				for (j = 0; j < 8; j++)
				{
					n = decode_adpcm(&xa_block.adpcm[i], pcm_l, j,
									 pcm_l[27], pcm_l[26]);
				
					total_size += write_wav_data(fp_wav, pcm_l, NULL, n);
				}
			}
		}
	}

	write_wav_header(fp_wav, freq, channels, total_size);

	fclose(fp_wav);
	fclose(fp_adp);

	return 0;
}

int decode_adpcm(XA_ADPCM *adpcm, INT16 *pcm, int unit, INT16 t1, INT16 t2)
{
	int i;
	int sf, fl;
	int d;
	INT16 t;
	INT32 u;

	static INT32 K1[4] = {0, 0x0f0,  0x1cc,  0x188};
	static INT32 K2[4] = {0, 0,       -0x0d0, -0x0dc};

	sf = get_adpcm_scalefactor(adpcm, unit);
	fl = get_adpcm_filter(adpcm, unit);
	
	for (i = 0; i < 28; i++)
	{
		d = (long)get_adpcm_data(adpcm, unit, i);
	
		u = d * (1 << (sf));
		u += (K1[fl] * t1 + K2[fl] * t2) / 256;
		u = (u > 32767) ? 32767 : u;
		u = (u < -32768) ? -32768 : u;
		t = (INT16)u;

		t2 = t1;
		t1 = t;

		pcm[i] = t;
	}
	return i;
}

long read_aiff_header(FILE *fp_adp, long *freq, int *channels)
{
	static CARD8 form[4] = {'F', 'O', 'R', 'M'};
	static CARD8 aiff[4] = {'A', 'I', 'F', 'F'};
	static CARD8 comm[4] = {'C', 'O', 'M', 'M'};
	static CARD8 apcm[4] = {'A', 'P', 'C', 'M'};

	CARD8 d[256];
	long size;
	long samples;
	int  bitspersample;
	long block_size;
	unsigned long f;
	int shift;

	fread(d, 12, 1, fp_adp);
	if (memcmp(d, form, 4) != 0 || memcmp(d + 8, aiff, 4) != 0)
	{
		fprintf(stderr, "not aiff file.\n");
		exit(255);
	}

	fread(d, 8, 1, fp_adp);
	if (memcmp(d, comm, 4) != 0)
	{
		fprintf(stderr, "unsupported aiff file.\n");
		exit(255);
	}
	size = 0x1000000 * d[4] + 0x10000 * d[5] + 0x100 * d[6] + d[7];
	fread(d, size, 1, fp_adp);

	*channels = 0x100 * d[0] + d[1];
	fprintf(stderr, "Channels : %d\n", *channels);
	
	samples = 0x1000000 * d[2] + 0x10000 * d[3] + 0x100 * d[4] + d[5];
	fprintf(stderr, "Samples : %ld\n", samples);

	bitspersample = 0x100 * d[6] + d[7];
	fprintf(stderr, "bits/sample : %d\n", bitspersample);

	shift = 0x100 * (d[8] & 0x7f) + d[9];
	f = 0x1000000 * d[10] + 0x10000 * d[11] + 0x100 * d[12] + d[13];
	f >>= (0x401e - shift);
	*freq = (long)f;
	fprintf(stderr, "Sampling frequency : %ld\n", *freq);

	fread(d, 8, 1, fp_adp);
	if (memcmp(d, apcm, 4) != 0)
	{
		fprintf(stderr, "not ADPCM(XA) file.\n");
		exit(255);
	}
	size = 0x1000000 * d[4] + 0x10000 * d[5] + 0x100 * d[6] + d[7];

	fread(d, 8, 1, fp_adp);
	block_size = 0x1000000 * d[4] + 0x10000 * d[5] + 0x100 * d[6] + d[7];
	if (block_size != sizeof(XA_BLOCK))
	{
		fprintf(stderr, "unsupported XA format.\n");
		exit(255);
	}
	
	return (size - 8) / block_size;
}

void write_wav_header(FILE *fp_wav, long freq, int channels, long total_size)
{
	static CARD8 riff[4] = {'R', 'I', 'F', 'F'};
	static CARD8 wave[4] = {'W', 'A', 'V', 'E'};
	static CARD8 fmt_[4] = {'f', 'm', 't', ' '};
	static CARD8 data[4] = {'d', 'a', 't', 'a'};

	CARD8 d[4];

	fseek(fp_wav, 0, SEEK_SET);

	fwrite(riff, 4, 1, fp_wav);  
	set_CARD32_l(d, total_size + 0x24);
	fwrite(d, 4, 1, fp_wav); 
	fwrite(wave, 4, 1, fp_wav);

	fwrite(fmt_, 4, 1, fp_wav);
	set_CARD32_l(d, 0x10); /* size */
	fwrite(d, 4, 1, fp_wav);
	set_CARD16_l(d, 1); /* format */
	fwrite(d, 2, 1, fp_wav);
	set_CARD16_l(d, (CARD16)channels);
	fwrite(d, 2, 1, fp_wav);
	set_CARD32_l(d, (CARD32)freq);
	fwrite(d, 4, 1, fp_wav);
	set_CARD32_l(d, (CARD32)(freq * channels * 2));
	fwrite(d, 4, 1, fp_wav);
	set_CARD16_l(d, (CARD16)(channels * 2)); /*nBlockAlign */
	fwrite(d, 2, 1, fp_wav);
	set_CARD16_l(d, 0x10); /*nBitsPerSample */
	fwrite(d, 2, 1, fp_wav);
	
	fwrite(data, 4, 1, fp_wav);  
	set_CARD32_l(d, total_size);
	fwrite(d, 4, 1, fp_wav);
}

int write_wav_data(FILE *fp_wav, INT16* pcm_l, INT16* pcm_r, int n)
{
	CARD8 d[4];
	int i;

	if (pcm_r != NULL)
	{
		for (i = 0; i < n; i++)
		{
			set_CARD16_l(&d[0], (CARD16)pcm_l[i]);
			set_CARD16_l(&d[2], (CARD16)pcm_r[i]);

			fwrite(&d, 4, 1, fp_wav);
		}
		return i * 4;
	}
	else
	{
		for (i = 0; i < n; i++)
		{
			set_CARD16_l(&d[0], (CARD16)pcm_l[i]);

			fwrite(&d, 2, 1, fp_wav);
		}
		return i * 2;
	}
}

int get_adpcm_data(XA_ADPCM *adpcm, int unit, int i)
{
	CARD8 d;

	d = adpcm->data[i * 4 + unit / 2];
	d = (unit % 2) ? d >> 4 : d & 0x0f;

	return (d > 7) ? -16 + (int)d : d;
}

int get_adpcm_filter(XA_ADPCM *adpcm, int unit)
{
	/* sf/filterの2個連続する4バイトの値が違うとき警告を発する */
	if (adpcm->sf_and_filter[(unit / 4) * 8 + unit % 4] !=
		adpcm->sf_and_filter[(unit / 4) * 8 + unit % 4 + 4])
	{
		fprintf(stderr, "CAUTION! Check scalefactor or filter value!\n"); 
	}	
	return adpcm->sf_and_filter[4 + unit] >> 4;
}

int get_adpcm_scalefactor(XA_ADPCM *adpcm, int unit)
{
	return 12 - (adpcm->sf_and_filter[4 + unit] & 0x0f);
}

void set_CARD16_l(CARD8 *d, CARD16 n)
{
	d[0] = (CARD8)n;
	d[1] = (CARD8)(n >> 8);
}

void set_CARD32_l(CARD8 *d, CARD32 n)
{
	d[0] = (CARD8)n;
	d[1] = (CARD8)(n >> 8);
	d[2] = (CARD8)(n >> 16);
	d[3] = (CARD8)(n >> 24);
}


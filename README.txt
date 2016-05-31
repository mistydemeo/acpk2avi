
			       acpk2avi

		  Another convertor from CPK to AVI.
		 Copyright (C) 1997-1998, GANA, Nishi

0. 要注意

	セガサターンのシネパック (CPK) のパソコン上での鑑賞や利用に関
	してセガ・エンタープライゼス社は公認していません. この件につい
	てのセガやソフトハウスへの質問などはご遠慮下さい.

1. はじめに

	これは, セガサターンで一般的に動画として使われているシネパック
	ファイルを Windows の AVI ファイルに変換するものです. 競合する
	ソフトウェアに対する長所としては, 

		1) 音声のない CPK に WAV や ADP を追加した AVI を作成できる
		2) ADP を WAV へ変換できる
		3) 作成される AVI の音声の圧縮ができる
		4) Windows95 OSR2 でもちゃんと再生できる AVI を作成する
		5) 作成される AVI のサイズが若干小さい
		6) Win32 他で動く

	などがあります. 

2. 使い方

	a) CPK ファイルから AVI ファイルを作成したい時
		acpk2avi [OPTIONS] <INPUT.cpk> <OUTPUT.avi>

	b) CPK ファイルと ADP ファイルから AVI ファイルを作成したい時
		acpk2avi [OPTIONS] <INPUT.cpk> <INPUT.adp> <OUTPUT.avi>

	c) CPK ファイルと WAV ファイルから AVI ファイルを作成したい時
		acpk2avi [OPTIONS] <INPUT.cpk> <INPUT.wav> <OUTPUT.avi>

	d) ADP ファイルから WAV ファイルを作成したい時
		acpk2avi <INPUT.adp> <OUTPUT.wav>

	e) バージョン 1.08 以前の acpk2avi で生成した AVI ファイルや, 
	   cpk2avi などで生成した AVI ファイルを Windows95 OSR2 でも再
	   生できるようにしたい時
		acpk2avi [OPTIONS] <INPUT.avi> <OUTPUT.avi>
		acpk2avi [OPTIONS] <INPUT.avi> <INPUT.adp> <OUTPUT.avi>
		acpk2avi [OPTIONS] <INPUT.avi> <INPUT.wav> <OUTPUT.avi>

	[OPTIONS] には以下のものを指定できます. 
	
	-msadpcm	音声を MS-ADPCM 圧縮します. 
	-xaadpcm	秘密. -msadpcm と同時に指定することはできません. 
	+<DELAY>	映像を <DELAY> 秒遅らせます. 
	-<DELAY>	音声を <DELAY> 秒遅らせます. 
	-e<EXTEND>	出力される AVI を <EXTEND> 秒長くします. 
	-fps<FPS>	FPS(frame/sec) 値を <FPS> だと仮定します. 
	-frequency<Hz>	音声の周波数を <Hz> Hz だと仮定します. 
	-freq<Hz>			〃

	Windows 95 の DOS 窓や Windows NT のコマンドプロンプトなどから
	起動してください.

	ファイル名は参照しないのでどんなファイル名でもかまいません. 例
	えば拡張子が .ABS でも中身がちゃんとした ADP ファイルなら 
	<INPUT.adp> のところに指定できます (「3. 使用例」の最後を参照)

	-fps<FPS> オプションを付けることによって, 強制的に再生する fps 
	値を設定できます. -fps24 とすると, 24 fps に設定されます. しか
	し, 実際の fps より小さい値にするとまともに再生できないでしょ
	う. このオプションは, acpk2avi が fps 値の算出に失敗した場合に, 
	適切な値を設定する目的のみに使うようにすべきです. ちなみに, 
	fps 値を増やしても減らしてもファイルサイズはほとんど変化しませ
	んが, 増やすと再生効率が悪くなります. 

3. 使用例

	・サターン版「新世紀工ヴァンゲリオン」の CD を Q ドライブに入
	れた状態で, 以下のようにすると, オープニングの AVI がカレント
	ディレクトリに作成されます. 

	  acpk2avi.exe Q:\4001.CPK Q:\4000.ADP EvaOpening.avi

	・サターン版「新世紀工ヴァンゲリオン」の 4001.CPK と音楽 CD の
	「NEON GENESIS EVANGELION II」のトラック 2 (TRACK2.WAV) からオー
	プニングを作成 (TRACK2.WAV は CD-DA 吸出しツールを利用して作成). 
	TRACK2.WAV は始まるタイミングが少々遅いので映像が始まるのを
	0.37 秒ほど送らせます. 

	  acpk2avi.exe +0.37 4001.CPK TRACK2.WAV EvaOpening.avi

	・サターン版「機動戦艦ナデツコ〜やっぱり最後は『愛が勝つ』?〜」
	のエンディングの ADP ファイルを WAV に変換. 

	  acpk2avi.exe WATASI.ABS 私らしく.wav

4. 注意

	・このソフトウェアはフリーウェアです. このソフトウェアに関係す
	る/しないに関わらず, いかなることについても作者は責任を負いま
	せん. 

	・転載などは御自由にどうぞ. 

	・うまく変換できないシネパックがあるかもしれません. そのときは
	ぜひ作者まで報告してください. 対処できるかもしれません. 

	・WAV 音声を合成するときの WAV はただの PCM 形式でなければなり
	ません. (PCM 形式以外も要望があればサポートしますが, 多分要望
	はないでしょう ^^;)

	・拡張子が ADP のファイルでも変換できないものがあるかもしれま
	せん. 変換できる例は, 「新世紀工ヴァンゲリオン」や「ソ二ック
	JAM」などです. 

	・拡張子が ADP でないファイルでも変換できる音声ファイルがあり
	ます. 例えば「機動戦艦ナデツコ」では拡張子が ACM と ABS のファ
	イルも変換できます. 一般的には, ファイルの先頭 48 バイトが, 以
	下の形式になっていれば変換できる可能性があります. 

0x00000000: 46 4f 52 4d .. .. .. .. - 41 49 46 46 43 4f 4d 4d FORM....AIFFCOMM
0x00000010: .. .. .. .. .. .. .. .. - .. .. .. .. .. .. .. .. ................
0x00000020: .. .. .. .. .. .. 41 50 - 43 4d .. .. .. .. .. .. ......APCM......

	"." の所はなんでもいいです. AIFF ファイルの ADPCM 形式ですね. 

	・最近では, CPK を直接再生/変換できる windvt2 (WV_102.LZH) (シェ
	アウェア) なるソフトも存在するようです. acpk2avi の存在意義が
	だんだん薄れてきましたね. 

	・adp2wav.c の全ての権利は Nishi 氏が保有し, GANA はほとんど手
	を加えておりません (改行コードを変えただけ). 又 ADP ファイルか
	ら WAV ファイルを作成する時は, acp2wav をそのまま起動している
	のと同じ動作をします. adp2wav ありがとうございます :) ＞Nishi

	・「新世紀工ヴァンゲリオン2nd」の 1000 番台, 2000 番台の CPK 
	ファイルは変換できません. これは, CPK という拡張子ではあるけど, 
	シネパックではないファイルだからです. 又 5105, 5267〜5271 番は
	うまく変換できてないように見えますが, 元々そういうデータなので
	しかたありません (じつは acpk2avi のバグだったりして…). 

	・version 1.11 以前では -msadpcm オプションをつけるとアクティ
	ブムービーでまともに再生できないファイルが作成されていました. 
	てっきりアクティブムービーのバグだと思ったのですが, Nishi 氏の
	指摘により acpk2avi のバグだということが判明しました. いいがか
	りでごめんなさいm(__)m＞アクティブムービー. 

	・最近サターンでは TrueMotion も多く使われるようになってきまし
	たが, TrueMotion には対応できません. もし何らかの資料があれば
	対応できるかも知れませんが, 現状では無理です. Windows で再生可
	能な TrueMotion 2.0 の AVI でもあれば解析できるかもしれないの
	ですが…でも忙しいので無理でしょう.

5. 参考にしたもの

	・cpk2avi で生成された AVI ファイル
	・X Cinepack Player のソース
	・adp2wav のソース
	・waveconv の中の msadpcm.c
	・Video for Windows のドキュメント
	などなど

6. コンパイル方法

	acpk2avi.exe は Windows95/NT 用ですが, コンパイルし直せば UNIX 
	などでも動きます. Makefile を若干修正して, 必要ならば文字
	コードを変更するだけでコンパイルできるでしょう.

	acpk2avi.exe は Visual C++ 5.0 がインストールされていれば, 

	C:\acpk2avi\source\> nmake -f Makefile.vc nodebug=1

	などとすれば作成できます. (同梱の acpk2avi.exe は上記のように
	して作成されています)

	以下の環境では, 

		Cygnus GNU-Win32-b18
		Debian GNU/Linux
		Solaris

	% make -f Makefile.gcc

	などとするだけで作成できます. 

	又, SunOS4 では memmove 関数がないのでコンパイルできません. 
	(memmove ぐらい自分で書けばいいですが ^^;) しかし UNIX でコン
	パイルできても, xanim ではうまく再生できない場合が多いのであま
	りうれしくはないのですが. 

7. 作者

	GANA - acpk2avi を作成
		TAGA Nayuta <nayuta@is.s.u-tokyo.ac.jp>

	Nishi - adp2wav を作成
		Nobuyuki Nishizawa <nishi@gavo.t.u-tokyo.ac.jp>

8. サポートホームページ

	http://www.is.s.u-tokyo.ac.jp/‾nayuta/acpk2avi/index-j.html

9. 履歴

	version 1.00 1997/04/13

	version 1.01 1997/04/14
		・biSizeImage の値を 0 に設定するようにした. 
		・RIFF chunk の WORD アラインメント処理をし忘れていた
		バグの修正. 

	version 1.02 1997/05/01
		・音声の入っていないシネパックに対応した. 

	version 1.03 1997/05/16
		・シネパックと音声を合成できるようにした. 

	version 1.04 1997/05/19
		・DELAY 機能を付けた. 
		・MS-ADPCM を中途半端に実装した. 

	version 1.05 1997/06/18
		・さくら大戦の FLY.CPK と FINAL.CPK がうまく変換できな
		かったのに伴う修正. 

	version 1.06 1997/06/18
		・音声の入っていないシネパックを変換しようとするとエラー
		を出すバグを修正. 

	version 1.07 1997/06/26
		・8bit の音声のシネパックを変換すると音がおかしくなる
		バグを修正. 

	version 1.08 1997/07/17
		・ステレオの音声が左右逆になってしまうバグの修正. 
		・-e<EXTEND> オプションの追加. 
		・秘密の機能 -xaadpcm の追加. (使おうとしても使い道が
		わからないと思います ^^;)

	version 1.09 1997/07/18
		・Windows95 OSR2 では画像が再生できないバグの修正. 

	version 1.10 1997/07/19
		・秘密の機能のせいで混入したまぬけなバグの退治. 

	version 1.11 1997/08/02
		・fps を算出時に 60 fps を超えることがないようにした. 
		・fps を設定できるようにした. 

	version 1.12 1997/08/10
		・MS-ADPCM 圧縮したものもアクティブムービーでうまく再
		生できるようになった. (thanks to Nishi)

	version 1.13 1997/08/21
		・バージョン 1.08 以前の acpk2avi で生成した AVI ファ
		イルや, cpk2avi などで生成した AVI ファイルを 
		Windows95 OSR2 でも再生できるように変換できるようになっ
		た. 

	version 1.14 1997/09/05
		・ADP ファイルから WAV ファイルを作成できなくなってい
		たバグを修正. 

	version 1.15 1997/09/11
		・AVI を出力する場合に音声の周波数を設定できるようにし
		た. 
		・ADP ファイルから WAV ファイルを作成する際の周波数計
		算における, 浮動小数点演算のバグの修正 (by Nishi). 

	version 1.16 1997/10/06
		・8bit の音声のシネパックで音声を遅らせるとノイズが入っ
		ていたかもしれないバグが修正されたかもしれない.

	version 1.17 1997/11/14
		・要注意を書いた.
		・Makefile.vc をつけた.

	version 1.18 1997/12/08
		・README.txt の記載内容のミスを修正.

	version 1.19 1998/01/27
		・avi から avi へのコンバート時に, 00dc ではなく 00id 
		  などでも画像チャンクとみなすようにした.

								文責:GANA

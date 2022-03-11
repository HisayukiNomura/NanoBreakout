#ifndef __block_h__
#define __block_h__

// 表示されるブロックの規定値
#define BLOCK_CNT_H	7           // ブロックは横に7個
#define BLOCK_CNT_V 21          // ブロックは縦に最大21個
#define BLOCK_SIZE_W 11         //  ブロックの横幅は11ドット
#define BLOCK_SIZE_H 6          // ブロックの縦は6ドット

// ブロックのテーブル
// 画面上はBLOCK_CNT_H x BLOCK_CNT_Vのマス目に分類され、そこにあるブロックの
// 種類が item に入っている。
// テーブルには、事前にこのブロックが存在する矩形の情報が入っており、ボールとの
//　衝突判定ではこの座標が使われる。
struct BLOCKINFO {
    unsigned char  item;
	unsigned char  x1;
	unsigned char  y1;
	unsigned char  x2;
	unsigned char  y2;
};

//全体のブロック数と、残りのブロック数。ブロックが少なくなったらボールの速度を上げる、などに使用する。
extern int blkCnt;			// 総ブロック数
extern int blkBrk;			// 残りブロック数

void  InitBlock(int Stage);
void DrawBlock();
void blockCheck(struct BALLINFO* bi);
#endif
#include "lcd/lcd.h"
#include "led.h"
#include "memory.h"
#include "gd32vf103.h"
#include "game.h"
#include "ball.h"
#include "wall.h"
#include "block.h"
#include "paddle.h"
#include "sound.h"


// ブロックのテーブル
// 画面上はBLOCK_CNT_H x BLOCK_CNT_Vのマス目に分類され、そこにあるブロックの
// 種類が item に入っている。
// テーブルには、事前にこのブロックが存在する矩形の情報が入っており、ボールとの
//　衝突判定ではこの座標が使われる。
struct BLOCKINFO blockmtx[BLOCK_CNT_H][BLOCK_CNT_V];

//全体のブロック数と、残りのブロック数。ブロックが少なくなったらボールの速度を上げる、などに使用する。
int blkCnt = 0;			// 総ブロック数
int blkBrk = 0;			// 残りブロック数


// ブロックのテーブルを初期化する
//
void InitBlock(int Stage)
{
	// ステージごとのブロック位置
	static int BlockStart[] = {3,4,5,2,2,1,0,0};
	static int BlockEnd[] =   {6,7,8,7,8,9,8,9};

	int stageWork = (Stage-1) & 0x7; // ステージは0～7を繰り返す
    // ブロックテーブルを初期化する。
	memset((void *)blockmtx,0,sizeof(blockmtx));
    blkCnt = 0;

    for (int i = 0;i<BLOCK_CNT_H;i++) {
        for (int j = BlockStart[stageWork];j<BlockEnd[stageWork];j++) {
            blockmtx[i][j].item = 1;
	        blockmtx[i][j].x1 = i * BLOCK_SIZE_W + (GAMEAREA_X0 + 2);
	        blockmtx[i][j].y1 = j * BLOCK_SIZE_H + (GAMEAREA_Y0 + 2);
	        blockmtx[i][j].x2 = blockmtx[i][j].x1 + BLOCK_SIZE_W  - 2 ;
	        blockmtx[i][j].y2 = blockmtx[i][j].y1 + BLOCK_SIZE_H  - 3 ;            
            blkCnt++;
        }
    }
    blkBrk = blkCnt;
}

// ブロックすべてを描画する
// ブロック崩しでは、ブロックが１つだけ表示される、ということはない（消えていくだけ）ので、
// 表示は無条件で全ブロックを表示させれば良いことになる。
void DrawBlock()
{
	static u16 colTbl[]  = {RED,  BLUE,     GREEN     ,MAGENTA,CYAN,     YELLOW};

	for (u8 i = 0 ; i< BLOCK_CNT_H;i++) {
		for (u8 j = 0; j<BLOCK_CNT_V;j++) {
			u16 col = colTbl[j % 6];
			if (blockmtx[i][j].item == 1) {
				LCD_Fill(blockmtx[i][j].x1,blockmtx[i][j].y1,blockmtx[i][j].x2,blockmtx[i][j].y2,col);
			}
		}
	}
	

}

//
// ブロック反射チェック
//　この時点では、BALLINFOの座標は移動済みの座標になっている。
//
void blockCheck(struct BALLINFO* bi)
{
	// 一つ前の座標
	int chkx = CVT_AXIS(bi->oldx);
	int chky = CVT_AXIS(bi->oldy);
	// 座標がある位置のブロック番号を求める
	int xNow = CVT_AXIS(bi->x);
	int yNow = CVT_AXIS(bi->y);
	int xidx = (xNow - (GAMEAREA_X0+2)) /BLOCK_SIZE_W;
	int yidx = (yNow - (GAMEAREA_Y0+2)) / BLOCK_SIZE_H;

	// 現在のボールの位置が、ブロックの中にある場合、衝突処理を行う
	if (blockmtx[xidx][yidx].item != 0 && GetOrthant(xNow,yNow, blockmtx[xidx][yidx].x1 ,blockmtx[xidx][yidx].y1,blockmtx[xidx][yidx].x2,blockmtx[xidx][yidx].y2) == 5) {
		blkBrk--;							// 残ブロック数を１つ減らす
		// ボールの新しい位置は、ブロックの内側なので、座標はひとつ前の位置に戻さないといけない。
		BallBack(bi);

		// 難易度調整
		if (blkBrk <= (blkCnt / 2)) {				// 残ブロックスが全ブロック数の半分以下になったら
			bi->SpeedMask  |= SPDMSK_BLOCKCNT_1;	// スピードレベル１
		} 
		if (blkBrk <= (blkCnt / 4)) {				// 残ブロックスが全ブロック数の1/4分以下になったら
			bi->SpeedMask |= SPDMSK_BLOCKCNT_2;		// スピードレベル２
		}

		// ボールを跳ね返す。
		u8 pos = GetOrthant(chkx,chky, blockmtx[xidx][yidx].x1 ,blockmtx[xidx][yidx].y1,blockmtx[xidx][yidx].x2,blockmtx[xidx][yidx].y2);
		if (pos == 2 || pos== 8) {
            BallSwapY(bi);
		} else if (pos == 4 || pos == 6) {
            BallSwapX(bi);
		} else {
            BallSwapX(bi);
            BallSwapY(bi);
		}

		// ブロックは消す
		blockmtx[xidx][yidx].item = 0;	 
		LCD_Fill(blockmtx[xidx][yidx].x1,blockmtx[xidx][yidx].y1,blockmtx[xidx][yidx].x2,blockmtx[xidx][yidx].y2,BLACK);

		// スコアの処理
		if (bi->SpeedMask & SPDMSK_BACKWALL) {		// 裏に入っていたらスコアは増量
			Score = Score + 2;
		} else {
			Score = Score + 1;
		}
		// ピポ音を出す
		BallSoundStart(0);

	}
}


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



// ボールの角度を示すデフォルト値と、角度の最大・最小値
#define BALL_DX_DEFAULT 1*2
#define BALL_DY_DEFAULT 2*2
#define BALL_DX_MIN  (1*2)
#define BALL_DX_MAX  (4*2)
#define BALL_DY_MIN  (1*2)
#define BALL_DY_MAX  (4*2)

//ボールの座標
struct BALLINFO ball[5];             // 最大5個のボールが出現するので配列にしておく

// ボールの情報にアクセスするための関数
struct BALLINFO* GetBallInfo(unsigned int idx)
{
    if (ball[idx].x == 0 && ball[idx].y == 0) return NULL;
    return &ball[idx];
}


u8 ballLive = 0;       // ボールの数




//ボールを1つ進める
void BallStep(struct BALLINFO *bi) 
{
	bi->oldx = bi->x;
	bi->oldy = bi->y;
	bi->x += bi->dx;
	bi->y += bi->dy;
}
//ボールを1つ前の位置に戻す
void BallBack(struct BALLINFO *bi) 
{
	bi->x = bi->oldx;
	bi->y = bi->oldy;
}
// ボールのdxを逆にする
void BallSwapX(struct BALLINFO *bi) 
{
    bi->dx = -bi->dx;
    bi->dxBase = -bi->dxBase;
}
// ボールのdyを逆にする
void BallSwapY(struct BALLINFO *bi) 
{
    bi->dy = -bi->dy;
    bi->dyBase = -bi->dyBase;
}
// ボールが失われた時の処理
void BallDead(struct BALLINFO *bi) 
{
	if (bi->x != 0) {
		ballLive--;
	}
	bi->oldx = 0;
	bi->oldy = 0;
	bi->x = 0;
	bi->y = 0;
}
// 生きているボールの数を返す
int GetBallCount()
{
	int ballCount = 0;
	for (u8 i = 0 ; i < 5;i++) {
		if (ball[i].x != 0 && ball[i].y != 0) {
			ballCount++;
		}
	}
	return ballCount;
}

// ボールの速度を調整する

void updateBallSpeed(struct BALLINFO *bi)
{
	int speedlvl = 1;
	/*
	if (bi->SpeedMask & SPDMSK_BACKWALL) speedlvl += 1; 
	if (bi->SpeedMask & SPDMSK_BLOCKCNT_1) speedlvl += 1;
	if (bi->SpeedMask & SPDMSK_BLOCKCNT_2) speedlvl += 1;
	*/
	bi->dx = bi->dxBase * speedlvl;
	bi->dy = bi->dyBase * speedlvl;
}


// ボールを初期化する
void InitBallPos(u8 mode, struct BALLINFO *bi)
{
	if (mode ==  0) {		// 完全初期化して最初のボールを生きにする。 ballIdxは使われない
		memset(ball,0,sizeof(ball));
		ball[0].x = (GAMEAREA_X0 + GAMEAREA_X1/2)*8;
		ball[0].y = (GAMEAREA_Y0 + (GAMEAREA_Y1-GAMEAREA_Y0)/2)*8;
		ball[0].dxBase = BALL_DX_DEFAULT;
		ball[0].dyBase = BALL_DY_DEFAULT;
		updateBallSpeed(&ball[0]);
		ballLive=1;	
	} else if (mode == 1)  { // ballIdxの位置を元にしてボールを１つ追加する。　ボールの速度・角度は元のボールと変える。
		for (int j = 1; j < 5;j++ ) {
			if (ball[j].x == 0) {			// このボールで行こう
				ball[j].x = bi->x;
				ball[j].y = bi->y;
				ball[j].dyBase = BALL_DY_DEFAULT;
				ball[j].dx = -bi->dx;
				ball[j].dxBase = -bi->dxBase;
				ball[j].SpeedMask = 0;
				updateBallSpeed(&ball[j]);
				ballLive++;
				break;
			}
		}
	}
}

//
// ボールを消す、または表示する
//　 true... 表示する、false ...消す
void drawDeleteBall(bool isDraw)
{
	for (u8 i = 0 ; i < 5;i++) {
		struct BALLINFO *bi = &ball[i];
		if(bi->x !=0) {
			u16 c;
			if (isDraw == FALSE) {
                c = BLACK;
			} else {
                c = WHITE;
			}
			LCD_Fill(CVT_AXIS(bi->x)-1 ,CVT_AXIS(bi->y)-1 ,CVT_AXIS(bi->x)+1,CVT_AXIS(bi->y)+1,c);

		}
	}
}


void CheckPaddle(struct BALLINFO *bi)
{
	int chkx = CVT_AXIS(bi->x);
	int chky = CVT_AXIS(bi->y);
	for (int j=0 ; j<2;j++) {
		struct PADDLEINFO* pi = GetPaddleInfo(j);
		if (pi == NULL) continue;
		u8 pos = GetOrthant(chkx,chky,pi->x1,pi->y1,pi->x2,pi->y2);
		if (pos == 5) {
			//ひとつ前のボール座標が、パドルのどこにあったかを求める。
			chkx = CVT_AXIS(bi->oldx);
			chky = CVT_AXIS(bi->oldy);
			u8 prevpos = GetOrthant(chkx,chky,pi->x1,pi->y1,pi->x2,pi->y2);
			//　ボールの新しい位置は、パドルの内側なので、ボールの座標をもとに戻さないといけない
			BallBack(bi);
			
			//　パドルにボールが反射する処理
			if (prevpos == 2 || prevpos == 8) {     // パドルの長径に当たった場合、y座標を反転
				bi->dyBase = -bi->dyBase;
				u8 pdlcx = pi->x1  + pi->Width/2;
				u8 ballpdldif = abs(chkx - pdlcx);
				
				if (ballpdldif > ( pi->Width/2) / 3) {	// 端だったら角度を増やす
					int cx = pi->x1 + pi->Width / 2;	// パドルの中央位置
					int ballX = CVT_AXIS(bi->x);		// ボールの位置


					if (bi->dxBase > 0) {					// ボールは右に移動中
						bi->dxBase += (ballX > cx) ? 1 : -1;
					} else {							// ボールは左に移動中
						bi->dxBase += (ballX > cx) ? -1 : 1;
					}

					if (abs(bi->dxBase) < BALL_DX_MIN) {
						bi->dxBase = bi->dxBase > 0 ? BALL_DX_MIN : -BALL_DX_MIN;
					} else if (abs(bi->dxBase) > BALL_DX_MAX) {
						bi->dxBase = bi->dxBase > 0 ? BALL_DX_MAX : -BALL_DX_MAX;
					}
				} 
			} else if (prevpos == 4 || prevpos == 6) {		// パドルの横に当たった時
				//　x反転
				bi->dxBase = -bi->dxBase;

			} else  {										// それ以外。パドルの角に当たった時
				bi->dyBase = -bi->dyBase;
				bi->dxBase = -bi->dxBase;						
			} 
			// ポピ音を出す
			BallSoundStart(1);
		}
	}
}

//  ボールを動かす。
// 0... ミス
// 1... 継続
// 2... クリア
unsigned char moveBall()
{
	
	for (u8 i = 0 ; i < 5;i++) {
		struct BALLINFO *bi = &ball[i];		
		if (bi->x == 0) continue;

		//ボールを動かす
        BallStep(bi);
		// 壁反射チェック
		{
			u8 ret = checkWall(bi);
			if (ret == 0) {
				return 0;
			} else if (ret == 2) {
				continue;
			}
		}


		//ブロック反射チェック
		// ボールの進行方向の隅がブロックに接しているかを調べる
		blockCheck(bi);
		if (blkBrk == 0) {
			return 2;
		}
		CheckPaddle(bi);
    	updateBallSpeed(bi);
    }
	return 1;
}

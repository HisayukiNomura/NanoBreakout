#ifndef __ball_h__
#define __ball_h__
// ボールの速度を変える条件を満たしたかのフラグ
#define SPDMSK_BACKWALL 0x01        // ボールが奥の壁にヒットしたら速度が上がる
#define SPDMSK_BLOCKCNT_1 0x02      // ブロック数が半分になったら速度が上がる
#define SPDMSK_BLOCKCNT_2 0x04      // ブロック数が1/4になったら速度が上がる

struct BALLINFO {
	int oldx;	    // 8倍された、ボールの直前のX座標
	int oldy;	    // 8倍された、ボールの直前のY座標
	int x;		    // 8倍された、ボールのX座標
	int y;		    // 8倍された、ボールのY座標
	int dx;         // 8倍された、ボールの現在のX増分
	int dy;         // 8倍された、ボールの現在のY増分
	int dxBase;     // 8倍された、ボールのX増分初期値
	int dyBase;     // 8倍された、ボールのY増分初期値
	unsigned char SpeedMask;   // ボールの速度レベルを変える条件を満たしたかのフラグ
};

// 座標を1/8する。
#define CVT_AXIS(__x) ((__x) >> 3)
struct BALLINFO* GetBallInfo(unsigned int idx);
void BallStep(struct BALLINFO *bi);
void BallBack(struct BALLINFO *bi);
void BallSwapX(struct BALLINFO *bi);
void BallSwapY(struct BALLINFO *bi);
void BallDead(struct BALLINFO *bi) ;
void InitBallPos(u8 mode, struct BALLINFO *bi);
void drawDeleteBall(bool isDraw);
int GetBallCount();
unsigned char  moveBall();
#endif
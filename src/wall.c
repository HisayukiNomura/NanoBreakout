#include "lcd/lcd.h"
#include "led.h"
#include "memory.h"
#include "gd32vf103.h"
#include "game.h"
#include "ball.h"


//　壁反射のチェック
// 0 .. ミス。すべてのボールが亡くなった
// 1 .. ゲーム継続
// 2 .. そのボールは失われたがまだボールが残っている
u8 checkWall(struct BALLINFO* bi)
{
	int x = CVT_AXIS(bi->x);
	int y = CVT_AXIS(bi->y);

	// 横の壁のチェック。ボールは3x3ドットなので、衝突判定はその範囲を考慮して、1ドット分広くチェックする。
	if ((x-1) <= GAMEAREA_X0 || (x+1) >=GAMEAREA_X1) {
		// ひとつ前の場所に戻して
		BallBack(bi);
		// X方向を逆にする
        BallSwapX(bi);
	} 
	if ((y-1) <=GAMEAREA_Y0 || (y+1) >=GAMEAREA_Y1) {
		// ひとつ前の場所に戻して
		BallBack(bi);
		// Y方向を逆にする。
        BallSwapY(bi);
		if ((y-1) <= GAMEAREA_Y0) {
			bi->SpeedMask |= SPDMSK_BACKWALL;
		} else {
			BallDead(bi);
			if (GetBallCount() == 0) {	// すべてのボールがなくなった
				return 0;
			} else {					// まだボールは残っている
				return 2;
			}
		}
	}
	return 1;
}

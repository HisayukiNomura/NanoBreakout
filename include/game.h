#ifndef __GAME_H__
#define __GAME_H__
extern void Game(bool);

// ゲームの表示領域
#define GAMEAREA_X0   0
#define GAMEAREA_Y0   20
#define GAMEAREA_X1   79
#define GAMEAREA_Y1  148

enum GAMESTATE {
	STATE_INIT,			// 初期化処理中
	STATE_IDLE,
	STATE_STARTGAME,	// ゲーム開始（初期化中）
	STATE_INGAME,		// ゲーム中
	STATE_NEXTSTAGE,	// 次のステージに進む
	STATE_BALLLOSS,		// ボールのロス
	STATE_GAMEOVER,		// ゲームオーバー処理中
};

extern int LifeCnt;
extern int Score;

unsigned char GetOrthant(int x , int y , int x1, int y1 , int x2 , int y2);
#endif
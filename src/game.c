#include "lcd/lcd.h"
#include "led.h"
#include "memory.h"
#include "gd32vf103.h"
#include "game.h"
#include "ball.h"
#include "block.h"
#include "paddle.h"
#include "button.h"
#include "sound.h"

enum GAMESTATE gameState;	    //　ゲームの状態
volatile u8 WakeFlag = 0;       // このフラグが１になると、処理が開始される

int LifeCnt = 0;
int Score = 0;
int Stage= 1;                      // ステージ

//
// 割り込みハンドラ。タイマーにより指定した周期で非同期に呼び出される
//
void TIMER5_IRQHandler(void)
{
    if(SET == timer_interrupt_flag_get(TIMER5, TIMER_INT_FLAG_UP)){
        timer_interrupt_flag_clear(TIMER5, TIMER_INT_FLAG_UP);
    	WakeFlag = 1;
	    
        static u8 oeFlag;
        if (oeFlag == 0) {
            gpio_bit_reset(GPIOB, GPIO_PIN_8); //OE#
            oeFlag = 1;
        } else {
            gpio_bit_set(GPIOB, GPIO_PIN_8); //OE#
            oeFlag = 0;
        }
    }
}

//
// タイマーの初期化
//
void timer5_config(int Cnt)
{
    // タイマーのパラメータを設定する。
	// タイマーの16ビットプリスケーラには54MHzが入力される・・・はずだが、108MHzが入力されるように振舞っている。
	// それを、10000分周(timer_initpara.prescaler = 10000-1)すると、おおよそ108,000,000/10000= 10.8Khz
    // それを、引数の cnt回数えて(timer_initpara.period = Cnt;)タイマーの周期を決める。
	// 例えば、cntが30の時は、10,800 / 30 =360で、360Hzとなる。
    timer_parameter_struct timer_initpara;
    rcu_periph_clock_enable(RCU_TIMER5);
    timer_deinit(TIMER5);
    timer_struct_para_init(&timer_initpara);
    timer_initpara.prescaler         = 10000 - 1;
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = Cnt;
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;
    timer_init(TIMER5, &timer_initpara);
    timer_auto_reload_shadow_enable(TIMER5);
    timer_interrupt_enable(TIMER5, TIMER_INT_UP);

    // 割り込みを有効にして、タイマー５を設定する
    eclic_global_interrupt_enable();
	eclic_set_nlbits(ECLIC_GROUP_LEVEL3_PRIO1);
	eclic_irq_enable(TIMER5_IRQn,1,0);
    
    // タイマーを開始する
    timer_enable(TIMER5);
}



//　外枠とスコア、ライフ残を表示する
void DrawBORDER()
{
	LCD_DrawLine(GAMEAREA_X0,GAMEAREA_Y1,GAMEAREA_X0,GAMEAREA_Y0,WHITE);
	LCD_DrawLine(GAMEAREA_X0,GAMEAREA_Y0,GAMEAREA_X1,GAMEAREA_Y0,WHITE);
	LCD_DrawLine(GAMEAREA_X1,GAMEAREA_Y0,GAMEAREA_X1,GAMEAREA_Y1,WHITE);

	LCD_ShowString(0,0,(const u8 *)"SCORE:",WHITE);
	LCD_ShowString(10,160-12,(const u8 *)"LIFE:",WHITE);
	char life[3];
	sprintf(life,"%1d",LifeCnt);
	LCD_ShowString(55,160-12,(u8 *)life,WHITE);
	u8 scr[12];
	sprintf((char *)scr,"%5d0",Score);
	LCD_ShowString(38,0,scr,WHITE);	
}


// 座標が、矩形の外側に対して、どの象限にいるのかを返す関数
//    1         2        3
//        +---------+
//   4    |    5    |    6
//        +---------+
//   7         8         9    
unsigned char GetOrthant(int x , int y , int x1, int y1 , int x2 , int y2)
{
	bool bLowerX1 =  (x < x1);
	bool bUpperX2 =  (x > x2);
	bool bLowerY1 =  (y < y1);
	bool bUpperY2 =  (y > y2);

    
	if (bLowerX1) {
        return bLowerY1 ? 1: (bUpperY2 ? 7:4);
	} else if (bUpperX2) {
        return bLowerY1 ? 3: (bUpperY2 ? 9:6);
	} else {
        return bLowerY1 ? 2: (bUpperY2 ? 8:5);
	}
}


//
// メイン処理
//
void Game(bool isDemo) 
{
    rcu_periph_clock_enable(RCU_GPIOB);
    gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_8);	// B8をデバッグに使う

    // 初期化処理
    gameState = STATE_INIT;             // ステータスを初期化にする
    timer5_config(100);                  // タイマーの初期化を行う
    LCD_Init();
    LCD_Clear(BLACK);
	Adc_init();
    sound_pwm_init();

    BallSoundInit();                    // ボール音の初期化


    u16 tick = 0;                       // LEDを点滅させるためのカウンターを初期化
    gameState = STATE_IDLE;             //初期化処理が終了したのでゲーム開始処理を行う
    static u16 heartBeat = 0;            // 状態遷移ループのカウンタ
    
    while (TRUE) { 
		timer_enable(TIMER5);               // タイマーを有効にする
        // タイマーのウェイト処理。wakeFlagが割り込みルーチン内で１になるまで無限ループする
		
        while(WakeFlag == 0) {
            delay_1ms(10);
            break;
        }

        tick = (tick + 1) & 0x8FFF;     // LEDの点滅用カウンタのインクリメント
		WakeFlag = 0;                   // タイマーのウエイトフラグを初期化する

        heartBeat = (heartBeat+1) & 0x7FFF;
        
        BallSoundTick();                // サウンド処理を実行
        switch (gameState) {
            case STATE_IDLE:{
                u16 cnt = heartBeat & 0xFF;
                if (cnt == 0x00) {
                    LCD_ShowString(5,40,(const unsigned char *)"PUSH BUTTON",WHITE);
                    LCD_ShowString(5,60,(const unsigned char *)" TO START  ",WHITE);
                } else if (cnt == 0x80) {
                    LCD_ShowString(5,40,(const unsigned char *)"PUSH BUTTON",RED);
                    LCD_ShowString(5,60,(const unsigned char *)" TO START  ",RED);
                }
                if (CheckP1Button()) {
                    LCD_Clear(BLACK);
                    Score = 0;
                    LifeCnt = 3;
                    Stage = 1;
                    InitBlock(Stage);
                    gameState = STATE_STARTGAME;
                }
                break;
            }
            case STATE_STARTGAME:{
                /*ゲームの開始処理 */

                DrawBORDER();               //外枠とライフ残、スコアを画面に表示させる
                DrawBlock();
                InitBallPos(0,NULL);
				InitPaddle();
                gameState = STATE_INGAME;
                break;
            }
            case STATE_INGAME:{
        		drawDeleteBall(FALSE);			// ひとつ前のボールを消す
	            u8 ret= moveBall();
	            if (ret == 0) {                 // すべてのボールがなくなったら
                    LifeCnt--;
                    gameState = STATE_BALLLOSS; // ボールロスの状態に遷移させる
                    break;
                } else if (ret == 2) {          // ブロックがすべてなくなったら
                    gameState = STATE_NEXTSTAGE;
                    break;
                }
        		drawDeleteBall(TRUE);			// ひとつ前のボールを消す
				breakout_PaddleCtrl(isDemo);			// パドルを動かす
                break;                
            }
            case STATE_NEXTSTAGE:{
                static u16 waitNextCnt = 0;

                if (waitNextCnt == 0) {
                    // 画面に"WELL DONE!"と表示させる
                    LCD_ShowString(5,80,(const unsigned char *)"WELL DONE!",WHITE);
                    // 一定時間が経過するか、ボタンが押されたらブロックを初期化してゲームの再開に遷移する
				    SoundPlay("E3200 E3200 F3200 G3200 G3200 F3200 E3200 D3200 C3200 C3200 D3200 E3200 D3300 C3100 C3400",10);	
                }
                waitNextCnt++;
                if (waitNextCnt == 0x10 || CheckP1Button()) {
                    LCD_Clear(BLACK);
                    waitNextCnt = 0;
                    Stage++;
                    InitBlock(Stage);                    
                    gameState = STATE_STARTGAME;
                }
               break;                
            }
            case STATE_BALLLOSS:{               // ボールロス
                static u16 waitCnt = 0;

                // ライフがないならゲームオーバーに遷移
                if (LifeCnt == 0) {
                    gameState = STATE_GAMEOVER;
                    break;
                }
                // 画面に"-- MISS! --"と表示させる
                if (waitCnt == 0) {
                    LCD_ShowString(5,80,(const unsigned char *)"-- MISS! --",WHITE);
                    SoundSet(800);
                    StartSound();
                }
                // 一定時間が経過するか、ボタンが押されたらゲームの再開に遷移する
                waitCnt++;
                if (waitCnt == 0x100 || CheckP1Button()) {
                    LCD_Clear(BLACK);
                    waitCnt = 0;
                    StopSound();
                    gameState = STATE_STARTGAME;
                }
               break;
            }
            case STATE_GAMEOVER:{
                /*ゲーム－オーバー処理*/
                static u16 waitGameOverCnt = 0;
                if (waitGameOverCnt == 0) {
                    // 画面に"-- MISS! --"と表示させる
                    LCD_ShowString(5,80,(const unsigned char *)"-GAMEOVER-",WHITE);
                    // 一定時間が経過するか、ボタンが押されたらゲームの再開に遷移する
				    SoundPlay("B1400 B1300 B1100 B1400 D2300 c2100 c2300B1100 B1300 a1100 B1400",10);						
                }
                waitGameOverCnt++;
                if (waitGameOverCnt == 0x5 || CheckP1Button()) {
                    LCD_Clear(BLACK);
                    waitGameOverCnt = 0;
                    gameState = STATE_IDLE;
                }                
                break;
            }
        }

    }
}
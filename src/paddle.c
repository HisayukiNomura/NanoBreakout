#include "lcd/lcd.h"
#include "led.h"
#include "memory.h"
#include "gd32vf103.h"
#include "game.h"
#include "ball.h"
#include "block.h"
#include "paddle.h"



// ボリューム入力の初期化
void Adc_init(void) 
{
	// 	GPIOポートA　を、アナログ入力モード、５０MHｚ、ピン０とピン１
    gpio_init(GPIOA, GPIO_MODE_AIN, GPIO_OSPEED_50MHZ, GPIO_PIN_0|GPIO_PIN_1);
	// ADCのクロックプリスケーラを APB2/12 に設定する
	rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV12);

	// ADC0にクロックを供給する
    rcu_periph_clock_enable(RCU_ADC0);

	// ADCをクリアする
	adc_deinit(ADC0);
	// ADCをフリーモード（全ADCを独立して動作させる）にする。(ADC_CTL0のSYNCMを0b000)
	adc_mode_config(ADC_MODE_FREE);
	// ADCのデータを右詰めにする
	adc_data_alignment_config(ADC0,  ADC_DATAALIGN_RIGHT);
	// チャンネルグループのデータ長を１に設定する。（単発）
	adc_channel_length_config(ADC0,  ADC_REGULAR_CHANNEL,1);
	// ADCの外部トリガーソースを無効にする
 	adc_external_trigger_source_config(ADC0, ADC_REGULAR_CHANNEL, ADC0_1_EXTTRIG_INSERTED_NONE);	
	// ADCの外部トリガーのレギュラーチャンネルを有効にする
	adc_external_trigger_config(ADC0, ADC_REGULAR_CHANNEL, ENABLE);
	// ADCを有効にする
	adc_enable(ADC0);
    delay_1ms(1);
	// キャリブレーションを行う。この関数は、中でキャリブレーションの完了を待つので、呼び出すだけでよい
    adc_calibration_enable(ADC0);

}


//指定されたチャンネルからデータを読みだす
uint16_t Get_adc(int ch)
{
	// 読みだすチャンネルを指定する
	adc_regular_channel_config(ADC0, 0 , (uint8_t)ch, ADC_SAMPLETIME_7POINT5);
	// 読み出しを開始する
	ADC_CTL1(ADC0)|=ADC_CTL1_ADCON;
	// データが出そろうのを待つ
	while(adc_flag_get(ADC0 , ADC_FLAG_EOC) == RESET);
	// データを読みだす
	int ret = adc_regular_data_read(ADC0);
	// フラグをクリアする
	adc_flag_clear(ADC0,ADC_FLAG_EOC); 
    return ret;

}




// ADCの値を読み出し、パドルの位置に変更する。
#define MAX_PADDLE_SPEED  100
unsigned int getPaddlePos()
{
	static unsigned int actualValue = -1;

	// 0～4096の値を、4で割って0～1024くらいにしておく。端っこのほうが怪しいから。
	short CurrentPos = Get_adc(0);
	CurrentPos = CurrentPos >> 2;
	if (actualValue == -1) {
		actualValue = CurrentPos;
	 } else {
		int dif = abs(actualValue - CurrentPos);
		if (dif < MAX_PADDLE_SPEED) {
			actualValue = CurrentPos;
		} else {
			if (actualValue > CurrentPos) {
				actualValue-= MAX_PADDLE_SPEED;
			} else {
				actualValue+= MAX_PADDLE_SPEED;
			} 
		}
	 }
	 return actualValue;
}

// パドルの情報
struct PADDLEINFO Paddle[2];

void InitPaddle(void)
{
    memset(Paddle,0,sizeof(Paddle));

	Paddle[0].Width = 20;
	Paddle[0].x1 = (GAMEAREA_X1 /2) - Paddle[0].Width /2 ;
	Paddle[0].y1 = GAMEAREA_Y1  - 15;
	Paddle[0].x2 = (GAMEAREA_X1 /2) + Paddle[0].Width /2 ;
	Paddle[0].y2 = GAMEAREA_Y1  - 10;

}

// 外部からパドルの位置などの情報にアクセスさせるための関数
struct PADDLEINFO* GetPaddleInfo(int idx)
{
    if (Paddle[idx].y1 == 0) return NULL;
    return &Paddle[idx];
}

// パドルの描画
void drawPaddle()
{
	for (u8 i = 0 ; i < 2;i++) {
		struct PADDLEINFO* pi = &Paddle[i];
		if (pi->y1 == 0) continue;
		LCD_Fill(pi->x1,pi->y1,pi->x2,pi->y2, WHITE);
		if (pi->prevx1 >= (GAMEAREA_X0+1) && pi->x1 > pi->prevx1) {
			LCD_Fill(pi->prevx1,pi->y1,pi->x1,pi->y2,BLACK);		
		} else if (pi->x1 < pi->prevx1) {
			LCD_Fill(pi->x2,pi->y1,pi->prevx2,pi->y2,BLACK);
		} 
	}
}

// パドルの中心座標（X）を指定し、その位置にパドルを移動させる
void setPaddle(int pdlId , int cx) {

	Paddle[pdlId].prevx1 = Paddle[pdlId].x1;
	Paddle[pdlId].prevx2 = Paddle[pdlId].x2;

	u8 cxMin = GAMEAREA_X0 + (Paddle[pdlId].Width  / 2)+1;
	u8 cxMax = GAMEAREA_X1 - (Paddle[pdlId].Width  / 2)-1;
	if (cx >= cxMax) cx = cxMax;
	if (cx <= cxMin) cx = cxMin;
	
	Paddle[pdlId].x1 = cx - Paddle[pdlId].Width  / 2;
	Paddle[pdlId].x2 = cx + Paddle[pdlId].Width  / 2;


    // ボールが壁とパドルに挟まれないように、ボールの位置によってパドルの動きを制限する
	for (u8 i = 0 ; i < 5;i++) {
		struct BALLINFO *bi = GetBallInfo(i);
		if (bi == NULL) continue;

        // パドルのy座標とボールのｙ座標を比べ、横並びになっていたら
		if ( Paddle[pdlId].y1 <= CVT_AXIS(bi->y) && CVT_AXIS(bi->y) <= Paddle[pdlId].y2) {
			// ボールがパドルと横並び
			if (CVT_AXIS(bi->x) <=4 && Paddle[pdlId].x1 <=4) {			//ボールが左端４ドット内にいる場合
	        	Paddle[pdlId].x1 = 4;
		        Paddle[pdlId].x2 = Paddle[pdlId].x1 + Paddle[pdlId].Width;
			} else if (CVT_AXIS(bi->x) >= (GAMEAREA_X1 - 4) && Paddle[pdlId].x2 >= (GAMEAREA_X1 - 4)) {
		    	Paddle[pdlId].x2 =  (GAMEAREA_X1 - 4);
    			Paddle[pdlId].x1 = Paddle[pdlId].x2 - Paddle[pdlId].Width;
			}
		}
	}
}



// パドルを動かす
void breakout_PaddleCtrl(bool isDemo)
{
    u16 padPos = getPaddlePos();

	for (u8 i = 0 ; i < 2;i++) {
		struct PADDLEINFO* pi = &Paddle[i];
		if (pi->y1 == 0) continue;

        if (isDemo) {
            //追いかけるべきボールを探す
            // ボールの実質的な距離をベースにする
            int distMax = INT16_MAX;
            int ballIdx = -1;
            for (int j =0;j <5; j++) {
			    struct BALLINFO* bi = GetBallInfo(j);
			    if (bi == NULL) continue;
                int dist;
                if (bi->dy < 0) {       // ボールが遠ざかっているなら
                    dist = CVT_AXIS(bi->y) + pi->y1;

                }  else {               // ボールが近づいているなら
                    dist =  CVT_AXIS(pi->y1) - bi->y;
                }
                if (dist < distMax) {
                    ballIdx = j;
                    distMax = dist;
                }
            }
            struct BALLINFO* bi = GetBallInfo(ballIdx);
            u8 cx = pi->x1 + (pi->Width  / 2);
    	    if (CVT_AXIS(bi->x) > cx) {
    			setPaddle(i,cx+1);
			} else if (CVT_AXIS(bi->x) < cx) {
    			setPaddle(i,cx-1);
	    	}
        } else {
		    u8 newpadx = (GAMEAREA_X1 - GAMEAREA_X0) * padPos / 1024;
		    setPaddle(i,newpadx);
        }
		
	}

	drawPaddle();	
}

#include "lcd/lcd.h"
#include "led.h"
#include "memory.h"
#include "gd32vf103.h"
#include "sound.h"

void sound_pwm_init()
{
  	timer_oc_parameter_struct timer_ocinitpara; 
	timer_parameter_struct timer_initpara;           // タイマーパラメータ構造体の宣言
	timer_deinit(TIMER4);
    timer_struct_para_init(&timer_initpara);


   	// rcu_periph_clock_enable(RCU_AF);
    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_3);
    rcu_periph_clock_enable(RCU_TIMER4);

    timer_initpara.prescaler         = 539;                 // タイマーの16ビットプリスケーラには54MHzが入力されるので、54Mhz/540・・・
                                                            // のはずだが、108MHzが入力されるように振舞っている。
                                                            // なので108MHz/540 で、およそ200KHzが得られるハズ。
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;  // count alignment edge = 0,1,2,3,0,1,2,3... center align = 0,1,2,3,2,1,0
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;    // Counter direction
    timer_initpara.period            = 3;		            // 200KHz/4 で、おおよそ50KHzが得られるハズ。ただ、これはSoundSetで動的に書き換えられちゃうけどね。
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;    // This is used by deadtime, and digital filtering (not used here though)
    timer_initpara.repetitioncounter = 0;                   // Runs continiously	
    timer_init(TIMER4, &timer_initpara);  					// Apply settings to timer   

 	timer_channel_output_struct_para_init(&timer_ocinitpara);
    timer_ocinitpara.outputstate  = TIMER_CCX_ENABLE;        // Channel enable
    timer_ocinitpara.outputnstate = TIMER_CCXN_DISABLE;      // Disable complementary channel
    timer_ocinitpara.ocpolarity   = TIMER_OC_POLARITY_HIGH;  // Active state is high
    timer_ocinitpara.ocnpolarity  = TIMER_OCN_POLARITY_HIGH;  
    timer_ocinitpara.ocidlestate  = TIMER_OC_IDLE_STATE_LOW; // Idle state is low
    timer_ocinitpara.ocnidlestate = TIMER_OCN_IDLE_STATE_LOW;
    timer_channel_output_config(TIMER4, TIMER_CH_3,&timer_ocinitpara);   // Apply settings to channel

    timer_channel_output_pulse_value_config(TIMER4,TIMER_CH_3,0);                   // Set pulse width
    timer_channel_output_mode_config(TIMER4,TIMER_CH_3,TIMER_OC_MODE_PWM0);         // Set pwm-mode
    timer_channel_output_shadow_config(TIMER4,TIMER_CH_3,TIMER_OC_SHADOW_DISABLE);

    /* auto-reload preload enable */
    timer_auto_reload_shadow_enable(TIMER4);

    /* start the timer */
    timer_enable(TIMER4);
    
}

bool 	isSound = TRUE;
void StartSound(void) {
    if (isSound == FALSE) return;
    timer_enable(TIMER4);		
}
void StopSound(void) {
    if (isSound == FALSE) return;
    timer_disable(TIMER4);		
}
void SoundSet(int Period)
{
    TIMER_CH3CV(TIMER4) = Period/2;	   
    TIMER_CAR(TIMER4) =  Period;	 	
}


static bool bSound[2];          // ボールの音が出てるかのフラグ。[0]がポピ、[1]がピポ。
static int iSoundIdx=0;         // ボールの音がどこまで進んでいるか。
                                // 例えば「ポピ」を出しているなら、0でポ、1がピ。
static int iSoundStep[2][2];    // 今出している音の進行状況。例えば、「ポピ」の時であれば、
                                // [0][0]が「ポ」の間増やされていき、一定の値になったら
                                //「ピ」に変えて[0][1]の値を増やしていく
static int iSoundTim[2][2] = {{400,800},{1000,600}};		//200Hzに対して、これで分週する。400なら、500Hzになる。


void BallSoundInit(void)
{
    // 変数を初期化して
	for (u8 i = 0;i<2;i++) {
		iSoundIdx = 0;
		bSound[i] = FALSE;
		iSoundStep[i][0] = iSoundStep[i][1] = 0;
	}
	timer_disable(TIMER4);		//音を止める
}
// 音を出し始める。引数に音のインデックス「ピポ」「ポピ」を指定
void BallSoundStart(int sndNo)
{
	BallSoundStop();
	bSound[sndNo] = TRUE;

}
// 今出ている音をすべて止める。初期化と同じ処理でよい
void BallSoundStop(void)
{
    BallSoundInit();
}


// 音を出す主処理。タイマーで一定間隔に呼び出される必要がある
void BallSoundTick(void)
{
	for (u8 i = 0 ; i < 2;i++){
		if (bSound[i]) {
			if (iSoundStep[i][iSoundIdx] == 0) {		// 音のなり始め
				SoundSet(iSoundTim[i][iSoundIdx]);
				timer_enable(TIMER4);
				iSoundStep[i][iSoundIdx]++;					
			} else if (iSoundStep[i][iSoundIdx]<3) {
				iSoundStep[i][iSoundIdx]++;
			} else if (iSoundStep[i][iSoundIdx] == 3) {	// 次の音へ
				if (iSoundIdx == 1) {			//　全部の音を出し終わったら終了
					BallSoundStop();
				} else {
					iSoundStep[i][iSoundIdx] = INT16_MAX;
					iSoundIdx++;
				}
			}
		}
	}

}

// 指定した時間、ビープ音を鳴らす
void Beep(int ms)
{
    SoundSet(800);
    StartSound();
    delay_1ms(ms);
    StopSound();
}

//渡された文字列に基づいてメロディーを演奏する
//
const int FreqTbl[]={3058,2886,2724,2571,2427,2291,2162,2041,1926,1818,1716,1620};
const char* sndChar =  "CcDdEFfGgAaB";
#define TOVAL(__C__) ((__C__) - '0')

void SoundPlay(char *Snd,int waitCnt)
{

    char* pWork = Snd;
    while (*pWork != 0) {
        char S1 = *pWork++;
        if (S1 == ' ') {
            delay_1ms(waitCnt);
            continue;
        }
        char S2 = *pWork++;
        int idx = strchr(sndChar,(int)S1) - sndChar;
        int Freq = FreqTbl[idx] / (1 << (int)TOVAL(S2));
        int len = TOVAL(*pWork)*100 + TOVAL(*(pWork+1)) *10 +  TOVAL(*(pWork+2));
        pWork+=3;
        SoundSet(Freq);
        StartSound();
        delay_1ms(len);
        StopSound();
    }
}
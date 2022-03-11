#include "lcd/lcd.h"
#include "led.h"
#include "memory.h"



// ボタンが押され続けた
static const uint32_t PortTBL[] = {GPIO_PIN_6,GPIO_PIN_7};
bool IsButtonPushed(int PlayerNo)
{
	static u8 Counter[2];

	FlagStatus  fr = gpio_input_bit_get (GPIOA,PortTBL[PlayerNo]);
	if (!fr) {
		if (Counter[PlayerNo] == 0xFF) Counter[PlayerNo]=0xFE;
		Counter[PlayerNo]++;
	} else {
		Counter[PlayerNo]=0;
	}
	if (Counter[PlayerNo] >= 50) {
		return TRUE;
	} 
	return FALSE;
}
// ボタンが離され続けた
bool IsButtonReleased(int PlayerNo)
{
	static u8 Counter[2];
	FlagStatus  fr = gpio_input_bit_get (GPIOA,PortTBL[PlayerNo]);
	if (fr) {
		if (Counter[PlayerNo] == 0xFF) Counter[PlayerNo]=0xFE;
		Counter[PlayerNo]++;
	} else {
		Counter[PlayerNo]=0;
	}
	if (Counter[PlayerNo] >= 50) {
		return TRUE;
	} 
	return FALSE;
}


bool p1ButtonPushed = FALSE;
bool p2ButtonPushed = FALSE;


// 一定時間間隔で呼び出され、ボタンが押されたかを監視する。
// 一般的なボタンは、押したときに反応するのではなく、離した時に反応する。
// そのため、この処理では、ボタンの状態を示す p1ButtonPushed グローバル変数、関数の戻り値とも、
// ボタンが押された時ではなく、離されたときにtrueとなる


bool CheckP1Button()
{
	p1ButtonPushed = RESET;			// ボタンの変化は一回だけトリガーされれば良い
	static bool  isButton1 = FALSE;
	if (isButton1 == FALSE) {
		if (IsButtonPushed(0)) { //ボタンがOFF→ONに変化
			isButton1 = TRUE; 								// ボタンが押された
		} 
	}  else {
		if (IsButtonReleased(0)) {//ボタンがON→OFFに変化
			isButton1 = FALSE;
			p1ButtonPushed = TRUE;
            return TRUE;
		}
	}
    return FALSE;
}

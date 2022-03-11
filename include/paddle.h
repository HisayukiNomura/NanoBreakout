#ifndef __paddle_h__
#define __paddle_h__

//パドルの位置に関する情報
struct PADDLEINFO {
	unsigned char x1;
	unsigned char  y1;
	unsigned char  x2;
	unsigned char  y2;
	unsigned char  prevx1;
	unsigned char  prevx2;
	unsigned char  Width;
};
struct PADDLEINFO* GetPaddleInfo(int idx);

void Adc_init(void) ;
uint16_t Get_adc(int ch);
unsigned int getPaddlePos();
void breakout_PaddleCtrl(bool);
void InitPaddle(void);
#endif
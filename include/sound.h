#ifndef __sound_h__
#define __sound_h__

void sound_pwm_init();
void StartSound(void);
void StopSound(void);
void SoundSet(int Period);

void BallSoundInit(void);
void BallSoundStart(int sndNo);
void BallSoundStop(void);
void BallSoundTick(void);
void Beep(int ms);
void SoundPlay(char *Snd,int waitCnt);
#endif
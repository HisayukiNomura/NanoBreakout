#ifndef __button_h__
#define __button_h__

bool IsButtonReleased(int PlayerNo);
bool IsButtonPushed(int PlayerNo);

extern bool CheckP1Button();
extern bool p1ButtonPushed;
extern bool p2ButtonPushed;

#endif
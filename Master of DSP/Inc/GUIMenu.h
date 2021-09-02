#ifndef APPLICATION_USER_GUIMenu_H_
#define APPLICATION_USER_GUIMenu_H_

#include "main.h"

void GUI_Menu(void);
void BSP_Pointer_Update(void);
void setupMenu(void);
void checkButtonPress(void);
extern uint8_t displayOption;
extern int8_t channelDisplayOption;
extern int8_t filterDisplayOption;
#endif /* APPLICATION_USER_GUIMenu_H_ */

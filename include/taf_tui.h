#ifndef TAF_TUI_H
#define TAF_TUI_H

#include "test_logs.h"

// UI initialization
int taf_tui_init();

// UI deinitialization 
void taf_tui_deinit();

// UI update function; should be called every time pannel should be updated
void taf_tui_update();




#endif // TAF_TUI_H

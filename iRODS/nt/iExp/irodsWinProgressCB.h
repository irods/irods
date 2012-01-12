#ifndef _irodsWinProgressCB_h_
#define _irodsWinProgressCB_h_

#include "irodsGuiProgressCallback.h"

void irodsWinMainFrameProgressMsg(char *msg, float curpt);
void irodsWinMainFrameProgressCB(operProgress_t *operProgress);

#endif
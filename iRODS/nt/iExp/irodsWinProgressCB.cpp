#include "stdafx.h"
#include "Inquisitor.h"
#include "MainFrm.h"
#include "irodsGuiProgressCallback.h"

//void irodsWinProgressMsg(char *msg, float curpt)
void irodsWinMainFrameProgressMsg(char *msg, float curpt)
{
	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	if((msg != NULL) && (strlen(msg) > 0))
		frame->SetStatusMessage(msg);
	if(curpt < 0.0)
	{
		frame->ProgressBarStepIt();
	}
	else
	{
		frame->SetProgressBarPos(curpt);
	}
}

void irodsWinMainFrameProgressCB(operProgress_t *operProgress)
{
	if(operProgress == NULL)
		return;

	CMainFrame* frame = (CMainFrame*)AfxGetMainWnd();
	
	float pct = 0.1;
	char msg[MAX_NAME_LEN+200];
	float ft;
	
	if(operProgress->flag == 0)
	{
		if(operProgress->totalNumFilesDone > 1)
		{
			frame->ProgressBarMove2Full();
		}

		if(strlen(operProgress->curFileName) > 0)
		{
			frame->SetStatusMessage(operProgress->curFileName);
		}
		//frame->ProgressBarStepIt();
		frame->SetProgressBarPos(0.30);
	}
	else if((operProgress->flag == 1)&&(strlen(operProgress->curFileName) > 0))
	{
		frame->ProgressBarStepIt();

		/*
		if(operProgress->curFileSize > 0)
		{
			pct = ((float)operProgress->curFileSizeDone)/((float)operProgress->curFileSize);
		}
		else
		{
			pct = 0.0;
		}
		*/
		ft = ((float)operProgress->curFileSize)/1048576.0;
		int tt = (int)(ft/1024.0);
		if(tt > 0)
		{
			sprintf(msg, "%s (%.2f GB, %d%% done)", operProgress->curFileName, (ft/1204.0),(int)(pct*100.0));
		}
		else
		{
			sprintf(msg, "%s (%.2f MB, %d%% done)", operProgress->curFileName, ft,(int)(pct*100.0));
		}
		frame->SetStatusMessage(msg);
		
		//if(pct < 0.1) pct = 0.1;
		//frame->SetProgressBarPos(pct);
	}
	else
	{
		frame->ProgressBarStepIt();
	}
}
/* irodsNtServer.c */
#include <string.h>
#include "irodsntutil.h"
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#include <setjmp.h>

extern int irodsWinMain(int ac, char **av);
int irodsNtRunService(int ac, char **av);
char irods_service_name[1024];

int main(int argc, char **argv)
{
	int LocalStatus;

	iRODSNtServerCheckExecMode(argc, argv);
	iRODSNtSetServerHomeDir(argv[0]);

	if(iRODSNtServerRunningConsoleMode())
	{
		LocalStatus = irodsWinMain(argc, argv);
	}
	else  /* service mode */
	{
		if(iRODSNtGetServiceName(irods_service_name) < 0)
		{
			exit(0);
		}
		LocalStatus = irodsNtRunService(argc, argv);
	}
	return LocalStatus;
}


SERVICE_STATUS          ssStatus;
SERVICE_STATUS_HANDLE   sshStatusHandle;
DWORD                   dwErr = 0;
BOOL                    bDebug = FALSE;
int						aArgc;
char					**aArgv;

BOOL ReportStatusToSCMgr(DWORD dwCurrentState,
                         DWORD dwWin32ExitCode,
                         DWORD dwWaitHint)
{
    static DWORD dwCheckPoint = 1;

	if(bDebug)
		return TRUE;

    if (dwCurrentState == SERVICE_START_PENDING)
       ssStatus.dwControlsAccepted = 0;
    else
       ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    ssStatus.dwCurrentState = dwCurrentState;
    ssStatus.dwWin32ExitCode = dwWin32ExitCode;
    ssStatus.dwWaitHint = dwWaitHint;

    if ( ( dwCurrentState == SERVICE_RUNNING ) ||
             ( dwCurrentState == SERVICE_STOPPED ) )
        ssStatus.dwCheckPoint = 0;
    else
        ssStatus.dwCheckPoint = dwCheckPoint++;

    /* Report the status of the service to the service control manager. */
    return SetServiceStatus( sshStatusHandle, &ssStatus);
}

VOID ServiceStop()
{
	(VOID)ReportStatusToSCMgr(SERVICE_STOPPED,dwErr,0);
}

VOID WINAPI service_ctrl(DWORD dwCtrlCode)
{
	// Handle the requested control code.
    switch(dwCtrlCode)
    {
        // Stop the service.
        //
        // SERVICE_STOP_PENDING should be reported before
        // setting the Stop Event - hServerStopEvent - in
        // ServiceStop().  This avoids a race condition
        // which may result in a 1053 - The Service did not respond...
        // error.
        case SERVICE_CONTROL_STOP:
            ReportStatusToSCMgr(SERVICE_STOP_PENDING, NO_ERROR, 0);
            ServiceStop();
            return;

        // Update the service status.
        //
        case SERVICE_CONTROL_INTERROGATE:
            break;

        // invalid control code
        //
        default:
            break;

    }

    ReportStatusToSCMgr(ssStatus.dwCurrentState, NO_ERROR, 0);
}

int irodsNtStartService()
{
	int LocalStatus;

	if (!ReportStatusToSCMgr(SERVICE_START_PENDING,NO_ERROR,3000))
	{
		(VOID)ReportStatusToSCMgr(SERVICE_STOPPED,dwErr,0);
		return -1;
	}


	if (!ReportStatusToSCMgr(SERVICE_RUNNING,NO_ERROR,0))
	{
		(VOID)ReportStatusToSCMgr(SERVICE_STOPPED,dwErr,0);
		return -1;
	}

	/* since we are going to run as a WIndows service. We pause for 1 minutes for
	   TCP/IP service to be ready. */
	/* Sleep(60000); */
	LocalStatus = irodsWinMain(aArgc,aArgv);

	fprintf(stderr,"irodsWinMain exit, status = %d\n", LocalStatus);

	return 0;
}

void irodsNtServiceMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
	ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ssStatus.dwServiceSpecificExitCode = 0;

	sshStatusHandle = RegisterServiceCtrlHandler(irods_service_name, service_ctrl);

	if(!sshStatusHandle)
		return;

	if (!ReportStatusToSCMgr(SERVICE_START_PENDING,NO_ERROR,3000))
	{
		(VOID)ReportStatusToSCMgr(SERVICE_STOPPED,dwErr,0);
		return;
	}

	(VOID)irodsNtStartService();
}

int irodsNtRunService(int aargc, char **aargv)
{
	SERVICE_TABLE_ENTRY dispatchTable[] =
    {
        { irods_service_name, (LPSERVICE_MAIN_FUNCTION)irodsNtServiceMain},
        { NULL, NULL }
    };

	aArgc = aargc;
	aArgv = aargv;

	if (!StartServiceCtrlDispatcher(dispatchTable))
	{
		fprintf(stderr,"Windows failed to dispatch the service, '%s'. Make sure the service is installed.\n", irods_service_name);
		return -1;
	}

	return 0;
}

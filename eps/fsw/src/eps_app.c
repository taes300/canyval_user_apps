/************************************************************************
** File: eps_app.c
**
/*
** $Id: $
**
** Subsystem: cFE Application Template (EPS) Application
**
** Author: 
**
** Notes:
**
** $Log: $
**

/************************************************************************
** Includes
*************************************************************************/
#include "eps_app.h"

#include <string.h>
#include <stdio.h>

#include "eps_perfids.h"
#include "eps_msgids.h"
#include "eps_events.h"

//#include "cndh_msg.h"
#include "cndh_msgids.h"
#include "adc_msgids.h"
#include "coms_msgids.h"
//#include "eps_functions.h"
#include "eps_msg.h"

/************************************************************************
** EPS global data
*************************************************************************/

eps_hk_tlm_t    eps_HkTelemetryPkt;
eps_logtlm_t    eps_LogMsg1;
eps_log_2_cndh  eps_LogMsg;
eps_log_2_coms  eps_LogMsg2;



CFE_SB_PipeId_t    eps_CommandPipe;
CFE_SB_MsgPtr_t    epsMsgPtr;

static CFE_EVS_BinFilter_t  EPS_EventFilters[] =
       {  /* Event ID    mask */
          {EPS_STARTUP_INF_EID,       0x0000},
          {EPS_COMMAND_ERR_EID,       0x0000},
          {EPS_COMMANDNOP_INF_EID,    0x0000},
          {EPS_COMMANDRST_INF_EID,    0x0000},
       };

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* EPS application entry point and main process loop                */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void EPS_AppMain( void )
{
    int32  status;
    uint32 RunStatus = CFE_ES_APP_RUN;

    CFE_ES_PerfLogEntry(EPS_APPMAIN_PERF_ID);

    EPS_AppInit();

    /*
    ** eps Runloop
    */
    while (CFE_ES_RunLoop(&RunStatus) == TRUE)
    {
        CFE_ES_PerfLogExit(EPS_APPMAIN_PERF_ID);

        /* Pend on receipt of command packet -- timeout set to 500 millisecs */
        status = CFE_SB_RcvMsg(&epsMsgPtr, eps_CommandPipe, 500);

        CFE_ES_PerfLogEntry(EPS_APPMAIN_PERF_ID);

        if (status == CFE_SUCCESS)
        {
			EPS_ProcessCommandPacket();
        }

    }

    CFE_ES_ExitApp(RunStatus);

} /* End of template_AppMain() */
void EPS_AppInit(void)
{
	CFE_ES_RegisterApp();

	CFE_EVS_Register(EPS_EventFilters,
		sizeof(EPS_EventFilters) / sizeof(CFE_EVS_BinFilter_t),
		CFE_EVS_BINARY_FILTER);

	/*
	** Create the Software Bus command pipe and subscribe to housekeeping
	**  messages
	*/

	CFE_SB_CreatePipe(&eps_CommandPipe, EPS_PIPE_DEPTH, "EPS_CMD_PIPE");

	CFE_SB_Subscribe(EPS_CMD_MID, eps_CommandPipe);
	CFE_SB_Subscribe(EPS_SEND_HK_MID, eps_CommandPipe);
	CFE_SB_Subscribe(EPS_WAKE_UP_MID, eps_CommandPipe);

	EPS_ResetCounters();

	CFE_SB_InitMsg(&eps_HkTelemetryPkt,
					EPS_HK_TLM_MID,
					EPS_HK_TLM_LNGTH, TRUE);

	/*CFE_SB_InitMsg(&eps_LogMsg,
					EPS_LOGMSG_MID,
					EPS_LOG_LNGTH, TRUE);*/

	  CFE_EVS_SendEvent (EPS_STARTUP_INF_EID, CFE_EVS_INFORMATION,
	               "template EPS Initialized. Version %d.%d.%d.%d");
	  eps_LogMsg1.buf1 = 0;
	  eps_LogMsg1.buf2 = 0;
} /* End of template_AppInit() */

void EPS_ProcessCommandPacket(void)
{
    CFE_SB_MsgId_t  MsgId;

    MsgId = CFE_SB_GetMsgId(epsMsgPtr);

    switch (MsgId)
    {
        case EPS_CMD_MID:
        	EPS_ProcessGroundCommand();
            break;
        case EPS_SEND_HK_MID:
            EPS_ReportHousekeeping();
            break;
        case EPS_WAKE_UP_MID:
        	EPS_ProcessScheduleCommand();
			break;
        default:
        	eps_HkTelemetryPkt.eps_command_error_count++;
            CFE_EVS_SendEvent(EPS_COMMAND_ERR_EID,CFE_EVS_ERROR,
			"EPS: invalid command packet,MID = 0x%x", MsgId);
            break;
    }

    return;

} /* End template_ProcessCommandPacket */


void EPS_ProcessGroundCommand(void)
{
    uint16 CommandCode;

    CommandCode = CFE_SB_GetCmdCode(epsMsgPtr);

    switch (CommandCode)
    {
        case EPS_NOOP_CC:
            eps_HkTelemetryPkt.eps_command_count++;
            CFE_EVS_SendEvent(EPS_COMMANDNOP_INF_EID,CFE_EVS_INFORMATION,
			"EPS, NOOP command");
            break;

        case EPS_RESET_COUNTERS_CC:
        	EPS_ResetCounters();
            break;

		case EPS_AX100_ON:
			eps_LogMsg1.AX100_Status = 1;
			OS_printf("EPS, AX100 on\n");
			break;
        /* default case already found during FC vs length test */
        default:
            break;
    }
    return;

} /* End of template_ProcessGroundCommand() */

void EPS_ProcessScheduleCommand(void)
{
    CFE_SB_InitMsg(&eps_LogMsg, CNDH_CMD_MID, sizeof(eps_log_2_cndh), TRUE);
	CFE_SB_InitMsg(&eps_LogMsg2, COMS_CMD_MID, sizeof(eps_log_2_coms), TRUE);

	eps_LogMsg.Soc = 70;
	eps_LogMsg.temp = 30;
	//OS_printf("EPS, Soc, temp read\n");

		/*if (eps_LogMsg1.buf1 == 0)
		{
			CFE_SB_SetCmdCode((CFE_SB_MsgPtr_t) eps_LogMsg2.CmdCHeader,5);
			CFE_SB_SendMsg((CFE_SB_Msg_t *) &eps_LogMsg2);
			eps_LogMsg1.buf1 = 1;
		}*/


	if (eps_LogMsg1.AX100_Status == 1)
	{
		eps_LogMsg.AX100_Status = 1;
		eps_LogMsg2.AX100_Status = 1;
		if (eps_LogMsg1.buf2 == 0)
		{
			CFE_SB_SetCmdCode((CFE_SB_MsgPtr_t) eps_LogMsg2.CmdHeader,4);
			CFE_SB_SendMsg((CFE_SB_Msg_t *) &eps_LogMsg2);
		}
		eps_LogMsg1.buf2 = 1;
	}

	EPS_SendData();

}


void EPS_ReportHousekeeping(void)
{
    CFE_SB_TimeStampMsg((CFE_SB_Msg_t *) &eps_HkTelemetryPkt);
    CFE_SB_SendMsg((CFE_SB_Msg_t *) &eps_HkTelemetryPkt);
    return;
}/* End of template_ReportHousekeeping() */

void EPS_SendData(void)
{

	CFE_SB_SetCmdCode((CFE_SB_MsgPtr_t) eps_LogMsg.CmdHeader,CNDH_GET_EPS_CC);
	CFE_SB_SendMsg((CFE_SB_Msg_t *) &eps_LogMsg);
	return;
}
void EPS_ResetCounters(void)
{
    /* Status of commands processed by the template App */
  eps_HkTelemetryPkt.eps_command_count       = 0;
  eps_HkTelemetryPkt.eps_command_error_count = 0;

  CFE_EVS_SendEvent(EPS_COMMANDRST_INF_EID, CFE_EVS_INFORMATION,
		"EPS, RESET command");
  return;

} /* End of template_ResetCounters() */
boolean EPS_VerifyCmdLength(CFE_SB_MsgPtr_t msg, uint16 ExpectedLength)
{
    boolean result = TRUE;

    uint16 ActualLength = CFE_SB_GetTotalMsgLength(msg);

    /*
    ** Verify the command packet length.
    */
    if (ExpectedLength != ActualLength)
    {
        CFE_SB_MsgId_t MessageID   = CFE_SB_GetMsgId(msg);
        uint16         CommandCode = CFE_SB_GetCmdCode(msg);

        CFE_EVS_SendEvent(EPS_LEN_ERR_EID, CFE_EVS_ERROR,
           "Invalid msg length: ID = 0x%X,  CC = %d, Len = %d, Expected = %d",
              MessageID, CommandCode, ActualLength, ExpectedLength);
        result = FALSE;
        eps_HkTelemetryPkt.eps_command_error_count++;
    }

    return(result);

} /* End of template_VerifyCmdLength() */

/************************/
/*  End of File Comment */
/************************/


#ifndef IDLE_TASK_H
#define IDLE_TASK_H

/*! Parameters that are used to determine if micro can go into LPM3 */
typedef struct
{
  unsigned char SppReadyToSleep;
  unsigned char TaskDelayLockCount;
  unsigned int DisplayMessagesWaiting;
  unsigned int SppMessagesWaiting;
  
} tWatchdogInfo;

/*! This function prints information about the cause of a watchdog reset
 * It also saves the values to non-volatile memory for the last occurence
 * of a watchdog reset.
 *
 * \param ResetSource ( SYSRSTIV )
 */
void WatchdogTimeoutHandler(unsigned char ResetSource);

/*! This function keeps track of the number of messages in each queue. */
void UpdateWatchdogInfo(void);

/*! kick the watchdog timer */
void RestartWatchdog(void);

/*! cause a reset to occur because of the watchdog expiring */
void ForceWatchdogReset(void);

/* Prints reset code and the interrupt type */
void PrintResetSource(void);

/******************************************************************************/

typedef enum
{
  eWrapperTaskCheckInId = 0,
  eDisplayTaskCheckInId = 1,
  /* add user tasks here */
  
} etTaskCheckInId;

#define ALL_TASKS_HAVE_CHECKED_IN ( 0x03 )

void TaskCheckIn(etTaskCheckInId TaskId);

#endif /* IDLE_TASK_H */

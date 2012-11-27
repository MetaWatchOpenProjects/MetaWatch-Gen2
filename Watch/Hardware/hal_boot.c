
#include "msp430.h"

#include "hal_boot.h"


/******************************************************************************/

__no_init __root static unsigned long long Signature @SIGNATURE_ADDR;

void SetBootloaderSignature(void)
{
  Signature = BOOTLOADER_SIGNATURE;
}

void ClearBootloaderSignature(void)
{
  Signature = 0;
}

unsigned long long GetBootloaderSignature(void)
{
  return Signature;
}


/******************************************************************************/

__no_init __root static unsigned int ResetSource @RESET_REASON_ADDR;

void SaveResetSource(void)
{
  /* save and then clear reason for reset */
  ResetSource = SYSRSTIV;
  SYSRSTIV = 0;
  
}

unsigned int GetResetSource(void)
{
  return ResetSource;
}

/******************************************************************************/





#ifndef HAL_BOOT_H
#define HAL_BOOT_H


/******************************************************************************/

/* Things shared between bootloader and main application */

/* This address is in an unused location reserved for the alternate interrupt
 * table
 */
#define SIGNATURE_ADDR ( 0x5B80 )

/* this is at the start of ram and space is reserved in the linker file */
#define RESET_REASON_ADDR ( 0x1C00 )

#define WATCHDOG_INFO_ADDR ( 0x1C02 )

/******************************************************************************/

/* software reset (done using PMMSWBOR) */
#define VALID_RESET_TYPE_FOR_BOOTLOADER_ENTRY ( 0x0006 )
   
/******************************************************************************/
     
/* "MetaBoot" backwards */
#define BOOTLOADER_SIGNATURE ( 0x746F6F426174654D )

void SetBootloaderSignature(void);
void ClearBootloaderSignature(void);
unsigned long long GetBootloaderSignature(void);

void SaveResetSource(void);
unsigned int GetResetSource(void);

#endif /* HAL_BOOT */

/******************************************************************************
  Filename:       OSAL_Nv.c

  Description:    This module contains the OSAL non-volatile memory functions.


  Copyright 2006-2011 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License"). You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product. Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
******************************************************************************/

/******************************************************************************
  Notes:
    - A trick buried deep in initPage() requires that the MSB of the NV Item Id
      is to be reserved for use by this module.
******************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "FreeRTOS.h"
#include "semphr.h"

#include "MSP430FlashUtil.h"
#include "OSAL_Nv.h"
#include "NvIds.h"
#include "DebugUart.h"

/*********************************************************************
 * CONSTANTS
 */

#define TRUE  (1==1)
#define FALSE (0==1)


/* Physical pages per OSAL logical page - increase to get bigger OSAL_NV_ITEMs.
 * Changing this number requires a corresponding change in the linker file
 */
#ifndef OSAL_NV_PHY_PER_PG
  #define OSAL_NV_PHY_PER_PG    2
#endif

#define OSAL_NV_PAGES_USED     (HAL_NV_PAGE_CNT / OSAL_NV_PHY_PER_PG)
#if (OSAL_NV_PAGES_USED < 2)
#error Need to increase the NV_PG_CNT or decrease the OSAL_NV_PHY_PER_PG.
#endif

#if (HAL_NV_PAGE_CNT != (OSAL_NV_PAGES_USED * OSAL_NV_PHY_PER_PG))
#error HAL_NV_PAGE_CNT must be a multiple of OSAL_NV_PHY_PER_PG
#endif

#define OSAL_NV_PAGE_SIZE      (OSAL_NV_PHY_PER_PG * HAL_FLASH_PAGE_SIZE)

#define OSAL_NV_ACTIVE          0x00
#define OSAL_NV_ERASED          0xFF
#define OSAL_NV_ERASED_ID       0xFFFF
#define OSAL_NV_ZEROED_ID       0x0000
// Reserve MSB of Id to signal a search for the "old" source copy (new write interrupted/failed.)
#define OSAL_NV_SOURCE_ID       0x8000

// In case pages 0-1 are ever used, define a null page value.
#define OSAL_NV_PAGE_NULL       OSAL_NV_PAGES_USED

// In case item Id 0 is ever used, define a null item value.
#define OSAL_NV_ITEM_NULL       0

#define OSAL_NV_WORD_SIZE       HAL_FLASH_WORD_SIZE

#define OSAL_NV_PAGE_HDR_OFFSET 0

#define OSAL_NV_MAX_HOT         1

static const unsigned int hotIds[OSAL_NV_MAX_HOT] = {
  NVID_LINK_KEY
};

/*********************************************************************
 * MACROS
 */

#define OSAL_NV_CHECK_BUS_VOLTAGE  ( 1 ) //HalAdcCheckVdd(VDD_MIN_NV)

#define OSAL_NV_DATA_SIZE( LEN )  \
   ((((LEN) + OSAL_NV_WORD_SIZE - 1) / OSAL_NV_WORD_SIZE) * OSAL_NV_WORD_SIZE)

#define OSAL_NV_ITEM_SIZE( LEN )  \
  (((((LEN) + OSAL_NV_WORD_SIZE - 1) / OSAL_NV_WORD_SIZE) * OSAL_NV_WORD_SIZE) + OSAL_NV_HDR_SIZE)

#define COMPACT_PAGE_CLEANUP( COM_PG ) st ( \
  /* In order to recover from a page compaction that is interrupted,\
   * the logic in osal_nv_init() depends upon the following order:\
   * 1. State of the target of compaction is changed to ePgInUse.\
   * 2. Compacted page is erased.\
   */\
  setPageUse( pgRes, TRUE );  /* Mark the reserve page as being in use. */\
  erasePage( (COM_PG) ); \
  \
  pgRes = (COM_PG);           /* Set the reserve page to be the newly erased page. */\
)

// The NV pages must be located in lower flash to simplify read/write operations to using pointers.
#define OSAL_NV_PAGE_TO_PTR(pg) \
  ((unsigned char *)((unsigned char *)((HAL_NV_PAGE_BEG + ((pg) * OSAL_NV_PHY_PER_PG)) * HAL_FLASH_PAGE_SIZE)))


/*
 *  This macro is for use by other macros to form a fully valid C statement.
 *  Without this, the if/else conditionals could show unexpected behavior.
 *
 *  For example, use...
 *    #define SET_REGS()  st( ioreg1 = 0; ioreg2 = 0; )
 *  instead of ...
 *    #define SET_REGS()  { ioreg1 = 0; ioreg2 = 0; }
 *  or
 *    #define  SET_REGS()    ioreg1 = 0; ioreg2 = 0;
 *  The last macro would not behave as expected in the if/else construct.
 *  The second to last macro will cause a compiler error in certain uses
 *  of if/else construct
 *
 *  It is not necessary, or recommended, to use this macro where there is
 *  already a valid C statement.  For example, the following is redundant...
 *    #define CALL_FUNC()   st(  func();  )
 *  This should simply be...
 *    #define CALL_FUNC()   func()
 *
 * (The while condition below evaluates false without generating a
 *  constant-controlling-loop type of warning on most compilers.)
 */
#define st(x)      do { x } while (__LINE__ == -1)

/*********************************************************************
 * TYPEDEFS
 */

typedef struct
{
  unsigned int id;
  unsigned int len;   // Enforce Flash-WORD size on len.
  unsigned int chk;   // Byte-wise checksum of the 'len' data bytes of the item.
  unsigned int stat;  // Item status.
} osalNvHdr_t;
// Struct member offsets.
#define OSAL_NV_HDR_ID    0
#define OSAL_NV_HDR_LEN   2
#define OSAL_NV_HDR_CHK   4
#define OSAL_NV_HDR_STAT  6

#define OSAL_NV_HDR_ITEM  2  // Length of any item of a header struct.
#define OSAL_NV_HDR_SIZE  8
#define OSAL_NV_HDR_HALF (OSAL_NV_HDR_SIZE / 2)

typedef struct
{
  unsigned int active;
  unsigned int inUse;
  unsigned int xfer;
  unsigned int spare;
} osalNvPgHdr_t;
// Struct member offsets.
#define OSAL_NV_PG_ACTIVE 0
#define OSAL_NV_PG_INUSE  2
#define OSAL_NV_PG_XFER   4
#define OSAL_NV_PG_SPARE  6

#define OSAL_NV_PAGE_HDR_SIZE  8
#define OSAL_NV_PAGE_HDR_HALF (OSAL_NV_PAGE_HDR_SIZE / 2)

typedef enum
{
  eNvXfer,
  eNvZero
} eNvHdrEnum;

typedef enum
{
  ePgActive,
  ePgInUse,
  ePgXfer,
  ePgSpare
} ePgHdrEnum;

/*********************************************************************
 * GLOBAL VARIABLES
 */

#ifndef OAD_KEEP_NV_PAGES
// When NV pages are to remain intact during OAD download,
// the image itself should not include NV pages.

#ifdef __IAR_SYSTEMS_ICC__

#pragma location="NV_ADDRESS_SPACE"
__no_init unsigned char _nvBuf[OSAL_NV_PAGES_USED * OSAL_NV_PAGE_SIZE];
#pragma required=_nvBuf

#else

#pragma DATA_SECTION(_nvBuf, ".NV_ADDRESS_SPACE");
unsigned char _nvBuf[OSAL_NV_PAGES_USED * OSAL_NV_PAGE_SIZE];

#endif

#endif // OAD_KEEP_NV_PAGES

/*********************************************************************
 * LOCAL VARIABLES
 */

// Offset into the page of the first available erased space.
static unsigned int pgOff[OSAL_NV_PAGES_USED];

// Count of the bytes lost for the zeroed-out items.
static unsigned int pgLost[OSAL_NV_PAGES_USED];

static unsigned char pgRes;  // Page reserved for item compacting transfer.

// NV page and offsets for hot items.
static unsigned char hotPg[OSAL_NV_MAX_HOT];
static unsigned int hotOff[OSAL_NV_MAX_HOT];

/*********************************************************************
 * LOCAL FUNCTIONS
 */

static unsigned char  initNV( void );

static void   setPageUse( unsigned char pg, unsigned char inUse );
static unsigned int initPage( unsigned char pg, unsigned int id, unsigned char findDups );
static void   erasePage( unsigned char pg );
static unsigned char  compactPage( unsigned char srcPg, unsigned int skipId );

static unsigned int findItem( unsigned int id, unsigned char *findPg );
static unsigned char  initItem( unsigned char flag, unsigned int id, unsigned int len, void *buf );
static void   setItem( unsigned char pg, unsigned int offset, eNvHdrEnum stat );

static unsigned int calcChkB( unsigned int len, unsigned char *buf );
static unsigned int calcChkF( unsigned char pg, unsigned int offset, unsigned int len );

static void   readHdr( unsigned char pg, unsigned int offset, unsigned char *buf );

static void   writeBuf( unsigned char pg, unsigned int offset, unsigned int len, unsigned char *buf );
static void   xferBuf( unsigned char srcPg, unsigned int srcOff, unsigned char dstPg, unsigned int dstOff, unsigned int len );

static unsigned char  writeItem( unsigned char pg, unsigned int id, unsigned int len, void *buf, unsigned char flag );
static unsigned char  hotItem(unsigned int id);
static void   hotItemUpdate(unsigned char pg, unsigned int off, unsigned int id);


/*********************************************************************/

/*
 * Initialize an item in NV
 */
static unsigned char osal_nv_item_init( unsigned int id, unsigned int len, void *buf );

/*
 * Read an NV attribute
 */
static unsigned char osal_nv_read( unsigned int id, unsigned int offset, unsigned int len, void *buf );

/*
 * Write an NV attribute
 */
static unsigned char osal_nv_write( unsigned int id, unsigned int offset, unsigned int len, void *buf );

/*
 * Get the length of an NV item.
 */
static unsigned int osal_nv_item_len( unsigned int id );

/*
 * Delete an NV item.
 */
#if 0
static unsigned char osal_nv_delete( unsigned int id, unsigned int len );
#endif

#define MASTER_RESET_CODE ( 0xDEAF )

unsigned int nvMasterResetKey;
static void InitMasterResetKey(void);

static unsigned char MasterResetOnNvalInit;

static xSemaphoreHandle NvalMutex;

/*********************************************************************
 * @fn      initNV
 *
 * @brief   Initialize the NV flash pages.
 *
 * @param   none
 *
 * @return  TRUE
 */
static unsigned char initNV( void )
{
  osalNvPgHdr_t pgHdr;
  unsigned char oldPg = OSAL_NV_PAGE_NULL;
  unsigned char findDups = FALSE;
  unsigned char pg;

  pgRes = OSAL_NV_PAGE_NULL;

  for ( pg = 0; pg < OSAL_NV_PAGES_USED; pg++ )
  {
    readHdr( pg, OSAL_NV_PAGE_HDR_OFFSET, (unsigned char *)(&pgHdr) );

    if ( pgHdr.active == OSAL_NV_ERASED_ID )
    {
      if ( pgRes == OSAL_NV_PAGE_NULL )
      {
        pgRes = pg;
      }
      else
      {
        setPageUse( pg, TRUE );
      }
    }
    // An Xfer from this page was in progress.
    else if ( pgHdr.xfer != OSAL_NV_ERASED_ID )
    {
      oldPg = pg;
    }
  }

  // If a page compaction was interrupted before the old page was erased.
  if ( oldPg != OSAL_NV_PAGE_NULL )
  {
    /* Interrupted compaction before the target of compaction was put in use;
     * so erase the target of compaction and start again.
     */
    if ( pgRes != OSAL_NV_PAGE_NULL )
    {
      erasePage( pgRes );
      (void)compactPage( oldPg, OSAL_NV_ITEM_NULL );
    }
    /* Interrupted compaction after the target of compaction was put in use,
     * but before the old page was erased; so erase it now and create a new reserve page.
     */
    else
    {
      erasePage( oldPg );
      pgRes = oldPg;
    }
  }
  else if ( pgRes != OSAL_NV_PAGE_NULL )
  {
    erasePage( pgRes );  // The last page erase could have been interrupted by a power-cycle.
  }
  /* else if there is no reserve page, COMPACT_PAGE_CLEANUP() must have succeeded to put the old
   * reserve page (i.e. the target of the compacted items) into use but got interrupted by a reset
   * while trying to erase the page to be compacted. Such a page should only contain duplicate items
   * (i.e. all items will be marked 'Xfer') and thus should have the lost count equal to the page
   * size less the page header.
   */

  for ( pg = 0; pg < OSAL_NV_PAGES_USED; pg++ )
  {
    // Calculate page offset and lost bytes - any "old" item triggers an N^2 re-scan from start.
    if ( initPage( pg, OSAL_NV_ITEM_NULL, findDups ) != OSAL_NV_ITEM_NULL )
    {
      findDups = TRUE;
      pg = (256 - 1);  // Pre-decrement so that loop increment will start over at zero.
      continue;
    }
  }

  if (findDups)
  {
    // Final pass to calculate page lost after invalidating duplicate items.
    for ( pg = 0; pg < OSAL_NV_PAGES_USED; pg++ )
    {
      (void)initPage( pg, OSAL_NV_ITEM_NULL, FALSE );
    }
  }

  if ( pgRes == OSAL_NV_PAGE_NULL )
  {
    unsigned char idx, mostLost = 0;

    for ( idx = 0; idx < OSAL_NV_PAGES_USED; idx++ )
    {
      // Is this the page that was compacted?
      if (pgLost[idx] == (OSAL_NV_PAGE_SIZE - OSAL_NV_PAGE_HDR_SIZE))
      {
        mostLost = idx;
        break;
      }
      /* This check is not expected to be necessary because the above test should always succeed
       * with an early loop exit.
       */
      else if (pgLost[idx] > pgLost[mostLost])
      {
        mostLost = idx;
      }
    }

    pgRes = mostLost;
    erasePage( pgRes );  // The last page erase had been interrupted by a power-cycle.
  }

  return TRUE;
}

/*********************************************************************
 * @fn      setPageUse
 *
 * @brief   Set page header active/inUse state according to 'inUse'.
 *
 * @param   pg - Valid NV page to verify and init.
 * @param   inUse - Boolean TRUE if inUse, FALSE if only active.
 *
 * @return  none
 */
static void setPageUse( unsigned char pg, unsigned char inUse )
{
  osalNvPgHdr_t pgHdr;

  pgHdr.active = OSAL_NV_ZEROED_ID;

  if ( inUse )
  {
    pgHdr.inUse = OSAL_NV_ZEROED_ID;
  }
  else
  {
    pgHdr.inUse = OSAL_NV_ERASED_ID;
  }

  flashWrite(OSAL_NV_PAGE_TO_PTR(pg)+OSAL_NV_PAGE_HDR_OFFSET, OSAL_NV_HDR_HALF, (unsigned char *)(&pgHdr));
}

/*********************************************************************
 * @fn      initPage
 *
 * @brief   Walk the page items; calculate checksums, lost bytes & page offset.
 *
 * @param   pg - Valid NV page to verify and init.
 * @param   id - Valid NV item Id to use function as a "findItem".
 *               If set to NULL then just perform the page initialization.
 * @param   findDups - TRUE on recursive call from initNV() to find and zero-out duplicate items
 *                     left from a write that is interrupted by a reset/power-cycle.
 *                     FALSE otherwise.
 *
 * @return  If 'id' is non-NULL and good checksums are found, return the offset
 *          of the data corresponding to item Id; else OSAL_NV_ITEM_NULL.
 */
static unsigned int initPage( unsigned char pg, unsigned int id, unsigned char findDups )
{
  unsigned int offset = OSAL_NV_PAGE_HDR_SIZE;
  unsigned int sz, lost = 0;
  osalNvHdr_t hdr;

  do
  {
    readHdr( pg, offset, (unsigned char *)(&hdr) );

    if ( hdr.id == OSAL_NV_ERASED_ID )
    {
      break;
    }

    // Get the actual size in bytes which is the ceiling(hdr.len)
    sz = OSAL_NV_DATA_SIZE( hdr.len );

    // A bad 'len' write has blown away the rest of the page.
    if (sz > (OSAL_NV_PAGE_SIZE - OSAL_NV_HDR_SIZE - offset))
    {
      lost += (OSAL_NV_PAGE_SIZE - offset);
      offset = OSAL_NV_PAGE_SIZE;
      break;
    }

    offset += OSAL_NV_HDR_SIZE;

    if ( hdr.id != OSAL_NV_ZEROED_ID )
    {
      /* This trick allows function to do double duty for findItem() without
       * compromising its essential functionality at powerup initialization.
       */
      if ( id != OSAL_NV_ITEM_NULL )
      {
        /* This trick allows asking to find the old/transferred item in case
         * of a successful new item write that gets interrupted before the
         * old item can be zeroed out.
         */
        if ( (id & 0x7fff) == hdr.id )
        {
          if ( (((id & OSAL_NV_SOURCE_ID) == 0) && (hdr.stat == OSAL_NV_ERASED_ID)) ||
               (((id & OSAL_NV_SOURCE_ID) != 0) && (hdr.stat != OSAL_NV_ERASED_ID)) )
          {
            return offset;
          }
        }
      }
      // When invoked from the osal_nv_init(), verify checksums and find & zero any duplicates.
      else
      {
        if ( hdr.chk == calcChkF( pg, offset, hdr.len ) )
        {
          if ( findDups )
          {
            if ( hdr.stat == OSAL_NV_ERASED_ID )
            {
              /* The trick of setting the MSB of the item Id causes the logic
               * immediately above to return a valid page only if the header 'stat'
               * indicates that it was the older item being transferred.
               */
              unsigned char findPg;
              unsigned int off = findItem( (hdr.id | OSAL_NV_SOURCE_ID), &findPg );

              if ( off != OSAL_NV_ITEM_NULL )
              {
                setItem( findPg, off, eNvZero );  // Mark old duplicate as invalid.
              }
            }
          }
          // Any "old" item immediately exits and triggers the N^2 exhaustive initialization.
          else if ( hdr.stat != OSAL_NV_ERASED_ID )
          {
            return OSAL_NV_ERASED_ID;
          }
        }
        else
        {
          setItem( pg, offset, eNvZero );  // Mark bad checksum as invalid.
          lost += (OSAL_NV_HDR_SIZE + sz);
        }
      }
    }
    else
    {
      lost += (OSAL_NV_HDR_SIZE + sz);
    }
    offset += sz;

  } while (offset < (OSAL_NV_PAGE_SIZE - OSAL_NV_HDR_SIZE));

  pgOff[pg] = offset;
  pgLost[pg] = lost;

  return OSAL_NV_ITEM_NULL;
}

/*********************************************************************
 * @fn      erasePage
 *
 * @brief   Erases a page in Flash.
 *
 * @param   pg - Valid NV page to erase.
 *
 * @return  none
 */
static void erasePage( unsigned char pg )
{
  unsigned char *addr = OSAL_NV_PAGE_TO_PTR(pg);
  unsigned char cnt = OSAL_NV_PHY_PER_PG;

  do {
    flashErasePage(addr);
    addr += HAL_FLASH_PAGE_SIZE;
  } while (--cnt);

  pgOff[pg] = OSAL_NV_PAGE_HDR_SIZE;
  pgLost[pg] = 0;
}

/*********************************************************************
 * @fn      compactPage
 *
 * @brief   Compacts the page specified.
 *
 * @param   srcPg - Valid NV page to erase.
 * @param   skipId - Item Id to not compact.
 *
 * @return  TRUE if valid items from 'srcPg' are successully compacted onto the 'pgRes';
 *          FALSE otherwise.
 *          Note that on a failure, this could loop, re-erasing the 'pgRes' and re-compacting with
 *          the risk of infinitely looping on HAL flash failure.
 *          Worst case scenario: HAL flash starts failing in general, perhaps low Vdd?
 *          All page compactions will fail which will cause all osal_nv_write() calls to return
 *          NV_OPER_FAILED.
 *          Eventually, all pages in use may also be in the state of "pending compaction" where
 *          the page header member OSAL_NV_PG_XFER is zeroed out.
 *          During this "HAL flash brown-out", the code will run and OTA should work (until low Vdd
 *          causes an actual chip brown-out, of course.) Although no new NV items will be created
 *          or written, the last value written with a return value of NV_SUCCESS can continue to be
 *          read successfully.
 *          If eventually HAL flash starts working again, all of the pages marked as
 *          "pending compaction" may or may not be eventually compacted. But, initNV() will
 *          deterministically clean-up one page pending compaction per power-cycle
 *          (if HAL flash is working.) Nevertheless, one erased reserve page will be maintained
 *          through such a scenario.
 */
static unsigned char compactPage( unsigned char srcPg, unsigned int skipId )
{
  unsigned int srcOff = OSAL_NV_PAGE_HDR_SIZE;
  unsigned char rtrn = TRUE;

  while ( srcOff < (OSAL_NV_PAGE_SIZE - OSAL_NV_HDR_SIZE ) )
  {
    osalNvHdr_t hdr;
    unsigned int sz, dstOff = pgOff[pgRes];

    readHdr( srcPg, srcOff, (unsigned char *)(&hdr) );

    if ( hdr.id == OSAL_NV_ERASED_ID )
    {
      break;
    }

    // Get the actual size in bytes which is the ceiling(hdr.len)
    sz = OSAL_NV_DATA_SIZE( hdr.len );

    if ( (sz > (OSAL_NV_PAGE_SIZE - OSAL_NV_HDR_SIZE - srcOff)) ||
         (sz > (OSAL_NV_PAGE_SIZE - OSAL_NV_HDR_SIZE - dstOff)) )
    {
      break;
    }

    srcOff += OSAL_NV_HDR_SIZE;

    if ( (hdr.id != OSAL_NV_ZEROED_ID) && (hdr.id != skipId) )
    {
      if ( hdr.chk == calcChkF( srcPg, srcOff, hdr.len ) )
      {
        /* Prevent excessive re-writes to item header caused by numerous, rapid, & successive
         * OSAL_Nv interruptions caused by resets.
         */
        if ( hdr.stat == OSAL_NV_ERASED_ID )
        {
          setItem( srcPg, srcOff, eNvXfer );
        }

        if ( writeItem( pgRes, hdr.id, hdr.len, NULL, FALSE ) )
        {
          unsigned int chk;

          dstOff += OSAL_NV_HDR_SIZE;
          xferBuf( srcPg, srcOff, pgRes, dstOff, sz );
          // Calculate and write the new checksum.
          chk = calcChkF( pgRes, dstOff, hdr.len );
          dstOff -= OSAL_NV_HDR_SIZE;
          flashWrite(OSAL_NV_PAGE_TO_PTR(pgRes) + dstOff + OSAL_NV_HDR_CHK, OSAL_NV_HDR_ITEM,
                                                                            (unsigned char *)(&chk));
          chk = hdr.chk;
          readHdr( pgRes, dstOff, (unsigned char *)(&hdr) );

          if ( chk != hdr.chk )
          {
            rtrn = FALSE;
            break;
          }
          else
          {
            hotItemUpdate(pgRes, dstOff+OSAL_NV_HDR_SIZE, hdr.id);
          }
        }
        else
        {
          rtrn = FALSE;
          break;
        }
      }
    }

    srcOff += sz;
  }

  if (rtrn == FALSE)
  {
    erasePage(pgRes);
  }
  else if (skipId == OSAL_NV_ITEM_NULL)
  {
    COMPACT_PAGE_CLEANUP(srcPg);
  }
  // else invoking function must cleanup.

  return rtrn;
}

/*********************************************************************
 * @fn      findItem
 *
 * @brief   Find an item Id in NV and return the page and offset to its data.
 *
 * @param   id - Valid NV item Id.
 *
 * @return  Offset of data corresponding to item Id, if found;
 *          otherwise OSAL_NV_ITEM_NULL.
 *
 *          The page containing the item, if found;
 *          otherwise no valid assignment made - left equal to item Id.
 *
 */
static unsigned int findItem( unsigned int id, unsigned char *findPg )
{
  unsigned int off;
  unsigned char pg;

  for ( pg = 0; pg < OSAL_NV_PAGES_USED; pg++ )
  {
    if ( (off = initPage( pg, id, FALSE )) != OSAL_NV_ITEM_NULL )
    {
      *findPg = pg;
      return off;
    }
  }

  // Now attempt to find the item as the "old" item of a failed/interrupted NV write.
  if ( (id & OSAL_NV_SOURCE_ID) == 0 )
  {
    return findItem( (id | OSAL_NV_SOURCE_ID), findPg );
  }
  else
  {
    *findPg = OSAL_NV_PAGE_NULL;
    return OSAL_NV_ITEM_NULL;
  }
}

/*********************************************************************
 * @fn      initItem
 *
 * @brief   An NV item is created and initialized with the data passed to the function, if any.
 *
 * @param   flag - TRUE if the 'buf' parameter contains data for the call to writeItem().
 *                 (i.e. if invoked from osal_nv_item_init() ).
 *                 FALSE if writeItem() should just write the header and the 'buf' parameter
 *                 is ok to use as a return value of the page number to be cleaned with
 *                 COMPACT_PAGE_CLEANUP().
 *                 (i.e. if invoked from osal_nv_write() ).
 * @param   id  - Valid NV item Id.
 * @param   len - Item data length.
 * @param  *buf - Pointer to item initalization data. Set to NULL if none.
 *
 * @return  The OSAL Nv page number if item write and read back checksums ok;
 *          OSAL_NV_PAGE_NULL otherwise.
 */
static unsigned char initItem( unsigned char flag, unsigned int id, unsigned int len, void *buf )
{
  unsigned int sz = OSAL_NV_ITEM_SIZE( len );
  unsigned char rtrn = OSAL_NV_PAGE_NULL;
  unsigned char cnt = OSAL_NV_PAGES_USED;
  unsigned char pg = pgRes+1;  // Set to 1 after the reserve page to even wear across all available pages.

  do {
    if (pg >= OSAL_NV_PAGES_USED)
    {
      pg = 0;
    }
    if ( pg != pgRes )
    {
      if ( sz <= (OSAL_NV_PAGE_SIZE - pgOff[pg] + pgLost[pg]) )
      {
        break;
      }
    }
    pg++;
  } while (--cnt);

  if (cnt)
  {
    // Item fits if an old page is compacted.
    if ( sz > (OSAL_NV_PAGE_SIZE - pgOff[pg]) )
    {
      // Mark the old page as being in process of compaction.
      sz = OSAL_NV_ZEROED_ID;
      flashWrite(OSAL_NV_PAGE_TO_PTR(pg) + OSAL_NV_PAGE_HDR_OFFSET + OSAL_NV_PG_XFER,
                                           OSAL_NV_HDR_ITEM, (unsigned char *)(&sz));

      /* First the old page is compacted, then the new item will be the last one written to what
       * had been the reserved page.
       */
      if (compactPage( pg, id ))
      {
        if ( writeItem( pgRes, id, len, buf, flag ) )
        {
          rtrn = pgRes;
        }

        if ( flag == FALSE )
        {
          /* Overload 'buf' as an OUT parameter to pass back to the calling function
           * the old page to be cleaned up.
           */
          *(unsigned char *)buf = pg;
        }
        else
        {
          /* Safe to do the compacted page cleanup even if writeItem() above failed because the
           * item does not yet exist since this call with flag==TRUE is from osal_nv_item_init().
           */
          COMPACT_PAGE_CLEANUP( pg );
        }
      }
    }
    else
    {
      if ( writeItem( pg, id, len, buf, flag ) )
      {
        rtrn = pg;
      }
    }
  }

  return rtrn;
}

/*********************************************************************
 * @fn      setItem
 *
 * @brief   Set an item Id or status to mark its state.
 *
 * @param   pg - Valid NV page.
 * @param   offset - Valid offset into the page of the item data - the header
 *                   offset is calculated from this.
 * @param   stat - Valid enum value for the item status.
 *
 * @return  none
 */
static void setItem( unsigned char pg, unsigned int offset, eNvHdrEnum stat )
{
  osalNvHdr_t hdr;

  offset -= OSAL_NV_HDR_SIZE;
  readHdr( pg, offset, (unsigned char *)(&hdr) );

  if ( stat == eNvXfer )
  {
    hdr.stat = OSAL_NV_ACTIVE;
    flashWrite(OSAL_NV_PAGE_TO_PTR(pg) + offset + OSAL_NV_HDR_STAT, OSAL_NV_HDR_ITEM,
                                                             (unsigned char *)(&(hdr.stat)));
  }
  else // if ( stat == eNvZero )
  {
    unsigned int sz = ((hdr.len + (OSAL_NV_WORD_SIZE-1)) / OSAL_NV_WORD_SIZE) * OSAL_NV_WORD_SIZE +
                                                                          OSAL_NV_HDR_SIZE;
    hdr.id = 0;
    flashWrite(OSAL_NV_PAGE_TO_PTR(pg) + offset, OSAL_NV_HDR_ITEM, (unsigned char*)(&hdr));
    pgLost[pg] += sz;
  }
}

/*********************************************************************
 * @fn      calcChkB
 *
 * @brief   Calculates the data checksum over the 'buf' parameter.
 *
 * @param   len - Byte count of the data to be checksummed.
 * @param   buf - Data buffer to be checksummed.
 *
 * @return  Calculated checksum of the data bytes.
 */
static unsigned int calcChkB( unsigned int len, unsigned char *buf )
{
  unsigned char fill = len % OSAL_NV_WORD_SIZE;
  unsigned int chk;

  if ( !buf )
  {
    chk = len * OSAL_NV_ERASED;
  }
  else
  {
    chk = 0;
    while ( len-- )
    {
      chk += *buf++;
    }
  }

  // calcChkF() will calculate over OSAL_NV_WORD_SIZE alignment.
  if ( fill )
  {
    chk += (OSAL_NV_WORD_SIZE - fill) * OSAL_NV_ERASED;
  }

  return chk;
}

/*********************************************************************
 * @fn      calcChkF
 *
 * @brief   Calculates the data checksum by reading the data bytes from NV.
 *
 * @param   pg - A valid NV Flash page.
 * @param   offset - A valid offset into the page.
 * @param   len - Byte count of the data to be checksummed.
 *
 * @return  Calculated checksum of the data bytes.
 */
static unsigned int calcChkF( unsigned char pg, unsigned int offset, unsigned int len )
{
  unsigned char *addr = OSAL_NV_PAGE_TO_PTR( pg ) + offset;
  unsigned int chk = 0;

  len = (len + (OSAL_NV_WORD_SIZE-1)) / OSAL_NV_WORD_SIZE;

  while ( len-- )
  {
    chk += *addr++;
  }

  return chk;
}

/*********************************************************************
 * @fn      readHdr
 *
 * @brief   Reads "sizeof( osalNvHdr_t )" bytes from NV.
 *
 * @param   pg - Valid NV page.
 * @param   offset - Valid offset into the page.
 * @param   buf - Valid buffer space of at least sizeof( osalNvHdr_t ) bytes.
 *
 * @return  none
 */
static void readHdr( unsigned char pg, unsigned int offset, unsigned char *buf )
{
  unsigned char *addr = OSAL_NV_PAGE_TO_PTR( pg ) + offset;
  unsigned char len = OSAL_NV_HDR_SIZE;

  do
  {
    *buf++ = *addr++;
  } while ( --len );
}

/*********************************************************************
 * @fn      writeBuf
 *
 * @brief   Writes a data buffer to NV.
 *
 * @param   dstPg - A valid NV Flash page.
 * @param   offset - A valid offset into the page.
 * @param   len  - Byte count of the data to write.
 * @param   buf  - The data to write.
 *
 * @return  TRUE if data buf checksum matches read back checksum, else FALSE.
 */
static void writeBuf( unsigned char dstPg, unsigned int dstOff, unsigned int len, unsigned char *buf )
{
  unsigned char *addr;
  unsigned char idx, rem, tmp[OSAL_NV_WORD_SIZE];

  rem = dstOff % OSAL_NV_WORD_SIZE;
  if ( rem )
  {
    dstOff -= rem;
    addr = OSAL_NV_PAGE_TO_PTR( dstPg ) + dstOff;

    for ( idx = 0; idx < rem; idx++ )
    {
      tmp[idx] = *addr++;
    }

    while ( (idx < OSAL_NV_WORD_SIZE) && len )
    {
      tmp[idx++] = *buf++;
      len--;
    }

    while ( idx < OSAL_NV_WORD_SIZE )
    {
      tmp[idx++] = OSAL_NV_ERASED;
    }

    flashWrite(OSAL_NV_PAGE_TO_PTR(dstPg) + dstOff, OSAL_NV_WORD_SIZE, tmp);
    dstOff += OSAL_NV_WORD_SIZE;
  }

  rem = len % OSAL_NV_WORD_SIZE;
  len = (len / OSAL_NV_WORD_SIZE) * OSAL_NV_WORD_SIZE;
  flashWrite(OSAL_NV_PAGE_TO_PTR(dstPg) + dstOff, len, buf);

  if ( rem )
  {
    dstOff += len;
    buf += len;

    for ( idx = 0; idx < rem; idx++ )
    {
      tmp[idx] = *buf++;
    }

    while ( idx < OSAL_NV_WORD_SIZE )
    {
      tmp[idx++] = OSAL_NV_ERASED;
    }

    flashWrite(OSAL_NV_PAGE_TO_PTR(dstPg) + dstOff, OSAL_NV_WORD_SIZE, tmp);
  }
}

/*********************************************************************
 * @fn      xferBuf
 *
 * @brief   Xfers an NV buffer from one location to another, enforcing OSAL_NV_WORD_SIZE writes.
 *
 * @return  none
 */
static void xferBuf( unsigned char srcPg, unsigned int srcOff, unsigned char dstPg, unsigned int dstOff, unsigned int len )
{
  unsigned char *addr;
  unsigned char idx, rem, tmp[OSAL_NV_WORD_SIZE];

  rem = dstOff % OSAL_NV_WORD_SIZE;
  if ( rem )
  {
    dstOff -= rem;
    addr = OSAL_NV_PAGE_TO_PTR( dstPg ) + dstOff;

    for ( idx = 0; idx < rem; idx++ )
    {
      tmp[idx] = *addr++;
    }

    addr = OSAL_NV_PAGE_TO_PTR( srcPg ) + srcOff;

    while ( (idx < OSAL_NV_WORD_SIZE) && len )
    {
      tmp[idx++] = *addr++;
      srcOff++;
      len--;
    }

    while ( idx < OSAL_NV_WORD_SIZE )
    {
      tmp[idx++] = OSAL_NV_ERASED;
    }

    flashWrite(OSAL_NV_PAGE_TO_PTR(dstPg) + dstOff, OSAL_NV_WORD_SIZE, tmp);
    dstOff += OSAL_NV_WORD_SIZE;
  }

  rem = len % OSAL_NV_WORD_SIZE;
  len = (len / OSAL_NV_WORD_SIZE) * OSAL_NV_WORD_SIZE;
  addr = OSAL_NV_PAGE_TO_PTR( srcPg ) + srcOff;
  flashWrite(OSAL_NV_PAGE_TO_PTR(dstPg) + dstOff, len, addr);

  if ( rem )
  {
    dstOff += len;
    addr += len;

    for ( idx = 0; idx < rem; idx++ )
    {
      tmp[idx] = *addr++;
    }

    while ( idx < OSAL_NV_WORD_SIZE )
    {
      tmp[idx++] = OSAL_NV_ERASED;
    }

    flashWrite(OSAL_NV_PAGE_TO_PTR(dstPg) + dstOff, OSAL_NV_WORD_SIZE, tmp);
  }
}

/*********************************************************************
 * @fn      writeItem
 *
 * @brief   Writes an item header/data combo to the specified NV page.
 *
 * @param   pg - Valid NV Flash page.
 * @param   id - Valid NV item Id.
 * @param   len  - Byte count of the data to write.
 * @param   buf  - The data to write. If NULL, no data/checksum write.
 * @param   flag - TRUE if the checksum should be written, FALSE otherwise.
 *
 * @return  TRUE if header/data to write matches header/data read back, else FALSE.
 */
static unsigned char writeItem( unsigned char pg, unsigned int id, unsigned int len, void *buf, unsigned char flag )
{
  unsigned int offset = pgOff[pg];
  unsigned char rtrn = FALSE;
  osalNvHdr_t hdr;

  hdr.id = id;
  hdr.len = len;

  flashWrite(OSAL_NV_PAGE_TO_PTR(pg) + offset, OSAL_NV_HDR_HALF, (unsigned char *)(&hdr));
  readHdr( pg, offset, (unsigned char *)(&hdr) );

  if ( (hdr.id == id) && (hdr.len == len) )
  {
    if ( flag )
    {
      unsigned int chk = calcChkB( len, buf );

      offset += OSAL_NV_HDR_SIZE;
      if ( buf != NULL )
      {
        writeBuf( pg, offset, len, buf );
      }

      if ( chk == calcChkF( pg, offset, len ) )
      {
        flashWrite(OSAL_NV_PAGE_TO_PTR(pg) + offset - OSAL_NV_HDR_HALF, OSAL_NV_HDR_ITEM,
                                                                        (unsigned char *)(&chk));
        readHdr( pg, (offset-OSAL_NV_HDR_SIZE), (unsigned char *)(&hdr) );

        if ( chk == hdr.chk )
        {
          hotItemUpdate(pg, offset, hdr.id);
          rtrn = TRUE;
        }
      }
    }
    else
    {
      rtrn = TRUE;
    }

    len = OSAL_NV_ITEM_SIZE( hdr.len );
  }
  else
  {
    len = OSAL_NV_ITEM_SIZE( hdr.len );

    if (len > (OSAL_NV_PAGE_SIZE - pgOff[pg]))
    {
      len = (OSAL_NV_PAGE_SIZE - pgOff[pg]);
    }

    pgLost[pg] += len;
  }
  pgOff[pg] += len;

  return rtrn;
}

/*********************************************************************
 * @fn      hotItem
 *
 * @brief   Look for the parameter 'id' in the hot items array.
 *
 * @param   id - A valid NV item Id.
 *
 * @return  A valid index into the hot items if the item is hot; OSAL_NV_MAX_HOT if not.
 */
static unsigned char hotItem(unsigned int id)
{
  unsigned char hotIdx;

  for (hotIdx = 0; hotIdx < OSAL_NV_MAX_HOT; hotIdx++)
  {
    if (hotIds[hotIdx] == id)
    {
      break;
    }
  }

  return hotIdx;
}

/*********************************************************************
 * @fn      hotItemUpdate
 *
 * @brief   If the parameter 'id' is a hot item, update the corresponding hot item data.
 *
 * @param   pg - The new NV page corresponding to the hot item.
 * @param   off - The new NV page offset corresponding to the hot item.
 * @param   id - A valid NV item Id.
 *
 * @return  none
 */
static void hotItemUpdate(unsigned char pg, unsigned int off, unsigned int id)
{
  unsigned char hotIdx = hotItem(id);

  if (hotIdx < OSAL_NV_MAX_HOT)
  {
    {
      hotPg[hotIdx] = pg;
      hotOff[hotIdx] = off;
    }
  }
}

/*********************************************************************
 * @fn      OsalNvInit
 *
 * @brief   Initialize NV service.
 *
 * @param   p - Not used.
 *
 * @return  none
 */
void OsalNvInit( void *p )
{
  (void)p;  // Suppress Lint warning.
  (void)initNV();  // Always returns TRUE after pages have been erased.
  NvalMutex = xSemaphoreCreateMutex();
  xSemaphoreGive(NvalMutex);
  InitMasterResetKey();
}

/* redefine the failure case to include debug print output */
static inline unsigned char NvOperationFailed(void)
{
  PrintString("NV operation Failed\r\n");
  return NV_OPER_FAILED;  
}

/*********************************************************************
 * @fn      osal_nv_item_init
 *
 * @brief   If the NV item does not already exist, it is created and
 *          initialized with the data passed to the function, if any.
 *          This function must be called before calling osal_nv_read() or
 *          osal_nv_write().
 *
 * @param   id  - Valid NV item Id.
 * @param   len - Item length.
 * @param  *buf - Pointer to item initalization data. Set to NULL if none.
 *
 * @return  NV_ITEM_UNINIT - Id did not exist and was created successfully.
 *          NV_SUCCESS       - Id already existed, no action taken.
 *          NV_OPER_FAILED - Failure to find or create Id.
 */
static unsigned char osal_nv_item_init( unsigned int id, unsigned int len, void *buf )
{
  unsigned char findPg;
  unsigned int offset;

  if ( !OSAL_NV_CHECK_BUS_VOLTAGE )
  {
    return NvOperationFailed();
  }
  else if ((offset = findItem(id, &findPg)) != OSAL_NV_ITEM_NULL)
  {
    // Re-populate the NV hot item data if the corresponding items are already established.
    hotItemUpdate(findPg, offset, id);

    return NV_SUCCESS;
  }
  else if ( initItem( TRUE, id, len, buf ) != OSAL_NV_PAGE_NULL )
  {
    return NV_ITEM_UNINIT;
  }
  else
  {
    return NvOperationFailed();
  }
}

/*********************************************************************
 * @fn      osal_nv_item_len
 *
 * @brief   Get the data length of the item stored in NV memory.
 *
 * @param   id  - Valid NV item Id.
 *
 * @return  Item length, if found; zero otherwise.
 */
static unsigned int osal_nv_item_len( unsigned int id )
{
  unsigned char findPg;
  osalNvHdr_t hdr;
  unsigned int offset;
  unsigned char hotIdx;

  if ((hotIdx = hotItem(id)) < OSAL_NV_MAX_HOT)
  {
    findPg = hotPg[hotIdx];
    offset = hotOff[hotIdx];
  }
  else if ((offset = findItem(id, &findPg)) == OSAL_NV_ITEM_NULL)
  {
    return 0;
  }

  readHdr( findPg, (offset - OSAL_NV_HDR_SIZE), (unsigned char *)(&hdr) );
  return hdr.len;
}

/*********************************************************************
 * @fn      osal_nv_write
 *
 * @brief   Write a data item to NV. Function can write an entire item to NV or
 *          an element of an item by indexing into the item with an offset.
 *
 * @param   id  - Valid NV item Id.
 * @param   ndx - Index offset into item
 * @param   len - Length of data to write.
 * @param  *buf - Data to write.
 *
 * @return  NV_SUCCESS if successful, NV_ITEM_UNINIT if item did not
 *          exist in NV and offset is non-zero, NV_OPER_FAILED if failure.
 */
static unsigned char osal_nv_write( unsigned int id, unsigned int ndx, unsigned int len, void *buf )
{
  unsigned char rtrn = NV_SUCCESS;

  if ( !OSAL_NV_CHECK_BUS_VOLTAGE )
  {
    return NvOperationFailed();
  }
  else if ( len != 0 )
  {
    osalNvHdr_t hdr;
    unsigned int origOff, srcOff;
    unsigned int cnt, chk;
    unsigned char *addr, *ptr, srcPg;

    origOff = srcOff = findItem( id, &srcPg );
    if ( srcOff == OSAL_NV_ITEM_NULL )
    {
      return NV_ITEM_UNINIT;
    }

    readHdr( srcPg, (srcOff - OSAL_NV_HDR_SIZE), (unsigned char *)(&hdr) );
    if ( hdr.len < (ndx + len) )
    {
      return NvOperationFailed();
    }

    addr = OSAL_NV_PAGE_TO_PTR( srcPg ) + srcOff + ndx;
    ptr = buf;
    cnt = len;
    chk = 0;
    while ( cnt-- )
    {
      if ( *addr != *ptr )
      {
        chk = 1;  // Mark that at least one byte is different.
        // Calculate expected checksum after transferring old data and writing new data.
        hdr.chk -= *addr;
        hdr.chk += *ptr;
      }
      addr++;
      ptr++;
    }

    if ( chk != 0 )  // If the buffer to write is different in one or more bytes.
    {
      unsigned char comPg = OSAL_NV_PAGE_NULL;
      unsigned char dstPg = initItem( FALSE, id, hdr.len, &comPg );

      if ( dstPg != OSAL_NV_PAGE_NULL )
      {
        unsigned int tmp = OSAL_NV_DATA_SIZE( hdr.len );
        unsigned int dstOff = pgOff[dstPg] - tmp;
        srcOff = origOff;
        chk = hdr.chk;

        /* Prevent excessive re-writes to item header caused by numerous, rapid, & successive
         * OSAL_Nv interruptions caused by resets.
         */
        if ( hdr.stat == OSAL_NV_ERASED_ID )
        {
          setItem( srcPg, srcOff, eNvXfer );
        }

        xferBuf( srcPg, srcOff, dstPg, dstOff, ndx );
        srcOff += ndx;
        dstOff += ndx;

        writeBuf( dstPg, dstOff, len, buf );
        srcOff += len;
        dstOff += len;

        xferBuf( srcPg, srcOff, dstPg, dstOff, (hdr.len-ndx-len) );

        // Calculate and write the new checksum.
        dstOff = pgOff[dstPg] - tmp;
        tmp = calcChkF( dstPg, dstOff, hdr.len );
        dstOff -= OSAL_NV_HDR_SIZE;
        flashWrite(OSAL_NV_PAGE_TO_PTR(dstPg) + dstOff + OSAL_NV_HDR_CHK, OSAL_NV_HDR_ITEM,
                                                                          (unsigned char *)(&tmp));
        readHdr( dstPg, dstOff, (unsigned char *)(&hdr) );

        if ( chk != hdr.chk )
        {
          rtrn = NvOperationFailed();
        }
        else
        {
          hotItemUpdate(dstPg, dstOff+OSAL_NV_HDR_SIZE, hdr.id);
        }
      }
      else
      {
        rtrn = NvOperationFailed();
      }

      if ( comPg != OSAL_NV_PAGE_NULL )
      {
        /* Even though the page compaction succeeded, if the new item is coming from the compacted
         * page and writing the new value failed, then the compaction must be aborted.
         */
        if ( (srcPg == comPg) && (rtrn == NV_OPER_FAILED) )
        {
          erasePage( pgRes );
        }
        else
        {
          COMPACT_PAGE_CLEANUP( comPg );
        }
      }

      /* Zero of the old item must wait until after compact page cleanup has finished - if the item
       * is zeroed before and cleanup is interrupted by a power-cycle, the new item can be lost.
       */
      if ( (srcPg != comPg) && (rtrn != NV_OPER_FAILED) )
      {
        setItem( srcPg, origOff, eNvZero );
      }
    }
  }

  return rtrn;
}

/*********************************************************************
 * @fn      osal_nv_read
 *
 * @brief   Read data from NV. This function can be used to read an entire item from NV or
 *          an element of an item by indexing into the item with an offset.
 *          Read data is copied into *buf.
 *
 * @param   id  - Valid NV item Id.
 * @param   ndx - Index offset into item
 * @param   len - Length of data to read.
 * @param  *buf - Data is read into this buffer.
 *
 * @return  NV_SUCCESS if NV data was copied to the parameter 'buf'.
 *          Otherwise, NV_OPER_FAILED for failure.
 */
static unsigned char osal_nv_read( unsigned int id, unsigned int ndx, unsigned int len, void *buf )
{
  unsigned char *addr, *ptr = (unsigned char *)buf;
  unsigned char findPg;
  unsigned int offset;
  unsigned char hotIdx;

  if ((hotIdx = hotItem(id)) < OSAL_NV_MAX_HOT)
  {
    findPg = hotPg[hotIdx];
    offset = hotOff[hotIdx];
  }
  else if ((offset = findItem(id, &findPg)) == OSAL_NV_ITEM_NULL)
  {
    return NvOperationFailed();
  }

  addr = OSAL_NV_PAGE_TO_PTR(findPg) + offset + ndx;
  while ( len-- )
  {
    *ptr++ = *addr++;
  }

  return NV_SUCCESS;
}

#if 0
/*********************************************************************
 * @fn      osal_nv_delete
 *
 * @brief   Delete item from NV. This function will fail if the length
 *          parameter does not match the length of the item in NV.
 *
 * @param   id  - Valid NV item Id.
 * @param   len - Length of item to delete.
 *
 * @return  NV_SUCCESS if item was deleted,
 *          NV_ITEM_UNINIT if item did not exist in NV,
 *          NV_BAD_ITEM_LEN if length parameter not correct,
 *          NV_OPER_FAILED if attempted deletion failed.
 */
static unsigned char osal_nv_delete( unsigned int id, unsigned int len )
{
  unsigned char findPg;
  unsigned int length;
  unsigned int offset;

  offset = findItem( id, &findPg );
  if ( offset == OSAL_NV_ITEM_NULL )
  {
    // NV item does not exist
    return NV_ITEM_UNINIT;
  }

  length = osal_nv_item_len( id );
  if ( length != len )
  {
    // NV item has different length
    return NV_BAD_ITEM_LEN;
  }

  // Set item header ID to zero to 'delete' the item
  setItem( findPg, offset, eNvZero );

  // Verify that item has been removed
  offset = findItem( id, &findPg );
  if ( offset != OSAL_NV_ITEM_NULL )
  {
    // Still there
    return NvOperationFailed();
  }
  else
  {
    // Yes, it's gone
    return NV_SUCCESS;
  }
}
#endif

extern unsigned char osal_nv_item_init( unsigned int id, unsigned int len, void *buf );

/*
 * Read an NV attribute
 */
extern unsigned char osal_nv_read( unsigned int id, unsigned int offset, unsigned int len, void *buf );



void PrintNvalSaveError(char *pString)
{
  PrintString3("Unable to save",pString," to non-volatile memory.\r\n");    
}



/******************************************************************************/

unsigned char OsalNvRead( unsigned int id, unsigned int offset, unsigned int len, void *buf )
{
  xSemaphoreTake(NvalMutex,portMAX_DELAY);
  
  unsigned char result = osal_nv_read(id,offset,len,buf);
  
  xSemaphoreGive(NvalMutex);
  
  if ( result != NV_SUCCESS )
  {
    PrintString("NvalError\r\n");    
  }
  
  return result;
  
}

unsigned char OsalNvWrite( unsigned int id, unsigned int offset, unsigned int len, void *buf )
{
  xSemaphoreTake(NvalMutex,portMAX_DELAY);
  
  unsigned char result = osal_nv_write(id,offset,len,buf);
  
  xSemaphoreGive(NvalMutex);
  
  if ( result != NV_SUCCESS )
  {
    PrintString("NvalError\r\n");    
  }
  
  return result;
  
}

unsigned int OsalNvItemLength( unsigned int id )
{
  xSemaphoreTake(NvalMutex,portMAX_DELAY);
  
  unsigned int result = osal_nv_item_len(id);
  
  xSemaphoreGive(NvalMutex);
  
  return result;
}

void OsalNvItemInit(unsigned int id, unsigned int len, void *buf )
{
  xSemaphoreTake(NvalMutex,portMAX_DELAY);
  
  if( NV_SUCCESS == osal_nv_item_init(id,len,buf) )
  {
    if ( MasterResetOnNvalInit == 1 )
    {
      osal_nv_write(id,NV_ZERO_OFFSET,len,buf);
    }
    else
    {
      osal_nv_read(id,NV_ZERO_OFFSET,len,buf);  
    }
  }   
  
  xSemaphoreGive(NvalMutex);
  
}


void InitMasterResetKey(void)
{ 
  xSemaphoreTake(NvalMutex,portMAX_DELAY);
  
  nvMasterResetKey = 0;
  MasterResetOnNvalInit = 0;
  if( NV_SUCCESS == osal_nv_item_init(NVID_MASTER_RESET,      
                                      sizeof(nvMasterResetKey),
                                      &nvMasterResetKey)  )
  {
    osal_nv_read(NVID_MASTER_RESET,
                 NV_ZERO_OFFSET,
                 sizeof(nvMasterResetKey),
                 &nvMasterResetKey);    
  }  
  
  if ( nvMasterResetKey == MASTER_RESET_CODE )
  {
    nvMasterResetKey = 0;
    MasterResetOnNvalInit = 1;

    osal_nv_write(NVID_MASTER_RESET,
                  NV_ZERO_OFFSET,
                  sizeof(nvMasterResetKey),
                  &nvMasterResetKey); 
  }
  
  xSemaphoreGive(NvalMutex);
  
}

void WriteMasterResetKey(void)
{
  nvMasterResetKey = MASTER_RESET_CODE;
  
  xSemaphoreTake(NvalMutex,portMAX_DELAY);
  
  osal_nv_write(NVID_MASTER_RESET,
                NV_ZERO_OFFSET,
                sizeof(nvMasterResetKey),
                &nvMasterResetKey);  
  
  xSemaphoreGive(NvalMutex);
  
}

/******************************************************************************/

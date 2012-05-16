//==============================================================================
//  Copyright 2011 Meta Watch Ltd. - http://www.MetaWatch.org/
// 
//  Licensed under the Meta Watch License, Version 1.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//  
//      http://www.MetaWatch.org/licenses/license-1.0.html
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//==============================================================================

/******************************************************************************/
/*! \file hal_accelerometer.h
 *
 * The accelerometer is an I2C device.  This contains the KIONIX device address
 * and the register mapping.
 *
 * Functions are provided to read and write the accelerometer (they block and 
 * must be called from a task).
 */
/******************************************************************************/

#ifndef HAL_ACCELEROMETER_H
#define HAL_ACCELEROMETER_H

/*! I2C Device Address 
 * read or write is determined by the least significant bit
 * 
 * This is handled by the micro 
 *
 * KIONIX_WRITE = ( KIONIX_DEVICE_ADDRESS | 0x00 );
 * KIONIX_READ  = ( KIONIX_DEVICE_ADDRESS | 0x01 );
 */
#define KIONIX_DEVICE_ADDRESS ( 0x0F )

/* KIONIX accelerometer register addresses */
#define KIONIX_XOUT_HPF_L        ( 0x00 )
#define KIONIX_XOUT_HPF_H        ( 0x01 )
#define KIONIX_YOUT_HPF_L        ( 0x02 )
#define KIONIX_YOUT_HPF_H        ( 0x03 )
#define KIONIX_ZOUT_HPF_L        ( 0x04 )
#define KIONIX_ZOUT_HPF_H        ( 0x05 )
#define KIONIX_XOUT_L            ( 0x06 )
#define KIONIX_XOUT_H            ( 0x07 )
#define KIONIX_YOUT_L            ( 0x08 )
#define KIONIX_YOUT_H            ( 0x09 )
#define KIONIX_ZOUT_L            ( 0x0A )
#define KIONIX_ZOUT_H            ( 0x0B )
#define KIONIX_DCST_RESP         ( 0x0C )
#define KIONIX_WHO_AM_I          ( 0x0F )
#define KIONIX_TILT_POS_CUR      ( 0x10 )
#define KIONIX_TILT_POS_PRE      ( 0x11 )
#define KIONIX_INT_SRC_REG1      ( 0x15 )
#define KIONIX_INT_SRC_REG2      ( 0x16 )
#define KIONIX_STATUS_REG        ( 0x18 )
#define KIONIX_INT_REL           ( 0x1A )
#define KIONIX_CTRL_REG1         ( 0x1B )
#define KIONIX_CTRL_REG2         ( 0x1C )
#define KIONIX_CTRL_REG3         ( 0x1D )
#define KIONIX_INT_CTRL_REG1     ( 0x1E )
#define KIONIX_INT_CTRL_REG2     ( 0x1F )
#define KIONIX_INT_CTRL_REG3     ( 0x20 )
#define KIONIX_DATA_CTRL_REG     ( 0x21 )
#define KIONIX_TILT_TIMER        ( 0x28 )
#define KIONIX_WUF_TIMER         ( 0x29 )
#define KIONIX_TDT_TIMER         ( 0x2B )
#define KIONIX_TDT_H_THRESH      ( 0x2C )
#define KIONIX_TDT_L_THRESH      ( 0x2D )
#define KIONIX_TDT_TAP_TIMER     ( 0x2E )
#define KIONIX_TDT_TOTAL_TIMER   ( 0x2F )
#define KIONIX_TDT_LATENCY_TIMER ( 0x30 )
#define KIONIX_TDT_WINDOW_TIMER  ( 0x31 )
#define KIONIX_SELF_TEST         ( 0x3A )
#define KIONIX_WUF_THRESH        ( 0x5A )
#define KIONIX_TILT_ANGLE        ( 0x5C )
#define KIONIX_HYST_SET          ( 0x5F )

/******************************************************************************/

/* CTRL_REG1 */
#define PC1_STANDBY_MODE   ( 0 << 7 )
#define PC1_OPERATING_MODE ( 1 << 7 )
#define RESOLUTION_8BIT    ( 0 << 6 )
#define RESOLUTION_12BIT   ( 1 << 6 )
#define WUF_ENABLE         ( 1 << 1 )
#define TAP_ENABLE_TDTE    ( 1 << 2 ) 
#define TILT_ENABLE_TPE    ( 1 << 0 )

/* CTRL_REG3 */
#define SRST               ( 1 << 7 )
#define TILT_ODR_1_6HZ     ( 0 << 5 )
#define TILT_ODR_6_3HZ     ( 1 << 5 )
#define TILT_ODR_12_5HZ    ( 2 << 5 )
#define TILT_ODR_50HZ      ( 3 << 5 )
#define DCST               ( 1 << 4 )
#define TAP_ODR_50HZ       ( 0 << 2 )
#define TAP_ODR_100HZ      ( 1 << 2 )
#define TAP_ODR_200HZ      ( 2 << 2 )
#define TAP_ODR_400HZ      ( 3 << 2 )
#define WUF_ODR_25HZ       ( 0 << 0 )
#define WUF_ODR_50HZ       ( 1 << 0 )
#define WUF_ODR_100HZ      ( 2 << 0 )
#define WUF_ODR_200HZ      ( 3 << 0 )

/* INT_CTRL_REG1 */
#define IEN ( 1 << 5 ) 
#define IEA ( 1 << 4 )
#define IEL ( 1 << 3 )
#define IEU ( 1 << 2 )

/* INT_CNTRL_REG2 */
#define XBW ( 1 << 7 ) 
#define YBW ( 1 << 6 )
#define ZBW ( 1 << 5 )

/* for readability */
#define ONE_BYTE ( 1 )

/******************************************************************************/

/*! Initialize the I2C interface in the msp430 that talks to the accelerometer */
void InitAccelerometerPeripheral(void);


/*! Write data to the accelerometer
 * 
 * \param RegisterAddress is an address in accelerometer chip
 * \param pData is a pointer to data 
 * \param Length is the number of bytes to write
 *
 * \note function must be called from a task that can block
 */
void AccelerometerWrite(unsigned char RegisterAddress,
                        unsigned char* pData,
                        unsigned char Length);

/*! Read data from the accelerometer
 * 
 * \param RegisterAddress is an address in accelerometer chip
 * \param pData is a pointer to an array of characters large enough to hold the read data
 * \param Length is the number of bytes to read
 *
 * \note function must be called from a task that can block
 */
void AccelerometerRead(unsigned char RegisterAddress,
                       unsigned char* pData,
                       unsigned char Length);


#endif /* HAL_ACCELEROMETER_H */

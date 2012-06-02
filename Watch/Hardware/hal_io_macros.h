//==============================================================================
//  Copyright Meta Watch Ltd. - http://www.MetaWatch.org/
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
/*! \file hal_io_macros.c
 *
 * Separate the pin definitions for each board type from the macros
 *
 */
/******************************************************************************/

#ifndef HAL_IO_MACROS
#define HAL_IO_MACROS

/******************************************************************************/

#define BT_CLK_REQ_CONIFIG_AS_INPUT() { \
  BT_CLK_REQ_PDIR &= ~BT_CLK_REQ_PIN; \
}

#define BT_CLK_REQ_CONIFIG_AS_OUTPUT_LOW() { \
  BT_CLK_REQ_PDIR |= BT_CLK_REQ_PIN; \
  BT_CLK_REQ_POUT &= ~BT_CLK_REQ_PIN; \
}

#define BT_IO1_CONFIG_AS_INPUT() { \
  BT_IO1_PDIR &= ~BT_IO1_PIN; }

#define BT_IO1_CONFIG_AS_OUTPUT_LOW() { \
  BT_IO1_PDIR |= BT_IO1_PIN;  \
  BT_IO1_POUT &= ~BT_IO1_PIN; \
}

#define BT_IO2_CONFIG_AS_INPUT() { \
  BT_IO2_PDIR &= ~BT_IO2_PIN; \
}

#define BT_IO2_CONFIG_AS_OUTPUT_LOW() { \
  BT_IO2_PDIR |= BT_IO2_PIN; \
  BT_IO2_POUT &= ~BT_IO2_PIN; }

/******************************************************************************/

#define ACCELEROMETER_INT_ENABLE() { \
  ACCELEROMETER_INT_PIFG &= ~ACCELEROMETER_INT_PIN;   \
  ACCELEROMETER_INT_PIE |= ACCELEROMETER_INT_PIN;     \
}

#define ACCELEROMETER_INT_DISABLE() { \
  ACCELEROMETER_INT_PIE &= ~ACCELEROMETER_INT_PIN;     \
  ACCELEROMETER_INT_PIFG &= ~ACCELEROMETER_INT_PIN;    \
}

#define CONFIG_ACCELEROMETER_PINS() { \
  ACCELEROMETER_SDA_PSEL |= ACCELEROMETER_SDA_PIN;  \
  ACCELEROMETER_SCL_PSEL |= ACCELEROMETER_SCL_PIN;  \
  ACCELEROMETER_INT_PDIR &= ~ACCELEROMETER_INT_PIN; \
}

/* only configuration 5 and later boards support turning on/off the power
 * to the accelerometer
 */
#define ENABLE_ACCELEROMETER_POWER() { \
  ACCELEROMETER_POWER_POUT |= ACCELEROMETER_POWER_PINS; \
}

#define DISABLE_ACCELEROMETER_POWER() { \
  ACCELEROMETER_POWER_POUT &= ~ACCELEROMETER_POWER_PINS; \
}

/******************************************************************************/

#endif /* HAL_IO_MACROS */

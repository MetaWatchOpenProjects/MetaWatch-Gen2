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
/*! \file hal_digital_v2_defs.h
 *
 * Pin and peripheral definitions for Digital Watch Version 2
 */
/******************************************************************************/

#ifndef HAL_DIGITAL_V2_DEFS_H
#define HAL_DIGITAL_V2_DEFS_H


// Defines for the LCD display interface, single channel SPI output
#define LCD_5V_PDIR P4DIR
#define LCD_5V_POUT P4OUT
#define LCD_5V_BIT  BIT0

#define DISABLE_DISPLAY_POWER() { LCD_5V_POUT &= ~LCD_5V_BIT; }
#define DISABLE_LCD_ENABLE() { LCD_ENABLE_POUT &= LCD_ENABLE_PIN; }

#define LCD_ENABLE_PDIR ( P3DIR )
#define LCD_ENABLE_POUT ( P3OUT )
#define LCD_ENABLE_PIN  ( BIT6 )

#define LCD_CS_PDIR ( P3DIR )
#define LCD_CS_POUT ( P3OUT )
#define LCD_CS_PIN  ( BIT2 )

#define LCD_SPI_PORT_SEL ( P3SEL )
#define LCD_SPI_SIMO_BIT ( BIT1 )
#define LCD_SPI_CLK_BIT  ( BIT3 )

#define LCD_CS_ASSERT()   { LCD_CS_POUT |= LCD_CS_PIN; }
#define LCD_CS_DEASSERT() { LCD_CS_POUT &= ~LCD_CS_PIN; }

/* unused in this version */
#define SPECIAL_DMA_CONFIG() { }
#define OLED_CS1N_ENABLE()   { }
#define OLED_CS1N_DISABLE()  { }


#define CONFIG_LCD_PINS() { \
  LCD_SPI_PORT_SEL |= LCD_SPI_SIMO_BIT; \
  LCD_SPI_PORT_SEL |= LCD_SPI_CLK_BIT; \
  LCD_ENABLE_PDIR |= LCD_ENABLE_PIN; \
  LCD_ENABLE_POUT |= LCD_ENABLE_PIN; \
  LCD_CS_PDIR |= LCD_CS_PIN; \
  LCD_CS_DEASSERT(); \
}

// labeled EL_EN on schematic
#define LCD_LED_PDIR ( P4DIR )
#define LCD_LED_POUT ( P4OUT )
#define LCD_LED_PIN  ( BIT4 )

#define DISABLE_LCD_LED() { \
  LCD_LED_PDIR |= LCD_LED_PIN;  \
  LCD_LED_POUT &= ~LCD_LED_PIN; \
}

#define ENABLE_LCD_LED() { \
  LCD_LED_PDIR |= LCD_LED_PIN;  \
  LCD_LED_POUT |= LCD_LED_PIN;  \
}
// Use UCB2 as the SPI port define the registers and interrupt vector to all be
// for UCB2 by replacing the x in the last section of the name with a 2
#define LCD_SPI_UCBxCTL0            UCB0CTL0          // Control register
#define LCD_SPI_UCBxCTL1            UCB0CTL1          // Control register
#define LCD_SPI_UCBxBR0             UCB0BR0           // Baudrate prescale
#define LCD_SPI_UCBxBR1             UCB0BR1           // Baudrate divider

#define LCD_SPI_UCBxTXBUF           UCB0TXBUF         // Transmit buffer
#define LCD_SPI_UCBxRXBUF           UCB0RXBUF         // Receive buffer

#define LCD_SPI_UCBxIE              UCB0IE            // Interrupt enable register
#define LCD_SPI_UCBxIFG             UCB0IFG           // Interrupt flag register
#define LCD_SPI_UCBxIV              UCB0IV            // Interrupt vector register
#define LCD_SPI_UCBxSTAT            UCB0STAT          // status register

#if 0
// Definitions for the OLED Display
// peripheral mapping for OLED
#define OLED_I2C_CTL0  ( UCB0CTL0 )
#define OLED_I2C_CTL1  ( UCB0CTL1 )
#define OLED_I2C_BR0   ( UCB0BR0 )
#define OLED_I2C_BR1   ( UCB0BR1 )
#define OLED_I2C_I2CSA ( UCB0I2CSA )
#define OLED_I2C_IE    ( UCB0IE )
#define OLED_I2C_IFG   ( UCB0IFG )
#define OLED_I2C_TXBUF ( UCB0TXBUF )
#define OLED_I2C_RXBUF ( UCB0RXBUF )
// interrupt mapping for OLED
#define USCI_OLED_I2C_VECTOR ( USCI_B0_VECTOR )
#define USCI_OLED_I2C_IV     ( UCB0IV ) 

// OLED reset is active low
#define OLED_RSTN_PDIR  ( P3DIR )
#define OLED_RSTN_POUT  ( P3OUT )
#define OLED_RSTN_PIN   ( BIT3  )

#define OLED_RSTN_CONFIG()   { OLED_RSTN_PDIR |= OLED_RSTN_PIN; OLED_RSTN_ASSERT(); }
#define OLED_RSTN_ASSERT()   { OLED_RSTN_POUT &= ~OLED_RSTN_PIN; }
#define OLED_RSTN_DEASSERT() { OLED_RSTN_POUT |= OLED_RSTN_PIN; }

#define OLED_CS1N_PDIR ( P8DIR )
#define OLED_CS1N_POUT ( P8OUT )
#define OLED_CS1N_PIN  ( BIT0 )

#define OLED_CS1N_CONFIG()  { OLED_CS1N_PDIR |= OLED_CS1N_PIN; OLED_CS1N_DISABLE(); }
#define OLED_CS1N_ENABLE()  { OLED_CS1N_POUT &= ~OLED_CS1N_PIN; }
#define OLED_CS1N_DISABLE() { OLED_CS1N_POUT |= OLED_CS1N_PIN; }

#define OLED_CS2N_PDIR ( P8DIR )
#define OLED_CS2N_POUT ( P8OUT )
#define OLED_CS2N_PIN  ( BIT1 )

#define OLED_CS2N_CONFIG()  { OLED_CS2N_PDIR |= OLED_CS2N_PIN; OLED_CS2N_DISABLE(); }
#define OLED_CS2N_ENABLE()  { OLED_CS2N_POUT &= ~OLED_CS2N_PIN; }
#define OLED_CS2N_DISABLE() { OLED_CS2N_POUT |= OLED_CS2N_PIN; }

#define OLED_SDA_PDIR ( P3DIR )
#define OLED_SDA_POUT ( P3OUT )
#define OLED_SDA_PIN  ( BIT1 )
#define OLED_SDA_PSEL ( P3SEL)

#define OLED_SCL_PDIR ( P3DIR )
#define OLED_SCL_POUT ( P3OUT )
#define OLED_SCL_PORT ( P3 )
#define OLED_SCL_PIN  ( BIT2 )
#define OLED_SCL_PSEL ( P3SEL )

// the select line overrides the pdir and pout settings
#define OLED_SDA_CONFIG_FOR_PERIPHERAL_USE() { OLED_SDA_PSEL |= OLED_SDA_PIN; }
#define OLED_SCL_CONFIG_FOR_PERIPHERAL_USE() { OLED_SCL_PSEL |= OLED_SCL_PIN; }

#define OLED_I2C_CONFIG_FOR_PERIPHERAL_USE() { \
  OLED_SDA_CONFIG_FOR_PERIPHERAL_USE();        \
  OLED_SCL_CONFIG_FOR_PERIPHERAL_USE(); }

#define OLED_SDA_CONFIG_FOR_SLEEP() { \
  OLED_SDA_PSEL &= ~OLED_SDA_PIN;     \
  OLED_SDA_PDIR |= OLED_SDA_PIN;      \
  OLED_SDA_POUT |= OLED_SDA_PIN; }

#define OLED_SCL_CONFIG_FOR_SLEEP() { \
  OLED_SCL_PSEL &= ~OLED_SCL_PIN;     \
  OLED_SCL_PDIR |= OLED_SCL_PIN;      \
  OLED_SCL_POUT |= OLED_SCL_PIN; }

#define OLED_I2C_CONFIG_FOR_SLEEP() { \
  OLED_SDA_CONFIG_FOR_SLEEP();        \
  OLED_SCL_CONFIG_FOR_SLEEP(); }

#define OLED_POWER_ENABLE_PIN  ( BIT0 )
#define OLED_POWER_ENABLE_POUT ( P4OUT )
#define OLED_POWER_ENABLE_PDIR ( P4DIR )
#define OLED_POWER_CONFIG()    { P4DIR |= OLED_POWER_ENABLE_PIN; OLED_POWER_DISABLE(); }
#define OLED_POWER_ENABLE()    { P4OUT |= OLED_POWER_ENABLE_PIN; }
#define OLED_POWER_DISABLE()   { P4OUT &= ~OLED_POWER_ENABLE_PIN; }

// VLED_EN is active low
#define OLED_IO_POWER_ENABLE_PIN  ( BIT3 )
#define OLED_IO_POWER_ENABLE_POUT ( P5OUT )
#define OLED_IO_POWER_ENABLE_PDIR ( P5DIR )
#define OLED_IO_POWER_CONFIG()    { P5DIR |= OLED_IO_POWER_ENABLE_PIN; OLED_IO_POWER_DISABLE(); }
#define OLED_IO_POWER_ENABLE()    { P5OUT &= ~OLED_IO_POWER_ENABLE_PIN; }
#define OLED_IO_POWER_DISABLE()   { P5OUT |= OLED_IO_POWER_ENABLE_PIN; }






#endif // OLED

#define CONFIG_OLED_PINS() { }


// Real time clock defines
// RTC 1 Hz output, also used to toggle the bias on the LCD
#define RTC_1HZ_PORT_DIR  ( P2DIR )
#define RTC_1HZ_PORT_SEL  ( P2SEL )
#define RTC_1HZ_BIT       ( BIT4 )


// Button defines

#define BUTTON_TIME_COUNT_ARRAY_LEN  8
#define ALL_BUTTONS_OFF              0xFF

#define SW_A  BIT0
#define SW_B  BIT1
#define SW_C  BIT2
#define SW_D  BIT3
// Bit 4 is not used
#define SW_E  BIT5
#define SW_F  BIT6
#define SW_P  BIT7

#define SW_A_INDEX        ( 0 )
#define SW_B_INDEX        ( 1 )
#define SW_C_INDEX        ( 2 )
#define SW_D_INDEX        ( 3 )
#define SW_UNUSED_INDEX   ( 4 )
#define SW_E_INDEX        ( 5 )
#define SW_F_INDEX        ( 6 )
#define SW_P_INDEX        ( 7 )
/* the switch does not count */
#define NUMBER_OF_BUTTONS ( 7 )

#ifdef ANALOG
  #define ALL_BUTTONS (SW_A | SW_B | SW_C | SW_D | SW_E | SW_F | SW_P)
#elif defined(DIGITAL)
  #define ALL_BUTTONS (SW_A | SW_B | SW_C | SW_D | SW_E | SW_F)
#endif

// The digital watch version 1 has 1 Meg pull downs
// so the code below wont work
#define RESISTOR_ENABLE_BUTTONS  ALL_BUTTONS

// Setting the edge select bit high generates and interrupt on the falling edge
#define INT_EDGE_SEL_BUTTONS     (ALL_BUTTONS)

#define BUTTON_PORT_VECTOR PORT2_VECTOR
#define BUTTON_PORT_DIR    P2DIR
#define BUTTON_PORT_SEL    P2SEL
#define BUTTON_PORT_OUT    P2OUT
#define BUTTON_PORT_REN    P2REN
#define BUTTON_PORT_IE     P2IE
#define BUTTON_PORT_IES    P2IES
#define BUTTON_PORT_IFG    P2IFG

#define DISABLE_BUTTONS() { \
  BUTTON_PORT_IE  &= ~INT_EDGE_SEL_BUTTONS; \
  BUTTON_PORT_REN &= ~ALL_BUTTONS;          \
  BUTTON_PORT_OUT &= ~ALL_BUTTONS;          \
}

// NOTE the the buttons are grounded. That means that we want to invert the bits
// when reading the port to have positive logic where a button press is a "1"
#define BUTTON_PORT_IN     ~P2IN

// Configure the hardware port for the button support P2.4 is excluded
#define CONFIGURE_BUTTON_PINS() { \
  BUTTON_PORT_REN = (unsigned char) RESISTOR_ENABLE_BUTTONS; \
  BUTTON_PORT_OUT = (unsigned char) ALL_BUTTONS;             \
  BUTTON_PORT_DIR &= (unsigned char) ~ALL_BUTTONS;           \
  BUTTON_PORT_IES |=   INT_EDGE_SEL_BUTTONS;         \
  BUTTON_PORT_IFG &=  ~ALL_BUTTONS;                  \
  BUTTON_PORT_IE  |=   ALL_BUTTONS;                  \
}


// Battery charger control for a BQ24080
#define BAT_CHARGE_DIR        P6DIR
#define BAT_CHARGE_REN        P6REN
#define BAT_CHARGE_OUT        P6OUT
#define BAT_CHARGE_IN         P6IN

#define BAT_CHARGE_ENABLE_PIN  BIT2
#define BAT_CHARGE_STAT1       BIT3
#define BAT_CHARGE_STAT2       BIT4
#define BAT_CHARGE_PWR_GOOD    BIT5

// Enable charging, asserted low
#define BATTERY_CHARGE_ENABLE()   { BAT_CHARGE_OUT &= ~BAT_CHARGE_ENABLE_PIN; }

// disable charging, sleep the part
#define BATTERY_CHARGE_DISABLE()  { BAT_CHARGE_OUT |= BAT_CHARGE_ENABLE_PIN; }


//
// Ambient Light Sensor
//
#define GC1_PDIR ( P6DIR )
#define GC1_POUT ( P6OUT )
#define GC1_PSEL ( P6SEL )
#define GC1_PIN  ( BIT0 )

#define GC2_PDIR ( P6DIR )
#define GC2_POUT ( P6OUT )
#define GC2_PSEL ( P6SEL )
#define GC2_PIN  ( BIT6 )

#define IOUT_PDIR ( P6DIR )
#define IOUT_PSEL ( P6SEL )
#define IOUT_PIN  ( BIT1 )

#define LIGHT_SENSE_INIT()                     \
{                                              \
  IOUT_PDIR &= ~IOUT_PIN;                      \
  IOUT_PSEL |= IOUT_PIN;                       \
  GC1_PDIR  |= GC1_PIN;                        \
  GC2_PDIR  |= GC2_PIN;                        \
  LIGHT_SENSE_DISABLE();                       \
}

#define LIGHT_SENSOR_SHUTDOWN() \
{                               \
  GC1_POUT &= ~GC1_PIN;         \
  GC2_POUT &= ~GC2_PIN;         \
}

#define LIGHT_SENSOR_L_GAIN() \
{                             \
  GC1_POUT |= GC1_PIN;        \
  GC2_POUT |= GC2_PIN;        \
}

#define LIGHT_SENSE_ENABLE()\
{                           \
  LIGHT_SENSOR_L_GAIN();    \
}

#define LIGHT_SENSE_DISABLE() \
{                             \
  LIGHT_SENSOR_SHUTDOWN();    \
}



//
// This pin controls if the battery voltage can be read
//
#define BATTERY_READ_CONTROL_PDIR ( P4DIR )
#define BATTERY_READ_CONTROL_POUT ( P4OUT )
#define BATTERY_READ_CONTROL_PIN  ( BIT6 )

#define BATTERY_SENSE_INPUT_PDIR ( P7DIR )
#define BATTERY_SENSE_INPUT_PSEL ( P7SEL )
#define BATTERY_SENSE_INPUT_PIN  ( BIT7 )

#define BATTERY_SENSE_INIT()                             \
{                                                        \
  BATTERY_READ_CONTROL_PDIR |= BATTERY_READ_CONTROL_PIN; \
  BATTERY_SENSE_INPUT_PDIR &= ~BATTERY_SENSE_INPUT_PIN;  \
  BATTERY_SENSE_INPUT_PSEL |= BATTERY_SENSE_INPUT_PIN;   \
  BATTERY_SENSE_DISABLE();                               \
}

#define BATTERY_SENSE_ENABLE() \
{                                                        \
  BATTERY_READ_CONTROL_POUT |= BATTERY_READ_CONTROL_PIN; \
}

#define BATTERY_SENSE_DISABLE() \
{                                                         \
  BATTERY_READ_CONTROL_POUT &= ~BATTERY_READ_CONTROL_PIN; \
}

//
// hardware configuration is a voltage divider to ADC input
//
#define HARDWARE_CFG_CONTROL_PDIR ( P8DIR )
#define HARDWARE_CFG_CONTROL_POUT ( P8OUT )
#define HARDWARE_CFG_CONTROL_PIN  ( BIT2 )

#define HARDWARE_CFG_INPUT_PDIR ( P7DIR )
#define HARDWARE_CFG_INPUT_PSEL ( P7SEL )
#define HARDWARE_CFG_INPUT_PIN  ( BIT5 )

#define HARDWARE_CFG_SENSE_INIT()                        \
{                                                        \
  HARDWARE_CFG_CONTROL_PDIR |= HARDWARE_CFG_CONTROL_PIN; \
  HARDWARE_CFG_INPUT_PDIR &= ~HARDWARE_CFG_INPUT_PIN;    \
  HARDWARE_CFG_INPUT_PSEL |= HARDWARE_CFG_INPUT_PIN;     \
  HARDWARE_CFG_SENSE_DISABLE();                          \
}

#define HARDWARE_CFG_SENSE_ENABLE()                         \
{                                                           \
  HARDWARE_CFG_CONTROL_POUT |= HARDWARE_CFG_CONTROL_PIN;    \
}

#define HARDWARE_CFG_SENSE_DISABLE()                        \
{                                                           \
  HARDWARE_CFG_CONTROL_POUT &= ~HARDWARE_CFG_CONTROL_PIN;   \
}


#define APPLE_SDA_PDIR ( P10DIR )
#define APPLE_SDA_POUT ( P10OUT )
#define APPLE_SDA_PIN  ( BIT1 )

#define APPLE_SCL_PDIR ( P10DIR )
#define APPLE_SCL_POUT ( P10OUT )
#define APPLE_SCL_PIN  ( BIT2 )

#define APPLE_POWER_PDIR ( P4DIR )
#define APPLE_POWER_POUT ( P4OUT )
#define APPLE_POWER_PIN  ( BIT5 )

/* power enable is active low */
#define APPLE_POWER_ENABLE()   { APPLE_POWER_POUT &= ~APPLE_POWER_PIN; }
#define APPLE_POWER_DISABLE()  { APPLE_POWER_POUT |= APPLE_POWER_PIN; }

#define APPLE_POWER_CONFIG()   { \
  APPLE_POWER_PDIR |= APPLE_POWER_PIN; \
  APPLE_POWER_DISABLE(); \
}


/* since the apple chip is currently not powered 
 * the pins should be at 0 instead of 1
 */
#define APPLE_I2C_PIN_CONFIG() {    \
  APPLE_SDA_PDIR |= APPLE_SDA_PIN;  \
  APPLE_SDA_POUT &= ~APPLE_SDA_PIN; \
  APPLE_SCL_PDIR |= APPLE_SCL_PIN;  \
  APPLE_SCL_POUT &= ~APPLE_SCL_PIN; \
}

#define APPLE_CONFIG() {  \
  APPLE_POWER_CONFIG();   \
  APPLE_I2C_PIN_CONFIG(); \
}

#define SRAM_SCLK_PSEL ( P3SEL )
#define SRAM_SCLK_PDIR ( P3DIR )
#define SRAM_SCLK_POUT ( P3OUT )
#define SRAM_SCLK_PIN  ( BIT0 )

#define SRAM_SOMI_PSEL ( P3SEL )
#define SRAM_SOMI_PDIR ( P3DIR )
#define SRAM_SOMI_POUT ( P3OUT )
#define SRAM_SOMI_PIN  ( BIT5 )

#define SRAM_SIMO_PSEL ( P3SEL )
#define SRAM_SIMO_PDIR ( P3DIR )
#define SRAM_SIMO_POUT ( P3OUT )
#define SRAM_SIMO_PIN  ( BIT4 )

#define SRAM_CSN_PDIR ( P9DIR )
#define SRAM_CSN_POUT ( P9OUT )
#define SRAM_CSN_PIN  ( BIT6)

#define SRAM_HOLDN_PDIR ( P9DIR )
#define SRAM_HOLDN_POUT ( P9OUT )
#define SRAM_HOLDN_PIN  ( BIT7 )

#define CONFIG_SRAM_PINS() \
{                                     \
  SRAM_CSN_PDIR |= SRAM_CSN_PIN;      \
  SRAM_CSN_POUT |= SRAM_CSN_PIN;      \
  SRAM_HOLDN_PDIR |= SRAM_HOLDN_PIN;  \
  SRAM_HOLDN_POUT |= SRAM_HOLDN_PIN;  \
}

#define SRAM_CSN_ASSERT() { SRAM_CSN_POUT &= ~SRAM_CSN_PIN; }

#define SRAM_CSN_DEASSERT() { \
  WAIT_FOR_SRAM_SPI_SHIFT_COMPLETE(); \
  SRAM_CSN_POUT |= SRAM_CSN_PIN; \
}

/* wait for shift to complete ( ~3 us ) */
#define WAIT_FOR_SRAM_SPI_SHIFT_COMPLETE() { while( (UCA0STAT & 0x01) != 0 ); }

#define ACCELEROMETER_SDA_PIN  ( BIT7 )
#define ACCELEROMETER_SDA_PSEL ( P3SEL )
#define ACCELEROMETER_SDA_POUT ( P3OUT )
#define ACCELEROMETER_SDA_PDIR ( P3OUT )
#define ACCELEROMETER_SDA_PREN ( P1REN )

#define ACCELEROMETER_SCL_PIN  ( BIT4 )
#define ACCELEROMETER_SCL_PSEL ( P5SEL )
#define ACCELEROMETER_SCL_POUT ( P5OUT )
#define ACCELEROMETER_SCL_PDIR ( P5OUT )
#define ACCELEROMETER_SCL_PREN ( P5REN )

#define ACCELEROMETER_INT_PIN  ( BIT7 )
#define ACCELEROMETER_INT_PDIR ( P1DIR )
#define ACCELEROMETER_INT_PSEL ( P1SEL )
#define ACCELEROMETER_INT_PIFG ( P1IFG )
#define ACCELEROMETER_INT_PIE  ( P1IE )

#define ACCELEROMETER_POWER_POUT ( P9OUT )
#define ACCELEROMETER_POWER_PINS ( BIT1 | BIT2 )

/* this is for reference only (it doesn't control anything) */
#define ACCELEROMETER_INT_NUM (PIN7_INT)

#define LED4_ON() { }
#define LED5_ON() { }
#define LED6_ON() { }
#define LED7_ON() { }

#define LED4_OFF() {  }
#define LED5_OFF() {  }
#define LED6_OFF() {  }
#define LED7_OFF() {  }

#define CONFIG_LED_PINS() { }



#define DEBUG1_HIGH() {  }
#define DEBUG3_HIGH() {  }
#define DEBUG4_HIGH() {  }
#define DEBUG5_HIGH() { }

#define DEBUG1_LOW() {  }
#define DEBUG3_LOW() { }
#define DEBUG4_LOW() { }
#define DEBUG5_LOW() { }

#define DEBUG1_PULSE() { }
#define DEBUG3_PULSE() { }
#define DEBUG4_PULSE() { }
#define DEBUG5_PULSE() { }

#define DEBUG1_TOGGLE() { }
#define DEBUG3_TOGGLE() { }
#define DEBUG4_TOGGLE() { }
#define DEBUG5_TOGGLE() { }

#define CONFIG_DEBUG_PINS() { }

/*
 * Sideband signals (for lack of another word)
 */
#define BT_CLK_REQ_PDIR     ( P1DIR )
#define BT_CLK_REQ_POUT     ( P1OUT )
#define BT_CLK_REQ_PIN      ( BIT4 )

#define BT_IO1_PDIR     ( P1DIR )
#define BT_IO1_POUT     ( P1OUT )
#define BT_IO1_PIN      ( BIT5 )

#define BT_IO2_PDIR     ( P1DIR )
#define BT_IO2_POUT     ( P1OUT )
#define BT_IO2_PIN      ( BIT6 )

/******************************************************************************/

/* peripheral mapping for accelerometer */
#define ACCELEROMETER_CTL0  ( UCB1CTL0 )
#define ACCELEROMETER_CTL1  ( UCB1CTL1 )
#define ACCELEROMETER_BR0   ( UCB1BR0 )
#define ACCELEROMETER_BR1   ( UCB1BR1 )
#define ACCELEROMETER_I2CSA ( UCB1I2CSA )
#define ACCELEROMETER_IE    ( UCB1IE )
#define ACCELEROMETER_IFG   ( UCB1IFG )
#define ACCELEROMETER_TXBUF ( UCB1TXBUF )
#define ACCELEROMETER_RXBUF ( UCB1RXBUF )
#define ACCELEROMETER_STAT  ( UCB1STAT )
/* interrupt mapping for accelerometer */
#define USCI_ACCELEROMETER_VECTOR ( USCI_B1_VECTOR )
#define USCI_ACCELEROMETER_IV     ( UCB1IV ) 

#endif // HAL_DIGITAL_V2_DEFS_H




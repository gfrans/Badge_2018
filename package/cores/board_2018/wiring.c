/*
  Copyright (c) 2015 Arduino LLC.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "Arduino.h"

#ifdef __cplusplus
extern "C" {
#endif


#if defined(__SAMD51__)
//CHANGE THIS IF YOU CHANGE THE CLOCK SPEED
uint32_t SystemCoreClock=120000000ul ;
#else
/*
 * System Core Clock is at 1MHz (8MHz/8) at Reset.
 * It is switched to 48MHz in the Reset Handler (startup.c)
 */
uint32_t SystemCoreClock=1000000ul ;
#endif

/*
void calibrateADC()
{
  volatile uint32_t valeur = 0;

  for(int i = 0; i < 5; ++i)
  {
    ADC->SWTRIG.bit.START = 1;
    while( ADC->INTFLAG.bit.RESRDY == 0 || ADC->STATUS.bit.SYNCBUSY == 1 )
    {
      // Waiting for a complete conversion and complete synchronization
    }

    valeur += ADC->RESULT.bit.RESULT;
  }

  valeur = valeur/5;
}*/

/*
 * Arduino Zero board initialization
 *
 * Good to know:
 *   - At reset, ResetHandler did the system clock configuration. Core is running at 48MHz.
 *   - Watchdog is disabled by default, unless someone plays with NVM User page
 *   - During reset, all PORT lines are configured as inputs with input buffers, output buffers and pull disabled.
 */
void init( void )
{
  // Set Systick to 1ms interval, common to all Cortex-M variants
  if ( SysTick_Config( SystemCoreClock / 1000 ) )
  {
    // Capture error
    while ( 1 ) ;
  }
  NVIC_SetPriority (SysTick_IRQn,  (1 << __NVIC_PRIO_BITS) - 2);  /* set Priority for Systick Interrupt (2nd lowest) */

  // Clock PORT for Digital I/O
//  PM->APBBMASK.reg |= PM_APBBMASK_PORT ;
//
//  // Clock EIC for I/O interrupts
//  PM->APBAMASK.reg |= PM_APBAMASK_EIC ;

#if defined(__SAMD51__)
  MCLK->APBAMASK.reg |= MCLK_APBAMASK_SERCOM0 | MCLK_APBAMASK_SERCOM1;
  
  MCLK->APBBMASK.reg |= MCLK_APBBMASK_SERCOM2 | MCLK_APBBMASK_SERCOM3 | MCLK_APBBMASK_TCC0 | MCLK_APBBMASK_TCC1 | MCLK_APBBMASK_TC3 | MCLK_APBBMASK_TC2;
  
  MCLK->APBCMASK.reg |= MCLK_APBCMASK_TCC2 | MCLK_APBCMASK_TC4 | MCLK_APBCMASK_TC5;
  
  MCLK->APBDMASK.reg |= MCLK_APBDMASK_DAC | MCLK_APBDMASK_SERCOM4 | MCLK_APBDMASK_SERCOM5 | MCLK_APBDMASK_ADC0 | MCLK_APBDMASK_ADC1;
#else
  // Clock SERCOM for Serial
  PM->APBCMASK.reg |= PM_APBCMASK_SERCOM0 | PM_APBCMASK_SERCOM1 | PM_APBCMASK_SERCOM2 | PM_APBCMASK_SERCOM3 | PM_APBCMASK_SERCOM4 | PM_APBCMASK_SERCOM5 ;

  // Clock TC/TCC for Pulse and Analog
  PM->APBCMASK.reg |= PM_APBCMASK_TCC0 | PM_APBCMASK_TCC1 | PM_APBCMASK_TCC2 | PM_APBCMASK_TC3 | PM_APBCMASK_TC4 | PM_APBCMASK_TC5 ;

  // ATSAMR, for example, doesn't have a DAC
  #ifdef PM_APBCMASK_DAC
   // Clock ADC/DAC for Analog
   PM->APBCMASK.reg |= PM_APBCMASK_ADC | PM_APBCMASK_DAC ;
  #endif
#endif

  // Setup all pins (digital and analog) in INPUT mode (default is nothing)
  for (uint32_t ul = 0 ; ul < NUM_DIGITAL_PINS ; ul++ )
  {
    pinMode( ul, INPUT ) ;
  }

  // Initialize Analog Controller
  // Setting clock
#if defined(__SAMD51__)
	GCLK->PCHCTRL[ADC0_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK1_Val | (1 << GCLK_PCHCTRL_CHEN_Pos); //use clock generator 1 (48Mhz)
	
	ADC0->CTRLA.bit.PRESCALER = ADC_CTRLA_PRESCALER_DIV256_Val;
	ADC0->CTRLB.bit.RESSEL = ADC_CTRLB_RESSEL_10BIT_Val;
	
	while( ADC0->SYNCBUSY.reg & ADC_SYNCBUSY_CTRLB );  //wait for sync
	
	ADC0->SAMPCTRL.reg = 0x3f;                        // Set max Sampling Time Length
	
	while( ADC0->SYNCBUSY.reg & ADC_SYNCBUSY_SAMPCTRL );  //wait for sync
	
	ADC0->INPUTCTRL.reg = ADC_INPUTCTRL_MUXNEG_GND;   // No Negative input (Internal Ground)
	
	while( ADC0->SYNCBUSY.reg & ADC_SYNCBUSY_INPUTCTRL );  //wait for sync
	
	// Averaging (see datasheet table in AVGCTRL register description)
	ADC0->AVGCTRL.reg = ADC_AVGCTRL_SAMPLENUM_1 |    // 1 sample only (no oversampling nor averaging)
						ADC_AVGCTRL_ADJRES(0x0ul);   // Adjusting result by 0
						
	while( ADC0->SYNCBUSY.reg & ADC_SYNCBUSY_AVGCTRL );  //wait for sync

	analogReference( AR_DEFAULT ) ; // Analog Reference is AREF pin (3.3v)
	
	GCLK->PCHCTRL[DAC_GCLK_ID].reg = GCLK_PCHCTRL_GEN_GCLK4_Val | (1 << GCLK_PCHCTRL_CHEN_Pos); //use clock generator 4 (12mhz)
	while (GCLK->PCHCTRL[DAC_GCLK_ID].bit.CHEN == 0);
	
	while ( DAC->SYNCBUSY.bit.SWRST == 1 ); // Wait for synchronization of registers between the clock domains
	DAC->CTRLA.bit.SWRST = 1;
	while ( DAC->SYNCBUSY.bit.SWRST == 1 ); // Wait for synchronization of registers between the clock domains
	
	DAC->CTRLB.reg = DAC_CTRLB_REFSEL_VREFPU; // TODO: fix this once silicon bug is fixed
	
	//set refresh rates
	DAC->DACCTRL[0].bit.REFRESH = 2;
	DAC->DACCTRL[1].bit.REFRESH = 2;

#else
  while(GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY);

  GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID( GCM_ADC ) | // Generic Clock ADC
                      GCLK_CLKCTRL_GEN_GCLK0     | // Generic Clock Generator 0 is source
                      GCLK_CLKCTRL_CLKEN ;

  while( ADC->STATUS.bit.SYNCBUSY == 1 );          // Wait for synchronization of registers between the clock domains

  ADC->CTRLB.reg = ADC_CTRLB_PRESCALER_DIV512 |    // Divide Clock by 512.
                   ADC_CTRLB_RESSEL_10BIT;         // 10 bits resolution as default

  ADC->SAMPCTRL.reg = 0x3f;                        // Set max Sampling Time Length

  while( ADC->STATUS.bit.SYNCBUSY == 1 );          // Wait for synchronization of registers between the clock domains

  ADC->INPUTCTRL.reg = ADC_INPUTCTRL_MUXNEG_GND;   // No Negative input (Internal Ground)

  // Averaging (see datasheet table in AVGCTRL register description)
  ADC->AVGCTRL.reg = ADC_AVGCTRL_SAMPLENUM_1 |    // 1 sample only (no oversampling nor averaging)
                     ADC_AVGCTRL_ADJRES(0x0ul);   // Adjusting result by 0

  analogReference( AR_DEFAULT ) ; // Analog Reference is AREF pin (3.3v)

  // Initialize DAC
  // Setting clock
  while ( GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY );
  GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID( GCM_DAC ) | // Generic Clock ADC
                      GCLK_CLKCTRL_GEN_GCLK0     | // Generic Clock Generator 0 is source
                      GCLK_CLKCTRL_CLKEN ;

 // ATSAMR, for example, doesn't have a DAC
 #ifdef DAC
  while ( DAC->STATUS.bit.SYNCBUSY == 1 ); // Wait for synchronization of registers between the clock domains
  DAC->CTRLB.reg = DAC_CTRLB_REFSEL_AVCC | // Using the 3.3V reference
                   DAC_CTRLB_EOEN ;        // External Output Enable (Vout)
 #endif


#endif //SAMD51
}

#ifdef __cplusplus
}
#endif
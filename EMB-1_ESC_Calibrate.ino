/*******************************************************************************
********************************************************************************
**
** File Name
** ---------
**
** EMB-1_ESC_Calibrate.ino
**
********************************************************************************
********************************************************************************
**
** Description
** -----------
**
** This module is able to calibrate an LC Racing EMB-1 Rally with original 
** transmitter/receiver.
** It connects directly to the ESC and when calibrating the ESC. 
** TODO: Describe the process of calibrating!
**
********************************************************************************
********************************************************************************
**                                                                            **
** COPYRIGHT NOTIFICATION (c) 2015    Johan Häggström.                        **
**                                                                            **
** This program is the property of Johan Häggström.                           **
** It may not be reproduced, distributed, or used without permission          **
** of an authorised company official.                                         **
**                                                                            **
********************************************************************************
********************************************************************************
*/

#include <TimerOne.h>


/*------------------------------------------------------------------------------
**
** Definitions
**
**------------------------------------------------------------------------------
*/

/*
** LED control definitions.
*/

#define WD_LED_CONTROL
#define LED_NBR_OF_LEDS              3

#define LED_WD                      0
#define LED_THR_MODE                1
#define LED_THR_MAX                 2

#define WATCHDOG_LED_PIN             13
#define LED_1_PIN                    12
#define LED_2_PIN                    11

#define LED_BLINK_05HZ_MASK          0x03FF
#define LED_BLINK_05HZ_LEN           20

#define LED_BLINK_1HZ_MASK           0x001F
#define LED_BLINK_1HZ_LEN            10

#define LED_BLINK_1_MASK             0x0001
#define LED_BLINK_1_LEN              9

#define LED_BLINK_2_MASK             0x0009
#define LED_BLINK_2_LEN              12

#define LED_BLINK_3_MASK             0x0049
#define LED_BLINK_3_LEN              15

#define LED_BLINK_4_MASK             0x0249
#define LED_BLINK_4_LEN              18

#define LED_BLINK_5_MASK             0x1249
#define LED_BLINK_5_LEN              21

#define LED_BLINK_ON_MASK            0xFFFF
#define LED_BLINK_ON_LEN             32

#define LED_BLINK_OFF_MASK           0x0000
#define LED_BLINK_OFF_LEN            32

/*
** Input push button definitions
*/

#define INPUT_1_PUSH_PIN              3
#define INPUT_2_PUSH_PIN              2

/*
** Output signal definitions
*/

#define OUTPUT_SIGNAL_PIN            9

/*
** The period is 20ms. For the PWM the period is divided into 1024 pices. 
** Resolution is thus, 19.5us/digit.
** Therefore, the PWM value in the following situations shold be:
** Neutral - 1400us - 72 PWM
** Forward - 1900us - 97 PWM (+25 pulses)
** Reverse - 1000us - 51 PWM
*/

#define NEUTRAL_PULSE        72
#define REVERSE_PULSE_100P   51

#define FORWARD_PULSE_50P    122
#define FORWARD_PULSE_60P    118
#define FORWARD_PULSE_70P    113
#define FORWARD_PULSE_80P    108
#define FORWARD_PULSE_90P    103
#define FORWARD_PULSE_100P   97



/*------------------------------------------------------------------------------
**
** Enumerations
**
**------------------------------------------------------------------------------
*/

typedef enum LedModeEnum
{
   LED_OFF      = 0,
   LED_ON,
   LED_05HZ,
   LED_1HZ,
   LED_1BLINK,
   LED_2BLINK,
   LED_3BLINK,
   LED_4BLINK,
   LED_5BLINK,
};


typedef enum ThrottlePosEnum
{
   THP_NEUTRAL,
   THP_FORWARD,
   THP_REVERSE
};

typedef enum ThrottleMaxEnum
{
   THMAX_50P,
   THMAX_60P,
   THMAX_70P,
   THMAX_80P,
   THMAX_90P,
   THMAX_100P,
};

typedef struct LedType
{
  int iPort;
  int iSeqLen;
  int iBitMask;
  int iWorkingSeq;
  int iWorkingBitMask;
};


/*------------------------------------------------------------------------------
**
** Global variables
**
**------------------------------------------------------------------------------
*/

LedType                WdLed;
LedType                axLedArray[ LED_NBR_OF_LEDS ];
ThrottlePosEnum        eThrottleMode;
ThrottleMaxEnum        eThrottleMax;
LedModeEnum            eLedModeDummy;


/*******************************************************************************
**
** Services
**
********************************************************************************
*/

/*------------------------------------------------------------------------------
** Timer1Cbf()
**------------------------------------------------------------------------------
*/

void Timer1Cbf()
{
   static int iCounter = 0;
   int i;
   
   
   /*
   ** This timer call-back function is called with the frequency of the PWM (which is 20ms).
   ** It should be called every 100ms, so let only every 5 call pass.
   */
   
   iCounter++;
   
   if( iCounter < 5 )
   {
      return;
   }
   else
   {
      iCounter = 0;
   }
   
   
   /*
   ** Update the LEDs
   */
   
   for( i=0; i< LED_NBR_OF_LEDS; i++ )
   {
      if( axLedArray[ i ].iWorkingSeq == 0 )
      {
         axLedArray[ i ].iWorkingSeq     = axLedArray[ i ].iSeqLen;
         axLedArray[ i ].iWorkingBitMask = axLedArray[ i ].iBitMask;
      }
      else
      {
         if( axLedArray[ i ].iWorkingBitMask & 0x01 )
         {
            digitalWrite( axLedArray[ i ].iPort, HIGH );
         }
         else
         {
            digitalWrite( axLedArray[ i ].iPort, LOW );
         }
      
         axLedArray[ i ].iWorkingBitMask = axLedArray[ i ].iWorkingBitMask >> 1;
         axLedArray[ i ].iWorkingSeq--;
      }
       
   } /* for( i=0; i< LED_NBR_OF_LEDS; i++ ) */
     
} /* end of Timer1Cbf() */


/*------------------------------------------------------------------------------
** setup()
**------------------------------------------------------------------------------
*/

void setup() 
{
   /*
   ** Timer-system setup
   ** 
   ** 100ms timer to drive the LED and state-machine
   **
   */

   Timer1.initialize( 100000 );         
   Timer1.attachInterrupt( Timer1Cbf );  
  

   /*
   ** LED Setup
   ** Pin 13 - Output WD LED
   ** Pin 12  - Output, Mode
   ** Pin 11  - Output, Mode 2
   **
   */

   pinMode( WATCHDOG_LED_PIN, OUTPUT );
   pinMode( LED_1_PIN, OUTPUT );
   pinMode( LED_2_PIN, OUTPUT );
  
   /*
   ** Push-button Setup
   ** Pin 2 - Input, Mode Select, Internal Pull-up
   ** Pin 3 - Input, Mode Select, Internal Pull-up
   ** Check what can be connected to IRQ
   **
   */

   pinMode( INPUT_1_PUSH_PIN, INPUT_PULLUP );
   pinMode( INPUT_2_PUSH_PIN, INPUT_PULLUP );

   /*
   ** Output signal setup
   ** PWM output on PORTB 1, Pin D9.
   */
  
   Timer1.setPeriod( 20000 );    
 

   /* Set initial values to the LEDs */
   axLedArray[ LED_WD ].iPort       = WATCHDOG_LED_PIN;
   axLedArray[ LED_THR_MODE ].iPort = LED_1_PIN;
   axLedArray[ LED_THR_MAX ].iPort  = LED_2_PIN;

   SetLEDIndication( LED_WD, LED_1HZ );
   SetLEDIndication( LED_THR_MODE, LED_OFF );
   SetLEDIndication( LED_THR_MAX, LED_OFF );

   /*
   ** Setup default values for state machine.
   */
  
   eThrottleMode  = THP_NEUTRAL;
   eThrottleMax   = THMAX_50P;
   
   /*
   ** ...and run it once to set proper outputs
   */
   
   RunStateMachine();

} /* end of setup() */


/*------------------------------------------------------------------------------
** SetLEDIndication()
**------------------------------------------------------------------------------
*/

void SetLEDIndication( int iLed, int eMode )
{
   /*
   ** Set specified LED indication
   */
   
   if( iLed >= LED_NBR_OF_LEDS )
   {
      return;
   }
  
   switch( eMode )
   {
   case LED_OFF:  
      axLedArray[ iLed ].iSeqLen       = LED_BLINK_OFF_LEN;
      axLedArray[ iLed ].iBitMask      = LED_BLINK_OFF_MASK;
      axLedArray[ iLed ].iWorkingSeq = 0;
      break;
      
   case LED_ON:
      axLedArray[ iLed ].iSeqLen       = LED_BLINK_ON_LEN;
      axLedArray[ iLed ].iBitMask      = LED_BLINK_ON_MASK;
      axLedArray[ iLed ].iWorkingSeq = 0;
      break;
      
   case LED_05HZ:
      axLedArray[ iLed ].iSeqLen       = LED_BLINK_05HZ_LEN;
      axLedArray[ iLed ].iBitMask      = LED_BLINK_05HZ_MASK;
      axLedArray[ iLed ].iWorkingSeq = 0;
      break;
      
   case LED_1HZ:
      axLedArray[ iLed ].iSeqLen       = LED_BLINK_1HZ_LEN;
      axLedArray[ iLed ].iBitMask      = LED_BLINK_1HZ_MASK;
      axLedArray[ iLed ].iWorkingSeq = 0;
      break;
      
   case LED_1BLINK:
      axLedArray[ iLed ].iSeqLen       = LED_BLINK_1_LEN;
      axLedArray[ iLed ].iBitMask      = LED_BLINK_1_MASK;
      axLedArray[ iLed ].iWorkingSeq = 0;
      break;
      
   case LED_2BLINK:
      axLedArray[ iLed ].iSeqLen       = LED_BLINK_2_LEN;
      axLedArray[ iLed ].iBitMask      = LED_BLINK_2_MASK;
      axLedArray[ iLed ].iWorkingSeq = 0;
      break;
      
   case LED_3BLINK:
      axLedArray[ iLed ].iSeqLen       = LED_BLINK_3_LEN;
      axLedArray[ iLed ].iBitMask      = LED_BLINK_3_MASK;
      axLedArray[ iLed ].iWorkingSeq = 0;
      break;
      
   case LED_4BLINK:
      axLedArray[ iLed ].iSeqLen       = LED_BLINK_4_LEN;
      axLedArray[ iLed ].iBitMask      = LED_BLINK_4_MASK;
      axLedArray[ iLed ].iWorkingSeq = 0;
      break;
      
   case LED_5BLINK:
      axLedArray[ iLed ].iSeqLen       = LED_BLINK_5_LEN;
      axLedArray[ iLed ].iBitMask      = LED_BLINK_5_MASK;
      axLedArray[ iLed ].iWorkingSeq = 0;
      break;
      
   } /* end of switch( eMode ) */
   
} /* end of SetLEDIndication()*/


/*------------------------------------------------------------------------------
** ThrottleModePushButtonPress()
**------------------------------------------------------------------------------
*/

void ThrottleModePushButtonPress()
{
   /*
   ** User has pressed the Throttle mode button. Step in the 
   ** state machine and output correct pulse length.
   */  
   
   switch( eThrottleMode )
   {
   case THP_NEUTRAL:
      eThrottleMode = THP_FORWARD;
      break;
     
   case THP_FORWARD:
      eThrottleMode = THP_REVERSE;
      break;
      
   case THP_REVERSE:
      eThrottleMode = THP_NEUTRAL;
      break;
      
   } /* end switch( eThrottleMode ) */
   
} /* end of ThrottleModePushButtonPress() */


/*------------------------------------------------------------------------------
** ThrottleMaxPushButtonPress()
**------------------------------------------------------------------------------
*/

void ThrottleMaxPushButtonPress()
{
   /*
   ** User has pressed the Throttle max button. Step in the 
   ** state machine.
   */  
   
   switch( eThrottleMax )
   {
   case THMAX_50P:
      eThrottleMax = THMAX_60P;
      break;
     
   case THMAX_60P:
      eThrottleMax = THMAX_70P;
      break;
      
   case THMAX_70P:
      eThrottleMax = THMAX_80P;
      break;
      
   case THMAX_80P:
      eThrottleMax = THMAX_90P;
      break;
      
   case THMAX_90P:
      eThrottleMax = THMAX_100P;
      break;
      
   case THMAX_100P:
      eThrottleMax = THMAX_50P;
      break;
      
   } /* end switch( eThrottleMax ) */
  
} /* end of ThrottleMaxPushButtonPress() */


/*------------------------------------------------------------------------------
** RunStateMachine()
**------------------------------------------------------------------------------
*/

void RunStateMachine()
{
   /*
   ** Set LED output for Throttle position.
   */
   
   switch( eThrottleMode )
   {
   case THP_NEUTRAL:
      SetLEDIndication( LED_THR_MODE, LED_1BLINK ); 
      break;
     
   case THP_FORWARD:
      SetLEDIndication( LED_THR_MODE, LED_2BLINK );
      break;
      
   case THP_REVERSE:
      SetLEDIndication( LED_THR_MODE, LED_3BLINK );
      break;
      
   default:
      /*
      ** What?! We should not end up HERE.
      */
      break;
      
   } /* end switch( eThrottleMode ) */
   
   /*
   ** Set LED output for Throttle max selection.
   */
   
   switch( eThrottleMax )
   {
   case THMAX_50P:
      SetLEDIndication( LED_THR_MAX, LED_1BLINK ); 
      break;
     
   case THMAX_60P:
      SetLEDIndication( LED_THR_MAX, LED_2BLINK );
      break;
      
   case THMAX_70P:
      SetLEDIndication( LED_THR_MAX, LED_3BLINK ); 
      break;
      
   case THMAX_80P:
      SetLEDIndication( LED_THR_MAX, LED_4BLINK );
      break;
      
   case THMAX_90P:
      SetLEDIndication( LED_THR_MAX, LED_5BLINK );
      break;
      
   case THMAX_100P:
      SetLEDIndication( LED_THR_MAX, LED_ON );
      break;
      
   } /* end switch( eThrottleMax ) */
   
   
   /*
   ** Set the PWM output depending on current Throttle position and max value.
   */
   
   if( eThrottleMode == THP_NEUTRAL )
   {
      Timer1.pwm( OUTPUT_SIGNAL_PIN, NEUTRAL_PULSE );
   }
   else if( eThrottleMode == THP_REVERSE )
   {
      Timer1.pwm( OUTPUT_SIGNAL_PIN, REVERSE_PULSE_100P );
   }
   else
   {
      switch( eThrottleMax )
      {
      case THMAX_50P:
         Timer1.pwm( OUTPUT_SIGNAL_PIN, FORWARD_PULSE_50P );
         break;
     
      case THMAX_60P:
         Timer1.pwm( OUTPUT_SIGNAL_PIN, FORWARD_PULSE_60P ); 
         break;
      
      case THMAX_70P:
         Timer1.pwm( OUTPUT_SIGNAL_PIN, FORWARD_PULSE_70P );
         break;
      
      case THMAX_80P:
         Timer1.pwm( OUTPUT_SIGNAL_PIN, FORWARD_PULSE_80P );
         break;
      
      case THMAX_90P:
         Timer1.pwm( OUTPUT_SIGNAL_PIN, FORWARD_PULSE_90P );
         break;
      
      case THMAX_100P:
         Timer1.pwm( OUTPUT_SIGNAL_PIN, FORWARD_PULSE_100P ); 
         break;
      
      } /* end switch( eThrottleMax ) */
     
   } /* end if( ... ) */
   
   
} /* end of RunStateMachine */


/*------------------------------------------------------------------------------
** loop()
**------------------------------------------------------------------------------
*/

void loop() 
{
  static int iThrottleModeLastValue = 1;
  static int iThrottleMaxLastValue = 1;
  bool   fUpdate;

   while( 1 )
   {
      fUpdate = false;
      
      /*
      ** Sample throttle position button.
      */ 
      
      if( digitalRead( INPUT_1_PUSH_PIN ) == LOW ) 
      {
         /*
         ** Wait for the user to release the button.
         */
        
         while( digitalRead( INPUT_1_PUSH_PIN ) == LOW );
        
         /*
         ** Update the throttle position.
         */
        
         ThrottleModePushButtonPress();
         
         fUpdate = true;
        
      } /* end if if( digitalRead( INPUT_1_PUSH_PIN ) == LOW ) */
     
     
      if( digitalRead( INPUT_2_PUSH_PIN ) == LOW ) 
      {
         /*
         ** Wait for the user to release the button.
         */
        
         while( digitalRead( INPUT_2_PUSH_PIN ) == LOW );
        
        
         /*
         ** Update the Throttle maximal value.
         */
        
         ThrottleMaxPushButtonPress();
         
         fUpdate = true;
         
      } /* end if( digitalRead( INPUT_2_PUSH_PIN ) == LOW ) */ 
      
      
      if( fUpdate )
      {
         /*
         ** Actualize the changes by executing the state-machine.
         */
         
         RunStateMachine();
      }
      
      delay( 100 );
    
   } /* end while( 1 ) */

} /* end of loop() */

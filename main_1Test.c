/*
 * File name            : Main.c
 * Compiler             : MPLAB XC8/ MPLAB C18 compiler
 * IDE                  : Microchip  MPLAB X IDE v5.25
 * Processor            : PIC18F66K40
 * Author               : Bhoomi Jalasandharan
 * Created on           : July 15, 2020, 05:23 PM
 * Description          : Main source file
 */

/***************************** Header file declarations#start ************************/

#include "congfigBits.h"
#include "variableDefinitions.h"
#include "controllerActions.h"
#include "eeprom.h"
#include "gsm.h"
#ifdef DEBUG_MODE_ON_H
#include "serialMonitor.h"
#endif
/***************************** Header file declarations#end **************************/

//**************interrupt service routine handler#start***********//

/*************************************************************************************************************************

This function is called when an interrupt has occurred at RX pin of ?c which is connected to TX pin of GSM.
Interrupt occurs at 1st cycle of each Data byte.
The purpose of this interrupt handler is to store the data received from GSM into Array called gsmResponse[]
Each response from GSM starts with '+' symbol, e.g. +CMTI: "SM", <index>
The End of SMS is detected by OK command.

 **************************************************************************************************************************/

void __interrupt(high_priority)rxANDiocInterrupt_handler(void) {
    // Interrupt on RX bit
    if (PIR4bits.RC3IF) {
        Run_led = GLOW; // Led Indication for system in Operational Mode
        rxCharacter = rxByte(); // Read byte received at Reception Register
        // Check if any overrun occur due to continuous reception
        if (RC3STAbits.OERR) {
            RC3STAbits.CREN = 0;
            Nop();
            RC3STAbits.CREN = 1;
        }
        // If interrupt occurred in sleep mode due to command from GSM
        if (inSleepMode) {
            SIM_led = GLOW;  // Led Indication for GSM interrupt in sleep mode 
            //sleepCount = 2; // Set minimum sleep to 2 for recursive reading from GSM
            //sleepCountChangedDueToInterrupt = true; // Sleep count is altered and hence needs to be read from memory
            // check if GSM initiated communication with '+'
            if (rxCharacter == '+') {
                msgIndex = CLEAR; // Reset message storage index to first character to start reading from '+'
                gsmResponse[msgIndex] = rxCharacter; // Load Received byte into storage buffer
                msgIndex++; // point to next location for storing next received byte
            }
            // Check if Sms type cmd is initiated and received byte is cmti command
            else if (msgIndex < 12 && cmti[msgIndex] == rxCharacter) {
                gsmResponse[msgIndex] = rxCharacter; // Load received byte into storage buffer
                msgIndex++; // point to next location for storing next received byte
                // check if storage index is reached to last character of CMTI command
                if (msgIndex == 12) {
                    cmtiCmd= true; // Set to indicate cmti command received	
                }
            } 
            //To extract sim location for SMS storage
            else if (cmtiCmd && msgIndex == 12) {
                cmtiCmd= false; // reset for next cmti command reception	
                temporaryBytesArray[0] = rxCharacter; // To store sim memory location of received message
                msgIndex = CLEAR;
                newSMSRcvd = true; // Set to indicate New SMS is Received
                //sleepCount = 1;
                //sleepCountChangedDueToInterrupt = true; // Sleep count is altered and hence needs to be calculated again
            }
        } 
        // check if GSM response to �c command is not completed
        else if (!controllerCommandExecuted) {
            SIM_led = GLOW;  // Led Indication for GSM interrupt in operational mode
            // Start storing response if received data is '+' at index zero
            if (rxCharacter == '+' && msgIndex == 0) {
                gsmResponse[msgIndex] = rxCharacter; // Load received byte into storage buffer
                msgIndex++; // point to next location for storing next received byte
            }
            // Cascade received data to stored response after receiving first character '+'
            else if (msgIndex > 0 && msgIndex <=220) {
                gsmResponse[msgIndex] = rxCharacter; // Load received byte into storage buffer
                // Cascade till 'OK'  is found
                
                if (gsmResponse[msgIndex - 1] == 'O' && gsmResponse[msgIndex] == 'K') {
                    controllerCommandExecuted = true; // GSM response to �c command is completed
                    msgIndex = CLEAR; // Reset message storage index to first character to start reading for next received byte of cmd
                } 
                // Read bytes till 500 characters
                else if (msgIndex <= 220) {
                    msgIndex++;
                }
            }
        }
        SIM_led = DARK;  // Led Indication for GSM interrupt is done 
        PIR4bits.RC3IF= CLEAR; // Reset the ISR flag.
    } // end RX interrupt
    //Interrupt-on-change pins
    else if (PIR0bits.IOCIF) {
        Run_led = GLOW; // Led Indication for system in Operational Mode
        // Rising Edge -- All phase present
        if ((IOCEF5 == 1 || IOCEF6 == 1 || IOCEF7 == 1)) {
            myMsDelay(5000);
            if (phaseB == 0 && phaseY == 0 &&  phaseR == 0) {
                //phase is on
                IOCEF &= (IOCEF ^ 0xFF); //Clearing Interrupt Flags
                phaseFailureDetected = false;
                setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
                myMsDelay(5000);
                RESET();
            }
            else {
                // phase is out
                IOCEF &= (IOCEF ^ 0xFF); //Clearing Interrupt Flags
                phaseFailureDetected = false; //true
                phaseFailureActionTaken = false;
            }
        }
        PIR0bits.IOCIF = CLEAR; // Reset the ISR flag.
    }          
}


/*************************************************************************************************************************

This function is called when an interrupt is occurred after 16 bit timer is overflow
The purpose of this interrupt handler is to count no. of overflows that the timer did.

 **************************************************************************************************************************/

void __interrupt(low_priority) timerInterrupt_handler(void) {
    /*To follow filtration  cycle sequence*/
    if (PIR0bits.TMR0IF) {
        Run_led = GLOW; // Led Indication for system in Operational Mode
        PIR0bits.TMR0IF = CLEAR;
        TMR0H = 0xE3; // Load Timer0 Register Higher Byte 
        TMR0L = 0xB0; // Load Timer0 Register Lower Byte
        Timer0Overflow++;
        // Control sleep count decrement for each one minute interrupt when Motor is ON i.e. Valve ON period 
        if (sleepCount > 0 && MotorControl == ON) {
            sleepCount--;
            if (dryRunCheckCount == 0 || dryRunCheckCount < 3) {
                dryRunCheckCount++;
            }
        }
        /*
        // Check Fertigation Level for each one minute interrupt when Fertigation Motor is ON during Valve ON period 
        if (fertigationValveControl == ON) {
           fertigationDry = false;
            if (!moistureSensorFailed) {  // to avoid repeated fertigation level check after sensor failure detected
                if (isFieldMoistureSensorWet(11)==false) {
                    if (!moistureSensorFailed) { // to avoid sensor dry detection due to sensor failure
                        fertigationValveControl = OFF;
                        fertigationDry = true;
                    }
                }
            }
        }
        */
        if (!fieldValve[8].isConfigured && !fieldValve[9].isConfigured && !fieldValve[10].isConfigured && !fieldValve[11].isConfigured) { // 9-12 used as injector
            //To follow fertigation cycle sequence
            if (fertigationValveControl == ON) {
                if (injector1Control == ON) {
                    if(injector1OnPeriodCnt == injector1OnPeriod) {
                        injector1Control = OFF;
                        injector1OnPeriodCnt = CLEAR;
                        injector1OffPeriodCnt++;
                        injector1CycleCnt++;
                    }
                    else injector1OnPeriodCnt++;
                }
                else if (injector1Control == OFF) {
                    if(injector1OffPeriodCnt == injector1OffPeriod) {
                        if (injector1CycleCnt < injector1Cycle) {
                            injector1Control = ON;
                            injector1OnPeriodCnt++;
                            injector1OffPeriodCnt = CLEAR;
                        }
                        else injector1OffPeriodCnt = injector1OffPeriod + 1;
                    }
                    else injector1OffPeriodCnt++;
                }
                if (injector2Control == ON) {
                    if(injector2OnPeriodCnt == injector2OnPeriod) {
                        injector2Control = OFF;
                        injector2OnPeriodCnt = CLEAR;
                        injector2OffPeriodCnt++;
                        injector2CycleCnt++;
                    }
                    else injector2OnPeriodCnt++;
                }
                else if (injector2Control == OFF) {
                    if(injector2OffPeriodCnt == injector2OffPeriod) {
                        if (injector2CycleCnt < injector2Cycle) {
                            injector2Control = ON;
                            injector2OnPeriodCnt++;
                            injector2OffPeriodCnt = CLEAR;
                        }
                        else injector2OffPeriodCnt = injector2OffPeriod + 1;
                    }
                    else injector2OffPeriodCnt++;
                }
                if (injector3Control == ON) {
                    if(injector3OnPeriodCnt == injector3OnPeriod) {
                        injector3Control = OFF;
                        injector3OnPeriodCnt = CLEAR;
                        injector3OffPeriodCnt++;
                        injector3CycleCnt++;
                    }
                    else injector3OnPeriodCnt++;
                }
                else if (injector3Control == OFF) {
                    if(injector3OffPeriodCnt == injector3OffPeriod) {
                        if (injector3CycleCnt < injector3Cycle) {
                            injector3Control = ON;
                            injector3OnPeriodCnt++;
                            injector3OffPeriodCnt = CLEAR;
                        }
                        else injector3OffPeriodCnt = injector3OffPeriod + 1;
                    }
                    else injector3OffPeriodCnt++;
                }
                if (injector4Control == ON) {
                    if(injector4OnPeriodCnt == injector4OnPeriod) {
                        injector4Control = OFF;
                        injector4OnPeriodCnt = CLEAR;
                        injector4OffPeriodCnt++;
                        injector4CycleCnt++;
                    }
                    else injector4OnPeriodCnt++;
                }
                else if (injector4Control == OFF) {
                    if(injector4OffPeriodCnt == injector4OffPeriod) {
                        if (injector4CycleCnt < injector4Cycle) {
                            injector4Control = ON;
                            injector4OnPeriodCnt++;
                            injector4OffPeriodCnt = CLEAR;
                        }
                        else injector4OffPeriodCnt = injector4OffPeriod + 1;
                    }
                    else injector4OffPeriodCnt++;
                }
            }
        }
        //*To follow filtration  cycle sequence*/
        if (filtrationCycleSequence == 99) {    // Filtration is disabled
            Timer0Overflow = 0;
        }
        else if (filtrationCycleSequence == 1 && Timer0Overflow == filtrationDelay1 ) { // Filtration1 Start Delay
            Timer0Overflow = 0;
            filtration1ValveControl = ON;
            filtrationCycleSequence = 2;
        }
        else if (filtrationCycleSequence == 2 && Timer0Overflow == filtrationOnTime ) {  // Filtration1 On Period
            Timer0Overflow = 0;
            filtration1ValveControl = OFF;
            filtrationCycleSequence = 3;
        }
        else if (filtrationCycleSequence == 3 && Timer0Overflow == filtrationDelay2 ) { // Filtration2 Start Delay
            Timer0Overflow = 0;
            filtration2ValveControl = ON;
            filtrationCycleSequence = 4;
        }
        else if (filtrationCycleSequence == 4 && Timer0Overflow == filtrationOnTime ) { // Filtration2 On Period
            Timer0Overflow = 0;
            filtration2ValveControl = OFF;
            filtrationCycleSequence = 5;
        }
        else if (filtrationCycleSequence == 5 && Timer0Overflow == filtrationDelay2 ) { // Filtration3 Start Delay
            Timer0Overflow = 0;
            filtration3ValveControl = ON;
            filtrationCycleSequence = 6;
        }
        else if (filtrationCycleSequence == 6 && Timer0Overflow == filtrationOnTime ) { // Filtration3 On Period
            Timer0Overflow = 0;
            filtration3ValveControl = OFF;
            filtrationCycleSequence = 7;
        }
        else if (filtrationCycleSequence == 7 && Timer0Overflow == filtrationSeperationTime ) { //Filtration Repeat Delay
            Timer0Overflow = 0;
            filtrationCycleSequence = 1;
        }
    }
/*To measure pulse width of moisture sensor output*/
    if (PIR5bits.TMR1IF) {
        Run_led = GLOW; // Led Indication for system in Operational Mode
        Timer1Overflow++;
        PIR5bits.TMR1IF = CLEAR;
    }

    if (PIR5bits.TMR3IF) {
        Run_led = GLOW; // Led Indication for system in Operational Mode
        PIR5bits.TMR3IF = CLEAR;
        TMR3H = 0xF0; // Load Timer3 Register Higher Byte 
        TMR3L = 0xDC; // Load Timer3 Register lower Byte 
        Timer3Overflow++;
        
        if (Timer3Overflow > timer3Count  && !controllerCommandExecuted) {
            controllerCommandExecuted = true; // Unlock key
            Timer3Overflow = 0;
            T3CONbits.TMR3ON = OFF; // Stop timer
            if(checkMoistureSensor) {
                moistureSensorFailed = true;
            }    
        } 
        else if (controllerCommandExecuted) {
            Timer3Overflow = 0;
            T3CONbits.TMR3ON= OFF; // Stop timer
        }       
    }
}
//**************interrupt service routine handler#end***********//



//****************************MAIN FUNCTION#Start***************************************//
 void main(void) {
    NOP();
    NOP();
    NOP();
    //unsigned char last_Field_No = CLEAR;
    actionsOnSystemReset();
    while (1) {
nxtVlv: if (!valveDue && !phaseFailureDetected && !lowPhaseCurrentDetected) {
            wetSensor = false; // reset wet sensor for first wet field detection
            myMsDelay(50);
            scanValveScheduleAndGetSleepCount(); // get sleep count for next valve action
            myMsDelay(50);
            dueValveChecked = true;
        }
        if (valveDue && dueValveChecked) {
            #ifdef DEBUG_MODE_ON_H
            //********Debug log#start************//
            transmitStringToDebug("actionsOnDueValve_IN\r\n");
            //********Debug log#end**************//
            #endif
            dueValveChecked = false;
            /************************Debug************************/
            // for field no. 01 to 09
            if (fieldList[0]<9){
                temporaryBytesArray[0] = 48; // To store field no. of valve in action 
                temporaryBytesArray[1] = fieldList[0] + 49; // To store field no. of valve in action 
            }// for field no. 10 to 12
            else if (fieldList[0] > 8 && fieldList[0] < 12) {
                temporaryBytesArray[0] = 49; // To store field no. of valve in action 
                temporaryBytesArray[1] = fieldList[0] + 39; // To store field no. of valve in action 
            }
            sendSms(SmsKey2, userMobileNo, fieldNoRequired); // Acknowledge user about successful Fertigation stopped action
            /************************Debug************************/
            actionsOnDueValve(fieldList[0]);// Copy field no. navigated through iterator. 
            #ifdef DEBUG_MODE_ON_H
            //********Debug log#start************//
            transmitStringToDebug("actionsOnDueValve_OUT\r\n");
            //********Debug log#end**************//
            #endif
        }
        // DeActivate last valve and switch off motor pump
        else if (valveExecuted) {
            wetSensor = false; // reset wet sensor for last wet field detection
            powerOffMotor();
            iterator = 0;
            while(lastFieldList[iterator] != 255 && iterator < fieldCount) {
                deActivateValve(lastFieldList[iterator]); // Successful Deactivate valve for not matched valve list
                myMsDelay(100);
                iterator++;
            }
            // Reset Active Valve list
            iterator = 0;
            while(iterator < fieldCount) {
                fieldList[iterator] = 255;
                iterator++;
            }
            /***************************/
            sendSms(SmsIrr5, userMobileNo, commonInActiveFieldNoRequired); // Acknowledge user about successful Irrigation stopped action
            #ifdef SMS_DELIVERY_REPORT_ON_H
            sleepCount = 2; // Load sleep count for SMS transmission action
            sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
            setBCDdigit(0x05,0);
            deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
            setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
            #endif
            /***************************/
            // Reset De-active Valve list
            iterator = 0;
            while(iterator < fieldCount) {
                lastFieldList[iterator] = 255;
                iterator++;
            }
            //last_Field_No = readFieldIrrigationValveNoFromEeprom();
            //deActivateValve(last_Field_No);      // Successful Deactivate valve
            valveExecuted = false;
            /***************************/
            sendSms(SmsMotor1, userMobileNo, noInfo); // Acknowledge user about successful action
            #ifdef SMS_DELIVERY_REPORT_ON_H
            sleepCount = 2; // Load sleep count for SMS transmission action
            sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
            setBCDdigit(0x05,0);
            deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
            setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
            #endif
            /***************************/
            //startFieldNo = 0;
            //goto nxtVlv;
        }
        // system on hold
        if (onHold) {
            /******************Debug****************/
            sendSms(SmsKey1, userMobileNo, noInfo);
            /******************Debug****************/
            sleepCount = 0; // Skip Next sleep for performing hold operation
        }
        if (!wetSensor) {
            /****************************/
            deepSleep(); // sleep for given sleep count (	default/calculated )
            /****************************/
            // check if Sleep count executed with interrupt occurred due to new SMS command reception
            #ifdef DEBUG_MODE_ON_H
            //********Debug log#start************//
            transmitStringToDebug((const char *)gsmResponse);
            transmitStringToDebug("\r\n");
            //********Debug log#end**************//
            #endif
            if (newSMSRcvd) {
                #ifdef DEBUG_MODE_ON_H
                //********Debug log#start************//
                transmitStringToDebug("newSMSRcvd_IN\r\n");
                //********Debug log#end**************//
                #endif
                setBCDdigit(0x02,1); // "2" BCD indication for New SMS Received 
                myMsDelay(500);
                newSMSRcvd = false; // received command is processed
                /******************Debug*****************/
                strncpy(decodedString,"HOLD01",6);
                /******************Debug*****************/
                extractReceivedSms(); // Read received SMS
                setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
                myMsDelay(500);
                /******************Debug*****************/
                //deleteMsgFromSIMStorage();
                /******************Debug*****************/
                #ifdef DEBUG_MODE_ON_H
                //********Debug log#start************//
                transmitStringToDebug("newSMSRcvd_OUT\r\n");
                //********Debug log#end**************//
                #endif
            } 
            //check if Sleep count executed without external interrupt
            else {
                #ifdef DEBUG_MODE_ON_H
                //********Debug log#start************//
                transmitStringToDebug("actionsOnSleepCountFinish_IN\r\n");
                //********Debug log#end**************//
                #endif
                actionsOnSleepCountFinish();
                #ifdef DEBUG_MODE_ON_H
                //********Debug log#start************//
                transmitStringToDebug("actionsOnSleepCountFinish_OUT\r\n");
                //********Debug log#end**************//
                #endif
                if (isRTCBatteryDrained() && !rtcBatteryLevelChecked){
                    /***************************/
                    sendSms(SmsRTC1, userMobileNo, noInfo); // Acknowledge user about replace RTC battery
                    rtcBatteryLevelChecked = true;
                    #ifdef SMS_DELIVERY_REPORT_ON_H
                    sleepCount = 2; // Load sleep count for SMS transmission action
                    sleepCountChangedDueToInterrupt = true; // Sleep count needs to read from memory after SMS transmission
                    setBCDdigit(0x05,0);
                    deepSleep(); // Sleep until message transmission acknowledge SMS is received from service provider
                    setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
                    #endif
                    /***************************/
                }
            }
        }
    }
}
//****************************MAIN FUNCTION#End***************************************//
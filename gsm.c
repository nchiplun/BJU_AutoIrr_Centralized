/*
 * File name            : gsm.c
 * Compiler             : MPLAB XC8/ MPLAB C18 compiler
 * IDE                  : Microchip  MPLAB X IDE v5.25
 * Processor            : PIC18F66K40
 * Author               : Bhoomi Jalasandharan
 * Created on           : July 15, 2020, 05:23 PM
 * Description          : uart functions definitions source file
 */

#include "congfigBits.h"
#include "variableDefinitions.h"
#include "controllerActions.h"
#include "gsm.h"
#ifdef DEBUG_MODE_ON_H
#include "serialMonitor.h"
#endif					  


//*****************Serial communication function_Start****************//

/*************************************************************************************************************************

This function is called to receive Byte data from GSM
The purpose of this function is to return Data loaded into Reception buffer (RCREG) until Receive flag (RCIF) is pulled down

 **************************************************************************************************************************/
unsigned char rxByte(void) {
    while (PIR4bits.RC3IF == CLEAR); // Wait until RCIF gets low
    // ADD indication if infinite
    return RC3REG; // Return data stored in the Reception register
}

/*************************************************************************************************************************

This function is called to transmit Byte data to GSM
The purpose of this function is to transmit Data loaded into Transmit buffer (TXREG) until Transmit flag (TXIF) is pulled down

 **************************************************************************************************************************/
// Transmit data through TX pin
void txByte(unsigned char serialData) {
    TX3REG = serialData; // Load Transmit Register 
    while (PIR4bits.TX3IF == CLEAR); // Wait until TXIF gets low
    // ADD indication if infinite
}

/*************************************************************************************************************************

This function is called to transmit data to GSM in string format
The purpose of this function is to call transmit Byte data (txByte) Method until the string register reaches null.

 **************************************************************************************************************************/
void transmitStringToGSM(const char *string) {
    // Until it reaches null
    while (*string) {
        txByte(*string++); // Transmit Byte Data
        myMsDelay(5);  // Delay reduced to 5 from 50
    }
}

/*************************************************************************************************************************

This function is called to transmit data to GSM in Number format
The purpose of this function is to call transmit Byte data (txByte) Method until mentioned index.

 **************************************************************************************************************************/
void transmitNumberToGSM(unsigned char *number, unsigned char index) {
    unsigned char j = CLEAR;
    // Until it reaches index no.
    while (j < index) {
        txByte(*number++); // Transmit Byte Data
        myMsDelay(5); // Delay reduced to 5 from 50
        j++;
    }
}

/*************************************************************************************************************************

This function is called to enable receive mode of GSM module.
The purpose of this function is to transmit AT commands which enables Receive mode of GSM module in Text mode

 **************************************************************************************************************************/
void configureGSM(void) {
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("configureGSM_IN\r\n");
    //********Debug log#end**************//
#endif
    timer3Count = 15; // 15 sec window
    setBCDdigit(0x0A,0); // (c.) BCD indication for configureGSM
    controllerCommandExecuted = false;
    msgIndex = 1;
    T3CONbits.TMR3ON = ON; // Start timer thread to unlock system if GSM fails to respond within 15 sec
    while (!controllerCommandExecuted) {
        transmitStringToGSM("ATE0\r\n"); // Echo off command
        myMsDelay(500);
    }
    PIR5bits.TMR3IF = SET; //Stop timer thread
    controllerCommandExecuted = false;
    msgIndex = 1;
    T3CONbits.TMR3ON = ON; // Start timer thread to unlock system if GSM fails to respond within 15 sec
    while (!controllerCommandExecuted) {
        transmitStringToGSM("AT+CMGF=1\r\n"); // Text Mode command
        myMsDelay(500);
    }
    PIR5bits.TMR3IF = SET;
    controllerCommandExecuted = false;
    msgIndex = 1;
    T3CONbits.TMR3ON = ON; // Start timer thread to unlock system if GSM fails to respond within 15 sec
    while (!controllerCommandExecuted) {
        transmitStringToGSM("AT+CNMI=1,1,0,0,0\r\n"); // enable new sms message indication
        myMsDelay(500);
    }
    PIR5bits.TMR3IF = SET;
    controllerCommandExecuted = false;
    msgIndex = 1;
    T3CONbits.TMR3ON = ON; // Start timer thread to unlock system if GSM fails to respond within 15 sec
    while (!controllerCommandExecuted) {
        transmitStringToGSM("AT+SCLASS0=1\r\n"); // Store class 0 SMS to SIM memory when received class 0 SMS
        myMsDelay(500);
    }
    PIR5bits.TMR3IF = SET;
    controllerCommandExecuted = false;
    msgIndex = 1;
    T3CONbits.TMR3ON = ON; // Start timer thread to unlock system if GSM fails to respond within 15 sec
    while (!controllerCommandExecuted) {
        transmitStringToGSM("AT+CSCS=\"GSM\"\r\n"); // send to GSM
        myMsDelay(500);
    }
    PIR5bits.TMR3IF = SET;
    setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("configureGSM_OUT\r\n");
    //********Debug log#end**************//
#endif
}

/*************************************************************************************************************************

This function is called to check connection between C and GSM  
The purpose of this function is to reset GSM until GSM responds OK to AT command.

 **************************************************************************************************************************/
/*
void checkGsmConnection(void) {
    timer3Count = 15;  // 15 sec window
    setBCDdigit(0x0A,0);  // (c.) BCD indication for checkGsmConnection
    controllerCommandExecuted = false;
    msgIndex = 1;
    T3CONbits.TMR3ON = ON; // Start timer thread to unlock system if GSM fails to respond within 15 sec
    while (!controllerCommandExecuted) {
        transmitStringToGSM("AT\r\n"); // Echo ON command
        myMsDelay(500);
    }
    PIR5bits.TMR3IF = SET; //Stop timer thread
    setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
}
*/
//************Set GSM at local time standard across the globe***********//

/*************************************************************************************************************************

This function is called to set GSm at Local time zone 
The purpose of this function is to send AT commands to GSM in order to set it at Local time zone.

 **************************************************************************************************************************/

void setGsmToLocalTime(void) {
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("setGsmToLocalTime_IN\r\n");
    //********Debug log#end**************//
#endif
    timer3Count = 30; // 30 sec window
    setBCDdigit(0x0B,0);  // (].) BCD indication for setGsmToLocalTime Action
    gsmSetToLocalTime = false;
    controllerCommandExecuted = false;
    msgIndex = CLEAR;
    transmitStringToGSM("AT+CLTS?\r\n"); // To get local time stamp  +CCLK: "18/05/26,12:00:06+22"
    T3CONbits.TMR3ON = ON; // Start timer thread to unlock system if GSM fails to respond within 15 sec
    while (!controllerCommandExecuted);
    PIR5bits.TMR3IF = SET; //Stop timer thread
    if (gsmResponse[7] != '1') {
        controllerCommandExecuted = false;
        msgIndex = CLEAR;
        transmitStringToGSM("AT+CLTS=1\r\n"); // To get local time stamp  +CCLK: "18/05/26,12:00:06+22"
        T3CONbits.TMR3ON = ON; // Start timer thread to unlock system if GSM fails to respond within 15 sec
        while (!controllerCommandExecuted);
        PIR5bits.TMR3IF = SET; //Stop timer thread
        controllerCommandExecuted = false;
        msgIndex = CLEAR;
        transmitStringToGSM("AT&W\r\n"); // To get local time stamp  +CCLK: "18/05/26,12:00:06+22"
        T3CONbits.TMR3ON = ON; // Start timer thread to unlock system if GSM fails to respond within 15 sec
        while (!controllerCommandExecuted);
        PIR5bits.TMR3IF = SET; //Stop timer thread
        transmitStringToGSM("AT+CFUN=0\r\n"); // Set minimum functionality, IMSI detach procedure
        myMsDelay(30000);
        myMsDelay(30000);
        transmitStringToGSM("AT+CFUN=1\r\n"); //Set the full functionality mode with a complete software reset
        myMsDelay(60000);//reboot wait time
        myMsDelay(60000);//reboot wait time
        controllerCommandExecuted = false;
        msgIndex = CLEAR;
        transmitStringToGSM("AT+CLTS?\r\n"); // To get local time stamp  +CCLK: "18/05/26,12:00:06+22"
        T3CONbits.TMR3ON = ON; // Start timer thread to unlock system if GSM fails to respond within 15 sec
        while (!controllerCommandExecuted);
        PIR5bits.TMR3IF = SET; //Stop timer thread
        if (gsmResponse[7] == '1') {
            gsmSetToLocalTime = true;
        }
    }
    else {
        gsmSetToLocalTime = true;
    }
    //myMsDelay(1000);
    //checkGsmConnection(); // Check GSM connection after reset.
    setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("setGsmToLocalTime_OUT\r\n");
    //********Debug log#end**************//
#endif
}

/*************************************************************************************************************************

This function is called to delete messages from SIM storage
The purpose of this function is to send AT commands to GSM in order delete messages from SIM storage for receiving new messages in future

 **************************************************************************************************************************/
void deleteMsgFromSIMStorage(void) {
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("deleteMsgFromSIMStorage_IN\r\n");
    //********Debug log#end**************//
#endif
    timer3Count = 15; // 15 sec window
    setBCDdigit(0x09,1);  // (9) BCD indication Delete SMS action
    controllerCommandExecuted = false;
    msgIndex = 1;
    T3CONbits.TMR3ON = ON; // Start timer thread to unlock system if GSM fails to respond within 15 sec
    while (!controllerCommandExecuted) {        
        transmitStringToGSM("AT+CMGD=1,4\r\n"); // delete message from ALL location
        myMsDelay(500);
    }
    PIR5bits.TMR3IF = SET;
    setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
    // ADD indication if infinite
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("deleteMsgFromSIMStorage_OUT\r\n");
    //********Debug log#end**************//
#endif
}

/*************************************************************************************************************************

This function is called to send sms to given mobile no.
The purpose of this function is to Notify sender regarding its Action in SMS format

 **************************************************************************************************************************/
void sendSms(const char *message, unsigned char phoneNumber[], unsigned char info) {
    _Bool valveMatched = false;
    unsigned char index = CLEAR;
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("sendSms_IN\r\n");
    //********Debug log#end**************//
#endif
    timer3Count = 30; // 30 sec window
    //myMsDelay(100);
    transmitStringToGSM("AT+CMGS=\""); // Command to send an SMS message to GSM mobile
    myMsDelay(50);
    transmitNumberToGSM(phoneNumber, 10); // mention user mobile no. to send message
    myMsDelay(50);
    transmitStringToGSM("\"\r\n"); // next line to start sms content 
    myMsDelay(100);
    transmitStringToGSM(message);
    /*Encode message in base64 format*/
    /*
    strcpy((char *)stringToEncode,message);
    base64Encoder();
    transmitStringToGSM((const char *)encodedString); // send encoded message
    */
    myMsDelay(100);
    switch (info) {
    case newAdmin: //Send Additional info like New Admin Mobile Number  
        /*
        strncpy(stringToEncode,temporaryBytesArray,10);
        base64Encoder();
        transmitNumberToGSM(encodedString,16); // send encoded info(mobile no.) of new Admin to old Admin 
        */
        transmitNumberToGSM(temporaryBytesArray,10);
        myMsDelay(100);
        break;
    case fieldNoRequired: //Send Additional info like field valve number. // do if field valve action is requested
        /*
        strncpy(stringToEncode,temporaryBytesArray,2);
        base64Encoder();
        transmitNumberToGSM(encodedString,4); // send encoded field no. of valve being requested 
        */
        transmitNumberToGSM(temporaryBytesArray,2);
        myMsDelay(100);
        break;
    case timeRequired:
        /*
        strncpy(stringToEncode,temporaryBytesArray,17);
        base64Encoder();
        transmitNumberToGSM(encodedString,24); // send encoded date time stamp 
        */
        transmitNumberToGSM(temporaryBytesArray,17);
        myMsDelay(100);
        break;
    case secretCodeRequired:
        /*
        strncpy(stringToEncode,factryPswrd,6);
        base64Encoder();
        transmitNumberToGSM(encodedString,24); // send encoded factory password 
        */
        transmitNumberToGSM(factryPswrd,6);
        myMsDelay(100);
        break;						 
    case motorLoadRequired:
        lower8bits = noLoadCutOff;
        temporaryBytesArray[14] = (unsigned char) ((lower8bits / 1000) + 48);
        temporaryBytesArray[15] = (unsigned char) (((lower8bits % 1000) / 100) + 48);
        temporaryBytesArray[16] = (unsigned char) (((lower8bits % 100) / 10) + 48);
        temporaryBytesArray[17] = (unsigned char) ((lower8bits % 10) + 48);
        transmitNumberToGSM(temporaryBytesArray+14,4);
        myMsDelay(50);
        transmitStringToGSM(" and ");
        myMsDelay(50);
        lower8bits = fullLoadCutOff;
        temporaryBytesArray[14] = (unsigned char) ((lower8bits / 1000) + 48);
        temporaryBytesArray[15] = (unsigned char) (((lower8bits % 1000) / 100) + 48);
        temporaryBytesArray[16] = (unsigned char) (((lower8bits % 100) / 10) + 48);
        temporaryBytesArray[17] = (unsigned char) ((lower8bits % 10) + 48);
        transmitNumberToGSM(temporaryBytesArray+14,4);
        myMsDelay(100);
        break;
    case frequencyRequired:
        transmitNumberToGSM(temporaryBytesArray,2);
        myMsDelay(50);
        transmitStringToGSM(" is ");
        myMsDelay(50);
        lower8bits = moistureLevel;
        temporaryBytesArray[14] = (unsigned char) ((lower8bits / 10000) + 48);
        temporaryBytesArray[15] = (unsigned char) (((lower8bits % 10000) / 1000) + 48);
        temporaryBytesArray[16] = (unsigned char) (((lower8bits % 1000) / 100) + 48);
        temporaryBytesArray[17] = (unsigned char) (((lower8bits % 100) / 10) + 48);
        temporaryBytesArray[18] = (unsigned char) ((lower8bits % 10) + 48);
        transmitNumberToGSM(temporaryBytesArray+14,5);
        myMsDelay(100);
        break;
    case IrrigationData:
        transmitNumberToGSM(temporaryBytesArray, 2);
        myMsDelay(10);
        transmitStringToGSM(" ONprd:");
        myMsDelay(10);
        lower8bits = fieldValve[iterator].onPeriod;
        temporaryBytesArray[0] = (unsigned char) ((lower8bits / 100) + 48);
        temporaryBytesArray[1] = (unsigned char) (((lower8bits % 100) / 10) + 48);
        temporaryBytesArray[2] = (unsigned char) ((lower8bits % 10) + 48);
        transmitNumberToGSM(temporaryBytesArray,3);
        myMsDelay(10);
        transmitStringToGSM(" OFFprd:");
        myMsDelay(10);
        temporaryBytesArray[0] = (fieldValve[iterator].offPeriod/10) + 48;
        temporaryBytesArray[1] = (fieldValve[iterator].offPeriod%10) + 48;
        transmitNumberToGSM(temporaryBytesArray,2);
        myMsDelay(10);
        transmitStringToGSM(" Dry:");
        myMsDelay(10);
        lower8bits = fieldValve[iterator].dryValue;
        temporaryBytesArray[0] = (unsigned char) ((lower8bits / 100) + 48);
        temporaryBytesArray[1] = (unsigned char) (((lower8bits % 100) / 10) + 48);
        temporaryBytesArray[2] = (unsigned char) ((lower8bits % 10) + 48);
        transmitNumberToGSM(temporaryBytesArray,3);
        myMsDelay(10);
        transmitStringToGSM(" Wet:");
        myMsDelay(10);
        lower8bits = fieldValve[iterator].wetValue;
        temporaryBytesArray[0] = (unsigned char) ((lower8bits / 100) + 48);
        temporaryBytesArray[1] = (unsigned char) (((lower8bits % 100) / 10) + 48);
        temporaryBytesArray[2] = (unsigned char) ((lower8bits % 10) + 48);
        transmitNumberToGSM(temporaryBytesArray,3);
        myMsDelay(10);
        transmitStringToGSM(" DueDate: ");
        myMsDelay(10);
        temporaryBytesArray[0] = (fieldValve[iterator].nextDueDD/10) + 48;
        temporaryBytesArray[1] = (fieldValve[iterator].nextDueDD%10) + 48;
        transmitNumberToGSM(temporaryBytesArray,2);
        myMsDelay(10);
        temporaryBytesArray[0] = (fieldValve[iterator].nextDueMM/10) + 48;
        temporaryBytesArray[1] = (fieldValve[iterator].nextDueMM%10) + 48;
        transmitNumberToGSM(temporaryBytesArray,2);
        myMsDelay(10);
        temporaryBytesArray[0] = (fieldValve[iterator].nextDueYY/10) + 48;
        temporaryBytesArray[1] = (fieldValve[iterator].nextDueYY%10) + 48;
        transmitNumberToGSM(temporaryBytesArray,2);
        myMsDelay(10);
        temporaryBytesArray[0] = (fieldValve[iterator].motorOnTimeHour/10) + 48;
        temporaryBytesArray[1] = (fieldValve[iterator].motorOnTimeHour%10) + 48;
        transmitNumberToGSM(temporaryBytesArray,2);
        myMsDelay(10);
        temporaryBytesArray[0] = (fieldValve[iterator].motorOnTimeMinute/10) + 48;
        temporaryBytesArray[1] = (fieldValve[iterator].motorOnTimeMinute%10) + 48;
        transmitNumberToGSM(temporaryBytesArray,2);
        myMsDelay(10);
        transmitStringToGSM("\r\n");
        if (fieldValve[iterator].isFertigationEnabled) {
            transmitStringToGSM("Fertigation enabled with delay:");
            lower8bits = fieldValve[iterator].fertigationDelay;
            temporaryBytesArray[0] = (unsigned char) ((lower8bits / 100) + 48);
            temporaryBytesArray[1] = (unsigned char) (((lower8bits % 100) / 10) + 48);
            temporaryBytesArray[2] = (unsigned char) ((lower8bits % 10) + 48);
            transmitNumberToGSM(temporaryBytesArray,3);
            myMsDelay(10);
            transmitStringToGSM(" ONprd:");
            myMsDelay(10);
            lower8bits = fieldValve[iterator].fertigationONperiod;
            temporaryBytesArray[0] = (unsigned char) ((lower8bits / 100) + 48);
            temporaryBytesArray[1] = (unsigned char) (((lower8bits % 100) / 10) + 48);
            temporaryBytesArray[2] = (unsigned char) ((lower8bits % 10) + 48);
            transmitNumberToGSM(temporaryBytesArray,3);
            myMsDelay(10);
            transmitStringToGSM(" Iteration:");
            myMsDelay(10);
            temporaryBytesArray[0] = (fieldValve[iterator].fertigationInstance/10) + 48;
            temporaryBytesArray[1] = (fieldValve[iterator].fertigationInstance%10) + 48;
            transmitNumberToGSM(temporaryBytesArray,2);
            myMsDelay(10);
            transmitStringToGSM("\r\n");
        } 
        else {
            transmitStringToGSM("Fertigation not configured\r\n");
            myMsDelay(10);
        }
        break;
    case filtrationData:
        myMsDelay(10);
        transmitStringToGSM("\r\nDelay1: ");
        myMsDelay(10);
        temporaryBytesArray[0] = (filtrationDelay1/10) + 48;
        temporaryBytesArray[1] = (filtrationDelay1%10) + 48;
        transmitNumberToGSM(temporaryBytesArray,2);
        myMsDelay(10);
        transmitStringToGSM("(Min) ");
        myMsDelay(10);
        transmitStringToGSM("Delay2: ");
        myMsDelay(10);
        temporaryBytesArray[0] = (filtrationDelay2/10) + 48;
        temporaryBytesArray[1] = (filtrationDelay2%10) + 48;
        transmitNumberToGSM(temporaryBytesArray,2);
        myMsDelay(10);
        transmitStringToGSM("(Min) ");
        myMsDelay(10);
        transmitStringToGSM("Delay3: ");
        myMsDelay(10);
        temporaryBytesArray[0] = (filtrationDelay3/10) + 48;
        temporaryBytesArray[1] = (filtrationDelay3%10) + 48;
        transmitNumberToGSM(temporaryBytesArray,2);
        myMsDelay(10);
        transmitStringToGSM("(Min)");
        myMsDelay(10);
        transmitStringToGSM("\r\nONTime: ");
        myMsDelay(10);
        temporaryBytesArray[0] = (filtrationOnTime/10) + 48;
        temporaryBytesArray[1] = (filtrationOnTime%10) + 48;
        transmitNumberToGSM(temporaryBytesArray,2);
        myMsDelay(10);
        transmitStringToGSM("(Min) ");
        myMsDelay(10);
        transmitStringToGSM("Separation Time: ");
        myMsDelay(10);
        lower8bits = filtrationSeperationTime;
        temporaryBytesArray[0] = (unsigned char) ((lower8bits / 100) + 48);
        temporaryBytesArray[1] = (unsigned char) (((lower8bits % 100) / 10) + 48);
        temporaryBytesArray[2] = (unsigned char) ((lower8bits % 10) + 48);
        transmitNumberToGSM(temporaryBytesArray,3);
        myMsDelay(10);
        transmitStringToGSM("(Min)");
        myMsDelay(10);
        break;
    case commonActiveFieldNoRequired: //Send Additional info like list of field valve numbers in Parallel action. // do if field valve action is requested       
        iterator = 0;
        while(fieldList[iterator] != 255 && iterator < fieldCount) {
            for (index = 0; lastFieldList[index] != 255; index++) {
                if (fieldList[iterator]==lastFieldList[index]) {
                    valveMatched = true; 
                    break; // skip sms for activating already active valve
                }
            }
            if (valveMatched == false) {
                /***************************/
                // for field no. 01 to 09
                if (fieldList[iterator]<9){
                    temporaryBytesArray[0] = 48; // To store field no. of valve in action 
                    temporaryBytesArray[1] = fieldList[iterator] + 49; // To store field no. of valve in action 
                }// for field no. 10 to 12
                else if (fieldList[iterator] > 8 && fieldList[iterator] < 12) {
                    temporaryBytesArray[0] = 49; // To store field no. of valve in action 
                    temporaryBytesArray[1] = fieldList[iterator] + 39; // To store field no. of valve in action 
                }
                /***************************/
                //txByte(':');
                transmitNumberToGSM(temporaryBytesArray,2);
                txByte(' ');
                myMsDelay(100);
            }
            valveMatched = false;
            iterator++;
        }
        break;
    case commonInActiveFieldNoRequired: //Send Additional info like list of field valve numbers in Parallel action. // do if field valve action is requested       
        iterator = 0;
        while(lastFieldList[iterator] != 255 && iterator < fieldCount) {
            for (index = 0; fieldList[index] != 255; index++) {
                if (lastFieldList[iterator]==fieldList[index]) {
                    valveMatched = true; 
                    break; // skip sms for deactivating reactivated valve
                }
            }
            if (valveMatched == false) {
                /***************************/
                // for field no. 01 to 09
                if (lastFieldList[iterator]<9){
                    temporaryBytesArray[0] = 48; // To store field no. of valve in action 
                    temporaryBytesArray[1] = lastFieldList[iterator] + 49; // To store field no. of valve in action 
                }// for field no. 10 to 12
                else if (lastFieldList[iterator] > 8 && lastFieldList[iterator] < 12) {
                    temporaryBytesArray[0] = 49; // To store field no. of valve in action 
                    temporaryBytesArray[1] = lastFieldList[iterator] + 39; // To store field no. of valve in action 
                }
                /***************************/
                //txByte(':');
                transmitNumberToGSM(temporaryBytesArray,2);
                txByte(' ');
                myMsDelay(100);
            }
            valveMatched = false;
            iterator++;
        }
        break;
    case fieldListRequired: //Send Additional info like list of field valve numbers in Parallel action. // do if field valve action is requested       
        iterator = 0;
        while(fieldList[iterator] != 255 && iterator < fieldCount) {
            /***************************/
            // for field no. 01 to 09
            if (fieldList[iterator]<9){
                temporaryBytesArray[0] = 48; // To store field no. of valve in action 
                temporaryBytesArray[1] = fieldList[iterator] + 49; // To store field no. of valve in action 
            }// for field no. 10 to 12
            else if (fieldList[iterator] > 8 && fieldList[iterator] < 12) {
                temporaryBytesArray[0] = 49; // To store field no. of valve in action 
                temporaryBytesArray[1] = fieldList[iterator] + 39; // To store field no. of valve in action 
            }
            /***************************/
            //txByte(':');
            transmitNumberToGSM(temporaryBytesArray,2);
            txByte(' ');
            myMsDelay(100);
			iterator++;		   
        }
        break;
    case secretCode1Required:
        /*
        strncpy(stringToEncode,factryPswrd,6);
        base64Encoder();
        transmitNumberToGSM(encodedString,24); // send encoded factory password 
        */
        transmitNumberToGSM(pwd,6);
        myMsDelay(100);
        break;
    }
    controllerCommandExecuted = false; // System initiated request of sending sms to GSM			
    msgIndex = CLEAR; // clear message storage index
    txByte(terminateSms); // terminate SMS
    myMsDelay(100);
    setBCDdigit(0x00,0);  // (0.) BCD indication for OUT SMS Error
    T3CONbits.TMR3ON = ON; // Start timer thread to unlock system if GSM fails to respond within 15 sec
    while (!controllerCommandExecuted); // wait until gsm responds to send SMS action
    PIR5bits.TMR3IF = SET; //Stop timer thread
    setBCDdigit(0x0F,0); // Blank "." BCD Indication for Normal Condition
    myMsDelay(500);
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("sendSms_OUT\r\n");
    //********Debug log#end**************//
#endif
}

/*************************************************************************************************************************

This function is called to reset GSM Module
The purpose of this function is to reset GSM module on ERROR.

 **************************************************************************************************************************/
/*
void resetGSM(void)
{
    gsmReboot = LOW;
    myMsDelay(1000);
    gsmReboot = HIGH;  
}
*/

/*************************************************************************************************************************

This function is called to Check GSM signal strength 
The purpose of this function is to send AT commands to GSM in order to get signal strength.

**************************************************************************************************************************/

void checkSignalStrength(void) {
	unsigned char digit = 0;
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("checkSignalStrength_IN\r\n");
    //********Debug log#end**************//
#endif
    while (1) {
        setBCDdigit(0x0F,1); // BCD Indication for Flash
        myMsDelay(1000);
        digit = 0;
        timer3Count = 30; // 30 sec window
        setBCDdigit(0x0A,1);  // (c) BCD indication for checkSignalStrength Action
        controllerCommandExecuted = false;
        msgIndex = CLEAR;
        transmitStringToGSM("AT+CSQ\r\n"); // To get signal strength
        T3CONbits.TMR3ON = ON; // Start timer thread to unlock system if GSM fails to respond within 15 sec
        while (!controllerCommandExecuted);
        PIR5bits.TMR3IF = SET; //Stop timer thread
        for(msgIndex = 6;  gsmResponse[msgIndex] != ',' ; msgIndex++)  
        {
            if(isNumber(gsmResponse[msgIndex])) //is number
            {	
                if(gsmResponse[msgIndex+1] == ',')
                {	
                    gsmResponse[msgIndex] = gsmResponse[msgIndex]-48;
                    digit = digit+gsmResponse[msgIndex];
                }
                else
                {
                    gsmResponse[msgIndex] = gsmResponse[msgIndex]-48;
                    gsmResponse[msgIndex] = gsmResponse[msgIndex]*10;
                    digit = digit*10;
                    digit = digit+gsmResponse[msgIndex];
                    }       
            }
        }  
        myMsDelay(1000);
        setBCDdigit(0x0F,1); // BCD Indication for Flash
        myMsDelay(1000);
        if(digit <= 5) //Poor Signal
        {   
            setBCDdigit(1,1); // BCD Indication for Reset Action#n
            myMsDelay(10000);
        }
        else if(digit >= 6 && digit <= 9) //Very Low Signal
        { 
            setBCDdigit(2,1); // BCD Indication for Reset Action#n
            myMsDelay(10000);
        }
        else if(digit >= 10&&digit <= 13) //Low Signal
        { 
            setBCDdigit(3,1); // BCD Indication for Reset Action#n
            myMsDelay(10000);
        }
        else if(digit >= 14&&digit <= 17) //Moderate Signal
        { 
            setBCDdigit(4,1); // BCD Indication for Reset Action#n
            myMsDelay(10000);
        }
        else if(digit >= 18 && digit <= 21) //Good Signal
        { 
            setBCDdigit(5,1); // BCD Indication for Reset Action#n
            myMsDelay(10000);
        }
        else if(digit >= 22&& digit <= 25) //very good Signal
        { 
            setBCDdigit(6,1); // BCD Indication for Reset Action#n
            myMsDelay(10000);
        }
        else if(digit >= 26 && digit <= 31) //Excellent Signal
        { 
            setBCDdigit(7,1); // BCD Indication for Reset Action#n
            myMsDelay(10000);
        }
        else if(digit == 99) // not known or not detectable
        { 
            setBCDdigit(0,1); // BCD Indication for not known or not detectable
            myMsDelay(10000);
        }
        else {
            setBCDdigit(0,0); // BCD Indication for not known or not detectable
            myMsDelay(10000);
        }
    }
#ifdef DEBUG_MODE_ON_H
    //********Debug log#start************//
    transmitStringToDebug("checkSignalStrength_OUT\r\n");
    //********Debug log#end**************//
#endif
}
//*****************Serial communication functions_End****************//

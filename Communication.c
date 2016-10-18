/***************************************************************************//**
 *   @file   Communication.c
 *   @brief  Implementation of Communication Driver for PIC32MX320F128H
             Processor.
 *   @author DBogdan (dragos.bogdan@analog.com)
********************************************************************************
 * Copyright 2013(c) Analog Devices, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Analog Devices, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *  - The use of this software may or may not infringe the patent rights
 *    of one or more patent holders.  This license does not release you
 *    from the requirement that you obtain separate licenses from these
 *    patent holders to use this software.
 *  - Use of the software either in source or binary form, must be run
 *    on or directly connected to an Analog Devices Inc. component.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*******************************************************************************/

static int channel_set; //something like that

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/
#include "Communication.h"    /*!< Communication definitions */
#include <Plib.h>

/******************************************************************************/
/************************ Functions Definitions *******************************/
/******************************************************************************/

/***************************************************************************//**
 * @brief Initializes the SPI communication peripheral.
 *
 * @param lsbFirst  - Transfer format (0 or 1).
 *                    Example: 0x0 - MSB first.
 *                             0x1 - LSB first.
 * @param clockFreq - SPI clock frequency (Hz).
 *                    Example: 1000 - SPI clock frequency is 1 kHz.
 * @param clockPol  - SPI clock polarity (0 or 1).
 *                    Example: 0x0 - Idle state for clock is a low level; active
 *                                   state is a high level;
 *                             0x1 - Idle state for clock is a high level; active
 *                                   state is a low level.
 * @param clockEdg  - SPI clock edge (0 or 1).
 *                    Example: 0x0 - Serial output data changes on transition
 *                                   from idle clock state to active clock state;
 *                             0x1 - Serial output data changes on transition
 *                                   from active clock state to idle clock state.
 *
 * @return status   - Result of the initialization procedure.
 *                    Example:  0 - if initialization was successful;
 *                             -1 - if initialization was unsuccessful.
*******************************************************************************/
unsigned int SPIMasterInit(int channel)
{
    channel_set = channel;

    int status = -1;
    //check what channel is selected
    if(channel == 3){

        mPORTDSetPinsDigitalOut(BIT_14);

        //open spi channel
        SpiChnOpen(3,
               SPI_OPEN_MSTEN|
               SPI_OPEN_CKP_HIGH|
               SPI_OPEN_MODE8|
               SPI_OPEN_ENHBUF|
               SPI_OPEN_ON,
               20);
        status = 0;
        
 
    }
    else if(channel == 4){

        mPORTFSetPinsDigitalOut(BIT_12);

        //open spi channel
        SpiChnOpen(4,
               SPI_OPEN_MSTEN|
               SPI_OPEN_CKP_HIGH|
               SPI_OPEN_MODE8|
               SPI_OPEN_ENHBUF|
               SPI_OPEN_ON,
               20);
        status = 0;


    }
    
    return status;

}

/***************************************************************************//**
 * @brief Writes data to SPI.
 *
 * @param slaveDeviceId - The ID of the selected slave device.
 * @param data          - Data represents the write buffer.
 * @param bytesNumber   - Number of bytes to write.
 *
 * @return Number of written bytes.
*******************************************************************************/
unsigned int SPIMasterIO(unsigned char bytes[], int numWriteBytes, int numReadBytes)
{
    
    if((channel_set != 3)&&(channel_set != 4)){
        return -1;
    }
    
    //clear the slave select pin on the active channel
    if(channel_set == 3){
        mPORTDClearBits(BIT_14);
    }
    if(channel_set == 4){
        mPORTFClearBits(BIT_12);
    }
    
    //writing to slave
    int i;
    for(i = 0; i < numWriteBytes; i++){
        SpiChnPutC(channel_set, bytes[i]);
        SpiChnGetC(channel_set);
    }

 
    //reading from slave
    for(i = numWriteBytes ; i < numWriteBytes+numReadBytes; i++){
        SpiChnPutC(channel_set, 0x00);
        bytes [i] = SpiChnGetC(channel_set);
    }


    //set the slave select output pin on the active channel
    if(channel_set == 3)
        mPORTDSetBits(BIT_14);
    if(channel_set == 4)
        mPORTFSetBits(BIT_12);


    return 0;
}


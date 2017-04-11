/*
 * twimaster.c
 *
 * Created: 11/04/2017 9:23:48 AM
 * Author : jakub
 */ 

#include <avr/io.h>
#include <util/delay.h>
#include "twimaster.h"

/* Replace with your library code */
void twimasterinit(void)
{
	TWIInfo0.mode = Ready;
	TWIInfo0.errorCode = 0xFF;
	TWIInfo0.repStart = 0;
	// Set pre-scalers (no pre-scaling)
	TWSR0 = 0;
	// Set bit rate
	TWI_MASTER_SETCLOCK0();
	// Enable TWI and interrupt
	TWI_MASTER_ENABLE0();
}

uint8_t twimasterisready(void)
{
	if ( (TWIInfo0.mode == Ready) | (TWIInfo0.mode == RepeatedStartSent) )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

uint8_t twimastertransmit(void *const TXdata, uint8_t dataLen, uint8_t repStart) 
{
		if (dataLen <= TWI_TX_MALENGTH)
		{
			// Wait until ready
			while (!twimasterisready()) {_delay_us(1);}
			// Set repeated start mode
			TWIInfo0.repStart = repStart;
			// Copy data into the transmit buffer
			uint8_t *data = (uint8_t *)TXdata;
			for (int i = 0; i < dataLen; i++)
			{
				TWITransmitBuffer0[i] = data[i];
			}
			// Copy transmit info to global variables
			TWITxBuffLen0 = dataLen;
			TWITxBuffIndex0 = 0;
			
			// If a repeated start has been sent, then devices are already listening for an address
			// and another start does not need to be sent.
			if (TWIInfo0.mode == RepeatedStartSent)
			{
				TWIInfo0.mode = Initializing;
				TWDR0 = TWITransmitBuffer0[TWITxBuffIndex0++]; // Load data to transmit buffer
				TWI_MASTER_TRANSMIT0(); // Send the data
			}
			else // Otherwise, just send the normal start signal to begin transmission.
			{
				TWIInfo0.mode = Initializing;
				TWI_MASTER_START0();
			}
			
		}
		else
		{
			return 1; // return an error if data length is longer than buffer
		}
		return 0;
}

uint8_t twimasterread(uint8_t TWIaddr, uint8_t bytesToRead, uint8_t repStart)
{
	// Check if number of bytes to read can fit in the RXbuffer
	if (bytesToRead < TWI_RX_MALENGTH)
	{
		// Reset buffer index and set RXBuffLen to the number of bytes to read
		TWIRxBuffIndex0 = 0;
		TWIRxBuffLen0 = bytesToRead;
		// Create the one value array for the address to be transmitted
		uint8_t TXdata[1];
		// Shift the address and AND a 1 into the read write bit (set to write mode)
		TXdata[0] = (TWIaddr << 1) | 0x01;
		// Use the TWITransmitData function to initialize the transfer and address the slave
		twimastertransmit(TXdata, 1, repStart);
	}
	else
	{
		return 0;
	}
	return 1;
}

void twimasterreceive(void) 
{
		// If there is more than one byte to be read, receive data byte and return an ACK
		if (TWIRxBuffIndex0 < TWIRxBuffLen0-1)
		{
			TWIInfo0.errorCode = TWI_NO_RELEVANT_INFO;
			TWI_MASTER_ACK0();
		}
		// Otherwise when a data byte (the only data byte) is received, return NACK
		else
		{
			TWIInfo0.errorCode = TWI_NO_RELEVANT_INFO;
			TWI_MASTER_NACK0();
		}
}

ISR (TWI0_vect)
{
	switch (TWI_STATUS0)
	{
		// MASTER TRANSMITTER

		case TWI_MT_SLAW_ACK: // SLA+W transmitted and ACK received
			// Set mode to Master Transmitter
			TWIInfo0.mode = MasterTransmitter;
		case TWI_START_SENT: // Start condition has been transmitted
		case TWI_MT_DATA_ACK: // Data byte has been transmitted, ACK received
			if (TWITxBuffIndex0 < TWITxBuffLen0) // If there is more data to send
			{
				TWDR0 = TWITransmitBuffer0[TWITxBuffIndex0++]; // Load data to transmit buffer
				TWIInfo0.errorCode = TWI_NO_RELEVANT_INFO;
				TWI_MASTER_TRANSMIT0(); // Send the data
			}
			// This transmission is complete however do not release bus yet
			else if (TWIInfo0.repStart)
			{
				TWIInfo0.errorCode = TWI_SUCCESS;
				TWI_MASTER_START0();
			}
			// All transmissions are complete, exit
			else
			{
				TWIInfo0.mode = Ready;
				TWIInfo0.errorCode = TWI_SUCCESS;
				TWI_MASTER_STOP0();
			}
			break;
		
		// MASTER RECEIVER
	
		case TWI_MR_SLAR_ACK: // SLA+R has been transmitted, ACK has been received
			// Switch to Master Receiver mode
			TWIInfo0.mode = MasterReceiver;
			twimasterreceive();
			break;
		
		case TWI_MR_DATA_ACK: // Data has been received, ACK has been transmitted.
			/// -- HANDLE DATA BYTE --- ///
			TWIReceiveBuffer0[TWIRxBuffIndex0++] = TWDR0;
			twimasterreceive();
			break;
		
		case TWI_MR_DATA_NACK: // Data byte has been received, NACK has been transmitted. End of transmission.
			/// -- HANDLE DATA BYTE --- ///
			TWIReceiveBuffer0[TWIRxBuffIndex0++] = TWDR0;
			// This transmission is complete however do not release bus yet
			if (TWIInfo0.repStart)
			{
				TWIInfo0.errorCode = TWI_SUCCESS;
				TWI_MASTER_START0();
			}
			// All transmissions are complete, exit
			else
			{
				TWIInfo0.mode = Ready;
				TWIInfo0.errorCode = TWI_SUCCESS;
				TWI_MASTER_STOP0();
			}
			break;
		
		// MASTER COMMON
		
		case TWI_MR_SLAR_NACK: // SLA+R transmitted, NACK received
		case TWI_MT_SLAW_NACK: // SLA+W transmitted, NACK received
		case TWI_MT_DATA_NACK: // Data byte has been transmitted, NACK received
		case TWI_LOST_ARBIT: // Arbitration has been lost
			// Return error and send stop and set mode to ready
			if (TWIInfo0.repStart)
			{
				TWIInfo0.errorCode = TWI_STATUS0;
				TWI_MASTER_START0();
			}
			// All transmissions are complete, exit
			else
			{
				TWIInfo0.mode = Ready;
				TWIInfo0.errorCode = TWI_STATUS0;
				TWI_MASTER_STOP0();
			}
			break;
		case TWI_REP_START_SENT: // Repeated start has been transmitted
			// Set the mode but DO NOT clear TWINT as the next data is not yet ready
			TWIInfo0.mode = RepeatedStartSent;
			break;

		// MISC

		case TWI_NO_RELEVANT_INFO: // It is not really possible to get into this ISR on this condition
			// Rather, it is there to be manually set between operations
			break;
		case TWI_ILLEGAL_START_STOP: // Illegal START/STOP, abort and return error
			TWIInfo0.errorCode = TWI_ILLEGAL_START_STOP;
			TWIInfo0.mode = Ready;
			TWI_MASTER_STOP0();
			break;
	}
	
}
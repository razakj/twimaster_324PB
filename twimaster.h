/*
 * twimaster.h
 *
 * Created: 11/04/2017 9:24:07 AM
 *  Author: jakub
 */ 


#ifndef TWIMASTER_H_
#define TWIMASTER_H_

// TWI bit rate
#define TWI_FREQ				400000

// TWI buffers
#define TWI_TX_MALENGTH 16
#define TWI_RX_MALENGTH 16

uint8_t TWITransmitBuffer0[TWI_TX_MALENGTH];
volatile uint8_t TWIReceiveBuffer0[TWI_RX_MALENGTH];

volatile int TWITxBuffIndex0; // Index of the transmit buffer. Is volatile, can change at any time.
int TWIRxBuffIndex0; // Current index in the receive buffer

int TWITxBuffLen0; // The total length of the transmit buffer
int TWIRxBuffLen0; // The total number of bytes to read (should be less than RXMAXBUFFLEN)

// TWI modes
typedef enum {
	Ready,
	Initializing,
	RepeatedStartSent,
	MasterTransmitter,
	MasterReceiver,
	SlaceTransmitter,
	SlaveReciever
} TWIMode;

typedef struct {
	TWIMode mode;
	uint8_t errorCode;
	uint8_t repStart;
} TWIInfoStruct;

TWIInfoStruct TWIInfo0, TWIInfo1;

// TWI status - Last 2 bits (+1 unused) must be masked away using 0xF8 for easier status comparison
#define TWI_STATUS0				(TWSR0 & 0xF8)
#define TWI_STATUS1				(TWSR1 & 0xF8)

#define TWI_START_SENT			0x08 // Start sent
#define TWI_REP_START_SENT		0x10 // Repeated Start sent

#define TWI_MT_SLAW_ACK			0x18 // SLA+W sent and ACK received
#define TWI_MT_SLAW_NACK		0x20 // SLA+W sent and NACK received
#define TWI_MT_DATA_ACK			0x28 // DATA sent and ACK received
#define TWI_MT_DATA_NACK		0x30 // DATA sent and NACK received

#define TWI_MR_SLAR_ACK			0x40 // SLA+R sent, ACK received
#define TWI_MR_SLAR_NACK		0x48 // SLA+R sent, NACK received
#define TWI_MR_DATA_ACK			0x50 // Data received, ACK returned
#define TWI_MR_DATA_NACK		0x58 // Data received, NACK returned

#define TWI_LOST_ARBIT			0x38 // Arbitration has been lost
#define TWI_NO_RELEVANT_INFO	0xF8 // No relevant information available
#define TWI_ILLEGAL_START_STOP	0x00 // Illegal START or STOP condition has been detected
#define TWI_SUCCESS				0xFF // Successful transfer, this state is impossible from TWSR as bit2 is 0 and read only

// TWI control macros
#define TWI_MASTER_SETCLOCK0()	(TWBR0 = ((F_CPU / TWI_FREQ) - 16) / 2); // Set clock speed
#define TWI_MASTER_ENABLE0()	(TWCR0 = (1 << TWIE) | (1 << TWEN)) // Enabled TWI0 interface and interrupts
#define TWI_MASTER_START0()		(TWCR0 = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN)|(1<<TWIE)) // Send the START signal, enable interrupts and TWI, clear TWINT flag to resume transfer.
#define TWI_MASTER_STOP0()		(TWCR0 = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN)|(1<<TWIE)) // Send the STOP signal, enable interrupts and TWI, clear TWINT flag.
#define TWI_MASTER_TRANSMIT0()	(TWCR0 = (1<<TWINT)|(1<<TWEN)|(1<<TWIE)) // Used to resume a transfer, clear TWINT and ensure that TWI and interrupts are enabled.
#define TWI_MASTER_ACK0()		(TWCR0 = (1<<TWINT)|(1<<TWEN)|(1<<TWIE)|(1<<TWEA)) // FOR MR mode. Resume a transfer, ensure that TWI and interrupts are enabled and respond with an ACK if the device is addressed as a slave or after it receives a byte.
#define TWI_MASTER_NACK0()		(TWCR0 = (1<<TWINT)|(1<<TWEN)|(1<<TWIE)) // FOR MR mode. Resume a transfer, ensure that TWI and interrupts are enabled but DO NOT respond with an ACK if the device is addressed as a slave or after it receives a byte.

uint8_t twimasterisready(void);
void twimasterinit(void);
uint8_t twimastertransmit(void *const TXdata, uint8_t dataLen, uint8_t repStart);
uint8_t twimasterread(uint8_t TWIaddr, uint8_t bytesToRead, uint8_t repStart);


#endif /* TWIMASTER_H_ */
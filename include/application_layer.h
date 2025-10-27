// Application layer protocol header.
// DO NOT CHANGE THIS FILE

#ifndef _APPLICATION_LAYER_H_
#define _APPLICATION_LAYER_H_

#define C_START 0x01
#define C_DATA 0x02
#define C_END 0x03

#define T_SIZE 0x00
#define T_NAME 0x01

// Size of maximum acceptable payload.
// Maximum number of bytes that application layer should send to link layer.
#define MAX_PAYLOAD_SIZE 1000

// Application layer main function.
// Arguments:
//   serialPort: Serial port name (e.g., /dev/ttyS0).
//   role: Application role {"tx", "rx"}.
//   baudrate: Baudrate of the serial port.
//   nTries: Maximum number of frame retries.
//   timeout: Frame timeout.
//   filename: Name of the file to send / receive.
void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename);


// Builds and sends a control packet (START or END)
int sendControlPacket(unsigned char controlField, const char *filename, long fileSize);

// Builds and sends a data packet (just data, no sequence number)
int sendDataPacket(const unsigned char *data, int dataLength);

// Reads and parses a control packet (START or END)
int receiveControlPacket(unsigned char expectedControl, char *filename, long *fileSize);

// Reads and parses a data packet (extracts payload)
int receiveDataPacket(unsigned char **data, int *dataLength);


////////////////////
/// Main helpers ///
////////////////////

// Handles transmitter role: send START, DATA, END
int transmitterMain(const char *filename);

// Handles receiver role: receive START, DATA, END
int receiverMain(const char *filename);

const char *getBaseFileName(const char *path);

long getFileSize(FILE *file);

#endif // _APPLICATION_LAYER_H_

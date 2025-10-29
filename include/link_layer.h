// Link layer header.
// DO NOT CHANGE THIS FILE

#ifndef _LINK_LAYER_H_
#define _LINK_LAYER_H_

#define FLAG   0x7E

#define A_TX   0x03
#define A_RX   0x01

#define C_SET  0x03
#define C_UA   0x07
#define C_RR0  0xAA
#define C_RR1  0xAB
#define C_REJ0 0x54
#define C_REJ1 0x55

#define ESC       0x7D
#define FLAG_ESC  0x5E
#define ESC_ESC   0x5D

#define DISC 0x0B

#define TRUE  1
#define FALSE 0

extern LinkLayer currentParams;
extern int linkFd;

typedef enum
{
    LlTx,
    LlRx,
} LinkLayerRole;

typedef struct
{
    char serialPort[50];
    LinkLayerRole role;
    int baudRate;
    int nRetransmissions;
    int timeout;
} LinkLayer;

// MISC
#define FALSE 0
#define TRUE 1

// Open a connection using the "port" parameters defined in struct linkLayer.
// Return 0 on success or -1 on error.
int llopen(LinkLayer connectionParameters);

// Send data in buf with size bufSize.
// Return number of chars written, or -1 on error.
int llwrite(const unsigned char *buf, int bufSize);

// Receive data in packet.
// Return number of chars read, or -1 on error.
int llread(unsigned char *packet);

// Close previously opened connection and print transmission statistics in the console.
// Return 0 on success or -1 on error.
int llclose();

int transmissorLLopen();
int receptorLLopen();

int readControlField();
int buildIFrame(unsigned char *frame, const unsigned char *buf, int bufSize, int sequenceNumber);
int applyByteStuffing(const unsigned char *input, int inputSize, unsigned char *output);
unsigned char calculateBCC2(const unsigned char *buf, int bufSize);

int readFrame(unsigned char *frame, int maxSize); 
int validateIFrame(const unsigned char *frame, int frameSize, unsigned char *packet, int *sequenceNumber, int maxPacketSize);
int destuffing(const unsigned char *input, int inputSize, unsigned char *output, int maxOutput);

void sendRR(int sequenceNumber);
void sendREJ(int sequenceNumber);

#endif // _LINK_LAYER_H_


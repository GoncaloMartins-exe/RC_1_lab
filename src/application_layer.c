// Application layer protocol implementation

#include "../include/application_layer.h"
#include "../include/link_layer.h"

#include <stdio.h>

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer linkLayerParameters = {serialPort, role, baudRate, nTries, timeout};


}


int sendControlPacket(unsigned char controlField, const char *filename, long fileSize){
    unsigned char packet[512];
    int index = 0;

    packet[index++] = controlField;


    // SIZE -------------------------------------------------------
    packet[index++] = T_SIZE;
    int sizeLength = 0; // Find number of bytes of the fileSize ()
    long tempSize = fileSize;
    while (tempSize > 0) {
        tempSize >>= 8;
        sizeLength++;
    }
    if (sizeLength == 0) sizeLength = 1;

    packet[index++] = sizeLength; // L1

    for (int i = sizeLength - 1; i >= 0; i--) { 
        packet[index++] = (fileSize >> (8 * i)) & 0xFF;
    }


    // NAME -------------------------------------------------------
    int nameLength = strlen(filename);

    packet[index++] = T_NAME;
    packet[index++] = nameLength; // L2
    memcpy(packet + index, filename, nameLength); // V2
    index += nameLength;

    // Send packet through link layer
    int result = llwrite(packet, index);
    if (result < 0) {
        fprintf(stderr, "Error sending control packet\n");
        return -1;
    }

    return 0;
}


int sendDataPacket(const unsigned char *data, int dataLength){
    unsigned char dataFrame[MAX_PAYLOAD_SIZE + 3];
    int index = 0;

    dataFrame[index++] = C_DATA; // C (Control Field)

    dataFrame[index++] = (dataLength >> 8) & 0xFF; // L2 
    dataFrame[index++] = dataLength & 0xFF;        // L1

    memcpy(dataFrame + index, data, dataLength);   // Copy data bytes
    index += dataLength;

    int result = llwrite(dataFrame, index);
    return result;
}


int receiveControlPacket(unsigned char expectedControl, char *filename, long *fileSize) {
    unsigned char frame[512];
    int frameSize = llread(frame);

    if (frameSize < 0) {
        fprintf(stderr, "Error reading control packet from link layer\n");
        return -1;
    }

    // Check control field (C)
    if (frame[0] != expectedControl) {
        fprintf(stderr, "Unexpected control field (expected 0x%02X, got 0x%02X)\n",
                expectedControl, frame[0]);
        return -1;
    }

    int index = 1;
    *fileSize = 0;
    filename[0] = '\0';

    // Parse the fields (T, L, V)
    while (index < frameSize) {
        unsigned char type = frame[index++];
        unsigned char length = frame[index++];

        if (type == T_SIZE) {
            // Read size
            long size = 0; //0x00 00 00 00 00 00 00 00
            for (int i = 0; i < length; i++) {
                size = (size << 8) | frame[index + i];
            }
            *fileSize = size;
            index += length;

        } else if (type == T_NAME) {
            memcpy(filename, frame + index, length);
            filename[length] = '\0';
            index += length;

        } else {
            fprintf(stderr, "Unknown type field (0x%02X)\n", type);
            return -1;
        }
    }

    printf("Received %s packet — File: '%s', Size: %ld bytes\n",
           expectedControl == C_START ? "START" :
           expectedControl == C_END   ? "END" : "UNKNOWN",
           filename, *fileSize);

    return 0;
}


int receiveDataPacket(unsigned char **data, int *dataLength) {
    unsigned char packet[MAX_PAYLOAD_SIZE + 3];
    int packetSize = llread(packet);

    if (packetSize < 0) {
        fprintf(stderr, "Error reading data packet from link layer\n");
        return -1;
    }

    if (packet[0] != C_DATA) {
        fprintf(stderr, "Unexpected control field (expected C_DATA = 0x%02X, got 0x%02X)\n",
                C_DATA, packet[0]);
        return -1;
    }

    *dataLength = (packet[1] << 8) | packet[2];
    if (*dataLength > MAX_PAYLOAD_SIZE || *dataLength < 0) {
        fprintf(stderr, "Invalid data length: %d\n", *dataLength);
        return -1;
    }

    *data = (unsigned char *)malloc(*dataLength);
    if (*data == NULL) {
        perror("Memory allocation failed");
        return -1;
    }

    // Copy data bytes
    memcpy(*data, packet + 3, *dataLength);

    return 0;
}


int transmitterMain(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return -1;
    }

    long fileSize = getFileSize(file);
    const char *baseName = getBaseFileName(filename);

    printf("Transmitter: Sending file '%s' (%ld bytes)\n", baseName, fileSize);

    // Send START control packet (using base file name)
    if (sendControlPacket(C_START, baseName, fileSize) < 0) {
        fprintf(stderr, "Error sending START packet\n");
        fclose(file);
        return -1;
    }

    // Send file data in chunks
    unsigned char buffer[MAX_PAYLOAD_SIZE];
    int bytesRead;
    while ((bytesRead = fread(buffer, 1, MAX_PAYLOAD_SIZE, file)) > 0) {
        printf("Sending DATA (%d bytes)\n", bytesRead);

        if (sendDataPacket(buffer, bytesRead) < 0) {
            fprintf(stderr, "Error sending DATA packet\n");
            fclose(file);
            return -1;
        }
    }

    // Send END control packet
    if (sendControlPacket(C_END, baseName, fileSize) < 0) {
        fprintf(stderr, "Error sending END packet\n");
        fclose(file);
        return -1;
    }

    printf("File transfer complete!\n");
    fclose(file);
    return 0;
}


int receiverMain(const char *filename) {
    char receivedFilename[256];
    long fileSize = 0;

    printf("Waiting for START control packet...\n");

    if (receiveControlPacket(C_START, receivedFilename, &fileSize) < 0) {
        fprintf(stderr, "Error receiving START packet.\n");
        return -1;
    }

    printf("Receiving file: %s (%ld bytes)\n", receivedFilename, fileSize);

    FILE *file = fopen(receivedFilename, "wb");
    if (!file) {
        perror("Error creating output file");
        return -1;
    }

    long totalBytesReceived = 0;

    while (1) {
        unsigned char *data = NULL;
        int dataLength = 0;

        int res = receiveDataPacket(&data, &dataLength);

        if (res == 0) {
            fwrite(data, 1, dataLength, file);
            totalBytesReceived += dataLength;
            free(data);

            printf("Received %ld / %ld bytes\r", totalBytesReceived, fileSize);
            fflush(stdout);

        } else if (res < 0) {
            // Might be END
            break;
        }
    }

    fclose(file);
    printf("\nFile received successfully: %s (%ld bytes written)\n", receivedFilename, totalBytesReceived);
    return 0;
}


const char *getBaseFileName(const char *path) {
    const char *slash = strrchr(path, '/');
    if (slash == NULL)
        return path;
    return slash + 1;
}

long getFileSize(FILE *file) {
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);
    return size;
}

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

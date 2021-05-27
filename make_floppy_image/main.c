// Copyright (c) 2021 Raphael Champeimont

#include <stdio.h>
#include <stdlib.h>

#define NUM_TRACKS 80
#define NUM_HEADS 2
#define NUM_SECTORS_PER_TRACK 18
#define SECTOR_SIZE 512

#define FLOPPY_SIZE (NUM_TRACKS*NUM_HEADS*NUM_SECTORS_PER_TRACK*SECTOR_SIZE)

// The special sector order we are going to use to read the floppy faster.
const int SECTOR_READ_ORDER[NUM_SECTORS_PER_TRACK] = {1, 3, 5, 7, 9, 11, 13, 15, 17, 2, 4, 6, 8, 10, 12, 14, 16, 18};

void translate8bitTo4bit(unsigned char *dst, unsigned char* src, int srcSize) {
    int i;
    for (i=0; i<srcSize/2; i++) {
        dst[i] = (src[2*i] & 0xf0) | (src[2*i+1] >> 4);
    }
}

int copyAndTranslate(FILE *fr, unsigned char *floppyImage) {
    unsigned char buf[SECTOR_SIZE*2];
    int totalBytesRead = 0;
    int totalBytesWritten = 0;

    for (int track = 0; track < NUM_TRACKS; track++) {
        for (int head = 0; head < NUM_HEADS; head++) {
            for (int i = 0; i < NUM_SECTORS_PER_TRACK; i++) {
                int sector = SECTOR_READ_ORDER[i];
                int absoluteSectorPosition = track*NUM_HEADS*NUM_SECTORS_PER_TRACK*SECTOR_SIZE + head*NUM_SECTORS_PER_TRACK*SECTOR_SIZE + (sector-1)*SECTOR_SIZE;
                int readBytes = fread(buf, 1, SECTOR_SIZE*2, fr);
                totalBytesRead += readBytes;
                if (readBytes == 0) {
                    printf("No more data to read after %d bytes have been read.\n", totalBytesRead);
                    return totalBytesWritten;
                }
                translate8bitTo4bit(floppyImage + absoluteSectorPosition, buf, readBytes);
                totalBytesWritten += readBytes/2;
            }
        }
    }
    return totalBytesWritten;
}

int main() {
    char *inputFile = "../spin_8kHz.wav";
    char *outputFile = "../spin_floppy.img";

    //char *inputFile = "../test_in.img";
    //char *outputFile = "../test_out.img";

    unsigned char *floppyImage = calloc(FLOPPY_SIZE, 1);

    printf("Reading input file %s\n", inputFile);
    FILE *fr = fopen(inputFile, "rb");
    if (!fr) {
        printf("File read error\n");
        exit(1);
    }

    // skip header
    fseek(fr, 44, SEEK_SET);

    int totalBytesWritten = copyAndTranslate(fr, floppyImage);
    printf("%d bytes are going to be written to non-zero values.\n", totalBytesWritten);
    printf("Floppy usage will be: %d %%\n", totalBytesWritten*100/FLOPPY_SIZE);

    fclose(fr);

    printf("Writing output file %s\n", outputFile);
    FILE *fw = fopen(outputFile, "wb");
    fwrite(floppyImage, 1, FLOPPY_SIZE, fw);
    fclose(fw);


    return 0;
}

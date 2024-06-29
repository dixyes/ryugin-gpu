
/* This program is free software. It comes without any warranty, to
     * the extent permitted by applicable law. You can redistribute it
     * and/or modify it under the terms of the Do What The Fuck You Want
     * To Public License, Version 2, as published by Sam Hocevar. See
     * http://www.wtfpl.net/ for more details. */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include <sys/stat.h>
#include <sys/mman.h>

#define XRES 800
#define YRES 600
#define REFRESH 60

// the volume
#define SCALE 3

int main(int argc, char **argv)
{
    int ret;
    if (argc < 2) {
        printf("Usage: <this> <wavfile>\n");
        return -1;
    }

    size_t pagesize = sysconf(_SC_PAGE_SIZE);

    // parse the wav file
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open the wav file");
        return 1;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("stat");
        return 1;
    }


    // mmap data to memory
    char *data = mmap(NULL, st.st_size / pagesize * pagesize + pagesize, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap data");
        return 1;
    }

    if (
        memcmp(data, "RIFF", 4) != 0 ||
        memcmp(data + 8, "WAVE", 4) != 0
    ) {
        printf("Not a wav file\n");
        return 1;
    }

    uint16_t format;
    uint16_t channels;
    uint32_t samplerate;
    uint32_t byterate;
    uint16_t blockalign;
    uint16_t bitspersample;

    size_t i = 12;
    while (i < st.st_size) {

        if (memcmp(data + i, "fmt ", 4) == 0) {
            uint32_t size = *(uint32_t *)(data + i + 4);
            if (size != 16) {
                printf("bad size %u\n", size);
                return -1;
            }

            format = *(uint16_t *)(data + i + 8);
            channels = *(uint16_t *)(data + i + 10);
            samplerate = *(uint32_t *)(data + i + 12);
            byterate = *(uint32_t *)(data + i + 16);
            blockalign = *(uint16_t *)(data + i + 20);
            bitspersample = *(uint16_t *)(data + i + 22);

            printf("format %u\n", format);
            printf("channels %u\n", channels);
            printf("samplerate %u\n", samplerate);
            printf("byterate %u\n", byterate);
            printf("blockalign %u\n", blockalign);
            printf("bitspersample %u\n", bitspersample);

            if (channels != 2) {
                printf("wave not stereo, how to use it?\n");
                return -1;
            }
            if (bitspersample != 16) {
                printf("non-16bit not implemented yet\n");
                return -1;
            }

            i += 8 + size;
        } else if (memcmp(data + i, "data", 4) == 0) {
            uint32_t size = *(uint32_t *)(data + i + 4);

            printf("data size %u\n", size);
            i += 8;
            break;
        } else {
            // not supported yet
            printf("Unknown chunk\n");
            return -1;
            // uint32_t size;
            // red = read(fd, &size, 4);
            // if (red != 4) {
            //     break;
            // }

            // lseek(fd, size, SEEK_CUR);
        }
    }

    int16_t *pcmData = (int16_t *)(data + i);
    size_t pcmDataLen = (st.st_size - i) / sizeof(int16_t);
    // uint32_t frames = (pcmDataLen / 2 / samplerate) * REFRESH;
    // printf("frames %u\n", frames);

    uint32_t pixelsPerSample = REFRESH * XRES * YRES / samplerate;
    printf("pixels per sample %u\n", pixelsPerSample);



    uint32_t *leftJitterBuf = malloc((pixelsPerSample + UINT16_MAX + 1) * sizeof(uint32_t));
    uint32_t *rightJitterBuf = malloc((pixelsPerSample + UINT16_MAX + 1) * sizeof(uint32_t));
    // sum of first pixelsPerSample should be zero
    memset(leftJitterBuf, 0, pixelsPerSample * sizeof(uint32_t));
    memset(rightJitterBuf, 0, pixelsPerSample * sizeof(uint32_t));
    uint32_t sum = 0;
    uint8_t newVal;
    for (uint16_t sampleVal = 0; sampleVal < UINT16_MAX; sampleVal++) {
        sum -= rightJitterBuf[sampleVal + 1];
        newVal = (pixelsPerSample * sampleVal / 256) - sum;
        sum += newVal;

        leftJitterBuf[pixelsPerSample + sampleVal] = newVal << 16;
        rightJitterBuf[pixelsPerSample + sampleVal] = newVal;
    }
    // last pixelsPerSample should be 0xff
    for (uint16_t sampleVal = UINT16_MAX - pixelsPerSample; sampleVal < UINT16_MAX; sampleVal++) {
        leftJitterBuf[pixelsPerSample + sampleVal] = 0xff << 16;
        rightJitterBuf[pixelsPerSample + sampleVal] = 0xff;
    }

    fd = open("/dev/fb0", O_RDWR, 0644);
    if (fd < 0) {
        perror("open /dev/fb0");
        return 1;
    }

    struct timespec last = {};
    struct timespec now = {};
    struct timespec sync = {
        .tv_sec = 0
    };
    uint64_t nsFramePeriod = 1000 * 1000 * 1000 / REFRESH;

    clock_gettime(CLOCK_MONOTONIC, &last);

    uint32_t pixelIndex = 0;

    uint32_t *frameBuf = malloc(XRES * YRES * 4);
    for (uint32_t pcmIndex = 0; pcmIndex < pcmDataLen; pcmIndex += 2) {
        uint32_t leftSample = (((int32_t)pcmData[pcmIndex]) * SCALE - INT16_MIN); 
        uint32_t rightSample = (((int32_t)pcmData[pcmIndex + 1]) * SCALE - INT16_MIN);
        for (uint32_t i = 0; i < pixelsPerSample; i++) {
            frameBuf[pixelIndex++] = leftJitterBuf[(leftSample & 0xffff) + i] | rightJitterBuf[(rightSample&0xffff) + i];
            if (pixelIndex > XRES * YRES) {
                // pause here
                clock_gettime(CLOCK_MONOTONIC, &now);
                uint64_t nsDiff = (now.tv_sec - last.tv_sec) * 1000000000 + now.tv_nsec - last.tv_nsec;
                sync.tv_nsec = nsFramePeriod - nsDiff;
                nanosleep(&sync, &sync);
                // printf("time diff in ns: %ld\n", (ts_end.tv_sec - ts_start.tv_sec) * 1000000000 + ts_end.tv_nsec - ts_start.tv_nsec);
                clock_gettime(CLOCK_MONOTONIC, &last);
                // break;
                write(fd, frameBuf, XRES * YRES * 4);
                lseek(fd, 0, SEEK_SET);
                pixelIndex = 0;
            }
        }
    }

    return 0;
}

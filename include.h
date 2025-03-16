// Ler Theng Loo (s5212872)
// 2803ICT - Assignment 2

// Header File

#ifndef INCLUDE_H
#define INCLUDE_H

#include <stdint.h>

#define BUF_SIZE 1024
#define BIT_SIZE 32
#define TEST_SET 3
#define SIZE 10
#define NO_DATA 0
#define ACCEPTED 0
#define DATA_READY 1
#define SLOT_READY 2
#define AVAILABLE 0
#define BUSY 32
#define QUIT -1
#define NO_RESPONSE 5.0

struct Memory {
    int number;
    int clientFlag;
    int serverFlag[SIZE];
    int slot[SIZE];
    uint32_t slotFacNum[SIZE]; // To store integer(rotated) of thread that is currently using slot array
    int slotState[SIZE]; // To keep track number of running threads of each request
    uint32_t slotReqNum[SIZE]; // To store requested integer
    double progress[SIZE];
    clock_t resTime[SIZE]; // To store response time of server for each request
};

// Every thread has one to store data
struct RequestData {
    int testMode; 
    int slot;
    int thrNo; // Thread no.
    uint32_t num; // Integer to factorize
    double *progress;
    struct Memory *shmPTR;
};

// For calculating progress percentage purpose
struct ProgressData {
    int *usedSlot;
    double *progress;
    struct Memory *shmPTR;
};

#endif
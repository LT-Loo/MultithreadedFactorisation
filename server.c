// Ler Theng Loo (s5212872)
// 2803ICT - Assignment 2

// Server program

#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <math.h>

#include "include.h"

pthread_mutex_t lock;
pthread_cond_t busy;

// Create and attach shared memory
struct Memory *createShrMemory(key_t *shmKEY, int *shmID) {
    *shmKEY = ftok(".", 'a'); // Get shared memory key
    *shmID = shmget(*shmKEY, sizeof(struct Memory), 0666); // Create shared memory segment
    if (*shmID < 0) {
        printf("Error: Failed to create shared memory segment.\n");
        exit(0);
    } else {printf("Shared memory successfully created.\n");}

    struct Memory *shmPTR;
    shmPTR = (struct Memory *)shmat(*shmID, NULL, 0); // Attached server to shared memory
    if (shmPTR == (void *)-1) {
        printf("Error: Failed to attach shared memory.\n");
        exit(0);
    } else {
        shmPTR->clientFlag = NO_DATA;
        printf("Server successfully attached to shared memory.\n\n");
    }

    return shmPTR;
}


// Write data into shared memory (mainly to slot array)
void writeToShm(int num, struct RequestData *data) {
    while (data->shmPTR->serverFlag[data->slot] != NO_DATA) {continue;} // Wait for shared memory to be available

    if (data->shmPTR->serverFlag[data->slot] == NO_DATA) { // Write data into shared memory
        data->shmPTR->slot[data->slot] = num;
        data->shmPTR->slotFacNum[data->slot] = data->num;
        data->shmPTR->serverFlag[data->slot] = DATA_READY;
    }

    while (data->shmPTR->serverFlag[data->slot] != ACCEPTED) {continue;} // Make sure client has received data
    if (data->testMode == 0) {
        printf("[Query: %d; Slot: %d; Number: %u] Factor successfully sent to client.\n", data->shmPTR->slotReqNum[data->slot], data->slot, data->num);
    } else {printf("[Test: %d; Thread: %d] Value successfully returned to client.\n", data->slot, data->num);}
}


// Thread function - Factorize 32-bits integer
void *factorization(void *dat) {
    struct RequestData data = *(struct RequestData *)dat;
    printf("\n===== FACTORIZATION BEGINS =====\nQuery: %d; Slot: %d; Number: %u\n\n", data.shmPTR->slotReqNum[data.slot], data.slot, data.num);

    uint32_t num = data.num;
    int count = 0, f = 2;
    while (num > 1) { // Factorize
        if (num % f == 0) {
            pthread_mutex_lock(&lock); // Block
            while (data.shmPTR->serverFlag[data.slot] != NO_DATA) {pthread_cond_wait(&busy, &lock);} // Wait for condition variable
            // Critical section
            printf("[Query: %d; Slot: %d; Number: %u] Factor found and sent to client...\n", data.shmPTR->slotReqNum[data.slot], data.slot, data.num);
            writeToShm(f, &data);
            data.progress[data.thrNo] = (double)((++count / sqrt(data.num)) * 100.0); // Calculate progress of thread
            pthread_cond_signal(&busy); // Signal condition variable
            pthread_mutex_unlock(&lock); // Unblock

            num /= f;
            //usleep(1); // Uncomment for faster computation
        } else {f += 1;}
        usleep(5); // Uncomment for slower computation
    }
    // Factorization complete
    pthread_mutex_lock(&lock); // Block
    while (data.shmPTR->serverFlag[data.slot] != NO_DATA) {pthread_cond_wait(&busy, &lock);} // Wait for condition variable
    // Critical section
    data.shmPTR->slotState[data.slot]--;
    writeToShm(-1, &data);
    data.progress[data.thrNo] = 100.0;
    pthread_cond_signal(&busy); // Signal condition variable
    pthread_mutex_unlock(&lock); // Unblock

    printf("\n[Query: %d; Slot: %d; Number: %u] Thread factorization complete! %d threads left...\n", data.shmPTR->slotReqNum[data.slot], data.slot, data.num, data.shmPTR->slotState[data.slot]);
    if (data.shmPTR->slotState[data.slot] == AVAILABLE) {
        printf("\n[Query: %d; Slot: %d] FACTORIZATION COMPLETE!\n\n", data.shmPTR->slotReqNum[data.slot], data.slot);
    }
    return NULL;
}


// Thread function - Calculate progress of requests
void *calcProgress(void *dat) {
    struct ProgressData *pData = (struct ProgressData *)dat;
    while (1) {
        for (int i = 0; i < SIZE; i++) { // Loop through slot array
            if (pData->usedSlot[i] != 0) { // If slot busy - running request
                float total = 0;
                for (int j = 0; j < BIT_SIZE; j++) {total += pData->progress[i * BIT_SIZE + j];} // Calculate total progress of threads
                pData->shmPTR->progress[i] = total / BIT_SIZE; // Update progress of request

                if (total / BIT_SIZE >= 100.0) {pData->usedSlot[i] = 0;}
            }    
        }
    }
    return NULL;
}


// Thread function - Test mode
void *testModeStm(void *dat) {
    srand(time(NULL));
    struct RequestData data = *(struct RequestData *)dat;
    printf("\n===== START TO PROCESS TEST MODE%d =====\nTest: %d; Thread: %d\n\n", data.slot, data.num);

    for (int i = 0; i < SIZE; i++) {
        int num = data.num * SIZE + i; // Calculate return value
        pthread_mutex_lock(&lock); // Block
        while (data.shmPTR->serverFlag[data.slot] != NO_DATA) {pthread_cond_wait(&busy, &lock);} // Wait for condition variable
        // Critical section
        writeToShm((num * -1) - SIZE, &data);
        if (i == SIZE - 1) {data.shmPTR->slotState[data.slot]--;}
        int delay = (rand() % (100 + 1 - 10)) + 10; // Random delay
        usleep(delay);
        pthread_cond_signal(&busy); // Signal condition variable
        pthread_mutex_unlock(&lock); // Unblock
    }
    printf("[Test: %d; Thread: %d] THREAD TEST COMPLETE\n", data.slot, data.num);
    if (data.shmPTR->slotState[data.slot] == AVAILABLE) {printf("\n===== TEST %d COMPLETE =====\n\n", data.slot);}
    
    return NULL;
}


// Main program
int main()
{int usedSlot[SIZE] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
 double progress[BIT_SIZE * SIZE];

 key_t shmKEY;
 int shmID;
 struct Memory *shmPTR = createShrMemory(&shmKEY, &shmID); // Create and attach shared memory

 if (pthread_mutex_init(&lock, NULL) != 0) { // Initialize mutex lock
    printf("Error: Failed to initialise mutex.\n");
    exit(0);
 } else {printf("Initialisation of mutex success.\n");}
 busy = PTHREAD_COND_INITIALIZER; // Initialize condition variable
 
 pthread_t tid[BIT_SIZE * SIZE]; // Threads for factorization
 pthread_t tidTest[TEST_SET * SIZE]; // Threads for test mode

 struct ProgressData pData;
 pData.usedSlot = usedSlot;
 pData.progress = progress;
 pData.shmPTR = shmPTR;
 pthread_t progressThr;
 pthread_create(&progressThr, NULL, calcProgress, (void *)&pData); // Create thread to calculate progress percentage

 // Run exact server program
 while (1) {
    while (shmPTR->clientFlag != DATA_READY) {continue;} // Wait for client to send data
    if (shmPTR->clientFlag == DATA_READY) { // Data received from client
        if (shmPTR->number < 0) {break;} // Quit program

        printf("\nRequest from client accepted. Processing client's request...\n\n");
        if (shmPTR->number == 0) { // Test mode
            printf("===== TEST MODE ACTIVATED =====\n");
            for (int i = 0; i < TEST_SET; i++) { // Loop through sets
                shmPTR->slotState[i] = SIZE;
                for (int j = 0; j < SIZE; j++) { // Create threads for each set
                    struct RequestData data;
                    data.testMode = 1;
                    data.slot = i;
                    data.num = j;
                    data.shmPTR = shmPTR;
                    pthread_create(&tid[i * SIZE + j], NULL, testModeStm, (void *)&data);
                    usleep(10);
                }               
            }
            shmPTR->clientFlag = ACCEPTED; // Notify client that data has been read
        }
        else { // Factorization
            for (int i = 0; i < SIZE; i++) {
                if (shmPTR->slotState[i] == 0) { // Search for unoccupied slot                  
                    shmPTR->slotReqNum[i] = shmPTR->number;
                    shmPTR->slotState[i] = BUSY;
                    for (int j = 0; j < BIT_SIZE; j++) { // Create 32 threads for request
                        struct RequestData data;
                        data.testMode = 0;
                        data.num = shmPTR->slotReqNum[i] >> j | shmPTR->slotReqNum[i] << BIT_SIZE - j; // Rotate integer
                        data.slot = i;
                        data.thrNo = i * BIT_SIZE + j;
                        data.shmPTR = shmPTR;
                        progress[i * BIT_SIZE + j] = 0.0;
                        data.progress = progress;
                        pthread_create(&tid[i * BIT_SIZE + j], NULL, factorization, (void *)&data);   
                        usleep(10);
                    }
                    shmPTR->number = i;
                    shmPTR->clientFlag = SLOT_READY; // Notify client that slot number has been sent
                    usedSlot[i] = 1;
                    break;
                }
            } 
        }
    } 
 }

 // Close program
 printf("\n\nUser requested to quit program...\nClosing threads...\n");
 for (int i = 0; i < BIT_SIZE * SIZE; i++) {
     if (i < TEST_SET * SIZE) {pthread_cancel(tidTest[i]);}
     pthread_cancel(tid[i]);
 }

 printf("Clearing mutex lock...\n");
 pthread_mutex_destroy(&lock);

 printf("Detaching from shared memory...\n");
 if (shmdt((void *) shmPTR) < 0) {
     printf("Error: Failed to detach shared memory from server.\n");
     exit(0);
 }
 printf("Terminating server program...\n");

 return 0;
}
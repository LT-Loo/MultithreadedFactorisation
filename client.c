// Ler Theng Loo (s5212872)
// 2803ICT - Assignment 2

// Client program

#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>

#include "include.h"

int testMode = 2; // Random number that is not 0 and 1 to indicate the program has just launched

// Check validity of user input
long int checkValidity(char *input) {
   char *endPTR;
   long int longNum = strtoll(input, &endPTR, 10);

   if ((endPTR == input) || ((*endPTR != 0) && !isspace(*endPTR))) {return -1;} // If input invalid
   if (longNum < 0 || longNum > UINT_MAX) {return -2;} // If overflow

   return longNum;
}


// Create and attach shared memory
struct Memory *createShrMemory(key_t *shmKEY, int *shmID) {
    *shmKEY = ftok(".", 'a'); // Get shared memory key
    *shmID = shmget(*shmKEY, sizeof(struct Memory), IPC_CREAT | 0666); // Create shared memory segment
    if (*shmID < 0) {
        printf("Error: Failed to create shared memory segment.\n");
        exit(0);
    } else {printf("Shared memory successfully created.\n");}

    struct Memory *shmPTR;
    shmPTR = (struct Memory *)shmat(*shmID, NULL, 0); // Attach client to shared memory
    if (shmPTR == (void *)-1) {
        printf("Error: Failed to attach shared memory.\n");
        exit(0);
    } 
    else { // Initialize some variables in shared memory
        shmPTR->clientFlag = NO_DATA;
        for (int i = 0; i < SIZE; i++) {
            shmPTR->serverFlag[i] = NO_DATA;
            shmPTR->progress[i] = 0.0;
            shmPTR->slotState[i] = AVAILABLE;
        }
        printf("Client successfully attached to shared memory.\n");
    }
    return shmPTR;
}


// Send request to server
void sendRequest(struct Memory* shmPTR, uint32_t value) {
    while (shmPTR->clientFlag != NO_DATA) {continue;} // Make sure 'number' is available
    if (shmPTR->clientFlag == NO_DATA) {
        shmPTR->number = value;
        shmPTR->clientFlag = DATA_READY; // Notify server data is ready
        printf("Request successfully sent to server.\n");
    }  
}


// Thread function - Read from shared memory (mainly from slot array)
void *readShm(void *arg) {
    struct Memory *shmPTR = (struct Memory *)arg;

    while (1) {
        for (int i = 0; i < SIZE; i++) { // Loop through slot array
            if (shmPTR->serverFlag[i] == DATA_READY) { // If data is ready in slot
                if (testMode == 1) { // Test mode
                    printf("[Test Set %d|%d] Return Value: %d\n", i, shmPTR->slotFacNum[i], abs(shmPTR->slot[i] + SIZE));
                    if (shmPTR->slotState[i] - 1== AVAILABLE) {
                        printf("\n[Test Set %d] Stimulation complete! Response Time: %.3fs\n\n", i, (double)(clock()-shmPTR->resTime[i])/CLOCKS_PER_SEC);
                    }
                }
                else { // Factorisation
                    if (shmPTR->slot[i] == -1 && shmPTR->slotState[i] == AVAILABLE) {
                        printf("\n[Query: %u] Factorization complete! Response Time: %.3fs\n\n", shmPTR->slotReqNum[i], (double)(clock()-shmPTR->resTime[i])/CLOCKS_PER_SEC); 
                    }
                    else if (shmPTR->slot[i] > 0) {
                        printf("[Query: %u|%u] Factor = %d\n", shmPTR->slotReqNum[i], shmPTR->slotFacNum[i], shmPTR->slot[i]);
                    }
                }

                shmPTR->serverFlag[i] = ACCEPTED; // Notify server that data has been read
            }
        }
    }
    return NULL;
}


// Thread function - Get and print progress percentage of each request (Only factorization)
void *printProgress(void *arg) {
    struct Memory *shmPTR = (struct Memory *)arg;
    clock_t startTime = clock(); // Start timer
    while (1) {
        int response = 0, running = 0;
        for (int i = 0; i < SIZE; i++) { // Check if there's response from server / user enters input
            if (shmPTR->serverFlag[i] == DATA_READY) {response = 1; break;}
            if (shmPTR->slotState[i] != AVAILABLE) {running = 1; break;}
        }

        double interval = ((double)(clock() - startTime) / CLOCKS_PER_SEC);
        if (interval >= NO_RESPONSE) { // If time out
            if (testMode == 0 && shmPTR->clientFlag == NO_DATA && response == 0) { // If no server response & no user input
                if (running == 0) {printf("\n[Progress] No factorization running.\n");} // Server not running any request
                else {
                    for (int j = 0; j < SIZE; j++) { // Search for working slots 
                        if (shmPTR->slotState[j] != AVAILABLE) { // If slot working, print request progress
                            printf("\n[Progress] Query %d (Slot %d): %.4f%% |", shmPTR->slotReqNum[j], j, shmPTR->progress[j]);
                            int k = 0;
                            while (k <= 100) {
                                if (k <= shmPTR->progress[j]) {printf("\u258A");} 
                                else {printf("\u25A2");}
                                k += 5;
                            }
                            printf("|\n");
                        }
                    }
                    printf("\n");
                }       
            }
            startTime = clock(); // Restart timer
        }        
    }
    return NULL;
}


// Check if server is busy (running 10 requests)
int serverBusy(struct Memory *shmPTR) {
    for (int i = 0; i < SIZE; i++) {
        if (shmPTR->slotState[i] == AVAILABLE) {return 0;}
    }
    return 1;
}


// Main program
int main()
{char input[BUF_SIZE];

 key_t shmKEY;
 int shmID;
 struct Memory *shmPTR = createShrMemory(&shmKEY, &shmID); // Create and attached shared memory

 pthread_t getFacThr; // Create thread for reading slot array (Get factors)
 if (pthread_create(&getFacThr, NULL, readShm, (void *)shmPTR) != 0) {
     printf("Error: Fail to create thread for data reading.\n");
     exit(0);
 }

 pthread_t progressThr; // Create thread for printing progress percentage
 if (pthread_create(&progressThr, NULL, printProgress, (void *)shmPTR) != 0) {
     printf("Error: Fail to create thread for data reading.\n");
     exit(0);
 }

 // Run exact client program
 do {
     printf("\nEnter any 32-bit integers (or q to quit): ");
     fgets(input, BUF_SIZE, stdin); // Get user input
     input[strlen(input) - 1] = '\0';

     if (strcmp(input, "q") == 0) { // Quit program
         sendRequest(shmPTR, QUIT);
         break;
     }

     if (serverBusy(shmPTR)) { // Check if server is busy (running 10 requests)
         printf("Warning: Sorry, server is busy! Please try later.\n");
         continue;
     } 

     long int check = checkValidity(input); // Check validity of user input
     if (check == -1) {
         printf("Error: Invalid input. Please try again.\n"); 
         continue;
     }
     else if (check == -2) {
         printf("Error: Overflow occured. Please try again.\n");
         continue;
     }
     else {
         uint32_t num = (uint32_t)check;
         if (num == 0) { // Test mode
            int valid = 1;
            for (int i = 0; i < SIZE; i++) { // Check if server running any request
                if (shmPTR->slotState[i] != AVAILABLE) {
                    valid = 0;
                    printf("Warning: Server is processing requests. Test Mode can only be activated when server is free.\n");
                    break;
                }
            }
            if (valid == 1) { // Server is free
                testMode = 1;
                printf("\n===== TEST MODE ACTIVATED =====\n\n");
                sendRequest(shmPTR, num);
                for (int i = 0; i < TEST_SET; i++) {shmPTR->resTime[i] = clock();} // Start timer
                while (shmPTR->clientFlag != NO_DATA) {continue;} // Make sure request has been received
            }    
         }
         else {
            testMode = 0;
            sendRequest(shmPTR, num);
            while (shmPTR->clientFlag != SLOT_READY) {continue;} // Wait for slot number from server
            if (shmPTR->clientFlag == SLOT_READY) { // Receive slot number
                shmPTR->resTime[shmPTR->number] = clock(); // Start timer for request
                shmPTR->clientFlag = ACCEPTED; // Notify server that slot number has been read
            }
        }    
     }
     
 } while (strcmp(input, "q") != 0);

 // Close program
 printf("\n\nUser requested to quit program...\nClosing threads...\n");
 pthread_cancel(getFacThr);
 pthread_cancel(progressThr);

 printf("Detaching from shared memory...\n");
 if (shmdt((void *) shmPTR) < 0) {
     printf("Error: Failed to detach shared memory from client.\n");
     exit(0);
 } 
 printf("Terminating client program...\n");

 return 0;
}
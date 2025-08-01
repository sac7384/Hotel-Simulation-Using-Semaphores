#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

//Constant variables for number of people in simulation
int FRONT_DESK_EMPLOYEES = 2, fdCount = 0;
int BELLHOPS = 2, bhCount = 0;
int GUESTS = 25, gCount = 0;

//Other variables
pthread_t* guestThreads = new pthread_t[GUESTS];    //These arrays let main join guest threads once they finish
int* finishedThreads = new int[GUESTS];
int* deskCounter = new int[2];      //Employee and guest communicate using the front desk counter they are at
int* bellhopDesk = new int[2];      //Bellhop and guest communicate using the bellhop desk

//Semaphores
sem_t start;            //Allows main to create threads one at a time so there is no overlap
sem_t deskLine1;        //Guests wait for a desk employee here
sem_t deskLine2;
sem_t employeeSem1;     //Used by employee during room assignments
sem_t employeeSem2;
sem_t deskGuestSem1;    //Used by guest during room assignments
sem_t deskGuestSem2;

sem_t bellhopLine1;     //Guests wait for a bellhop here
sem_t bellhopLine2;
sem_t bellhopSem1;      //Used by bellhop during bag transfer
sem_t bellhopSem2;
sem_t hopGuestSem1;     //Used by guest during bag transfer
sem_t hopGuestSem2;

void* frontDesk(void* arg) {
    int id = fdCount;
    printf("Front desk employee %d created\n", id);
    sem_post(&start);
    //Employee's give out keys from different parts of the hotel so they never choose the same room
    int nextOpenRoom = id * GUESTS + 1;
    int guestNum;

    //Employee 0
    if (id == 0) {
        while (true) {
            sem_post(&deskLine1);
            sem_wait(&employeeSem1);    //Wait for guest number
            guestNum = deskCounter[0];
            printf("Front desk employee %d registers guest %d and assigns room %d\n", 0, guestNum, nextOpenRoom);
            deskCounter[0] = nextOpenRoom;
            nextOpenRoom++;
            sem_post(&deskGuestSem1);
            sem_wait(&employeeSem1);    //Wait for guest to take room key
        }
    }
    //Employee 1
    else {
        while (true) {
            sem_post(&deskLine2);
            sem_wait(&employeeSem2);    //Wait for guest number
            guestNum = deskCounter[1];
            printf("Front desk employee %d registers guest %d and assigns room %d\n", 1, guestNum, nextOpenRoom);
            deskCounter[1] = nextOpenRoom;
            nextOpenRoom++;
            sem_post(&deskGuestSem2);
            sem_wait(&employeeSem2);    //Wait for guest to take room key
        }
    }
}//End of front desk employee

void* bellhop(void* arg) {
    int id = bhCount;
    printf("Bellhop %d created\n", id);
    sem_post(&start);

    int guestNum;
    //Bellhop 0
    if (id == 0) {
        while (true) {
            sem_post(&bellhopLine1);
            sem_wait(&bellhopSem1);    //Waiting on guest number
            guestNum = bellhopDesk[0];
            printf("Bellhop %d receives bags from guest %d\n", 0, guestNum);
            sem_post(&hopGuestSem1);
            sem_wait(&bellhopSem1);     //Waiting on guest to enter room
            printf("Bellhop %d delivers bags to guest %d\n", 0, guestNum);
            sem_post(&hopGuestSem1);
            sem_wait(&bellhopSem1);     //Wait for tip
        }
    }
    //Bellhop 1
    else {
        while (true) {
            sem_post(&bellhopLine2);
            sem_wait(&bellhopSem2);    //Waiting on guest number
            guestNum = bellhopDesk[1];
            printf("Bellhop %d receives bags from guest %d\n", 1, guestNum);
            sem_post(&hopGuestSem2);
            sem_wait(&bellhopSem2);     //Waiting on guest to enter room
            printf("Bellhop %d delivers bags to guest %d\n", 1, guestNum);
            sem_post(&hopGuestSem2);
            sem_wait(&bellhopSem2);     //Wait for tip
        }
    }
}//End of bellhop

void* guest(void* arg) {
    int id = gCount; int bags = rand() % 5;
    printf("Guest %d created\n", id);
    sem_post(&start);

    //Guest waits until they can get their room number
    int roomNum;
    printf("Guest %d enters hotel with %d bag(s)\n", id, bags);
    if (id % 2 == 0) {
        sem_wait(&deskLine1);       //Waiting in line
        deskCounter[0] = id;
        sem_post(&employeeSem1);
        sem_wait(&deskGuestSem1);   //Waiting for room key
        roomNum = deskCounter[0];
        sem_post(&employeeSem1);
    }
    else {
        sem_wait(&deskLine2);       //Waiting in line
        deskCounter[1] = id;
        sem_post(&employeeSem2);
        sem_wait(&deskGuestSem2);   //Waiting for room key
        roomNum = deskCounter[1];
        sem_post(&employeeSem2);
    }

    //Guest gives bags to bellhop
    if (bags > 2) {
        printf("Guest %d requests help with bags\n", id);
        if (id % 2 == 0) {
            sem_wait(&bellhopLine1);    //Waiting for available bellhop
            bellhopDesk[0] = id;
            sem_post(&bellhopSem1);
            sem_wait(&hopGuestSem1);    //Waiting for bellhop to take bags
        }
        else {
            sem_wait(&bellhopLine2);    //Waiting for available bellhop
            bellhopDesk[1] = id;
            sem_post(&bellhopSem2);
            sem_wait(&hopGuestSem2);    //Waiting for bellhop to take bags
        }
    }

    //Guest heads to room
    printf("Guest %d enters room %d\n", id, roomNum);

    //Guest gets bags and gives tip to bellhop
    if (bags > 2) {
        if (id % 2 == 0) {
            sem_post(&bellhopSem1);
            sem_wait(&hopGuestSem1);    //Wait for bellhop to give bags
            printf("Guest %d receives bags from bellhop %d and gives tip\n", id, 0);
            sem_post(&bellhopSem1);     //Give tip to bellhop
        }
        else {
            sem_post(&bellhopSem2);
            sem_wait(&hopGuestSem2);    //Wait for bellhop to give bags
            printf("Guest %d receives bags from bellhop %d and gives tip\n", id, 1);
            sem_post(&bellhopSem2);     //Give tip to bellhop
        }
    }

    printf("Guest %d retires for the evening\n", id);
    finishedThreads[id] = 1;    //Let main know this thread is finished and ready to join
    return NULL;
}//End of guest

int main() {
    srand(time(0));
    printf("Simulation starts\n");
    pthread_t ptid;
    sem_init(&start, 0, 0);
    sem_init(&deskLine1, 0, 0);
    sem_init(&deskLine2, 0, 0);
    sem_init(&employeeSem1, 0, 0);
    sem_init(&employeeSem2, 0, 0);
    sem_init(&deskGuestSem1, 0, 0);
    sem_init(&deskGuestSem2, 0, 0);

    sem_init(&bellhopLine1, 0, 0);
    sem_init(&bellhopLine2, 0, 0);
    sem_init(&bellhopSem1, 0, 0);
    sem_init(&bellhopSem2, 0, 0);
    sem_init(&hopGuestSem1, 0, 0);
    sem_init(&hopGuestSem2, 0, 0);

    //Creating all front desk employees, bellhops, and guests
    while (fdCount < FRONT_DESK_EMPLOYEES) {
        pthread_create(&ptid, NULL, &frontDesk, NULL);
        sem_wait(&start);
        fdCount++;
    }
    while (bhCount < BELLHOPS) {
        pthread_create(&ptid, NULL, &bellhop, NULL);
        sem_wait(&start);
        bhCount++;
    }
    while (gCount < GUESTS) {
        pthread_create(&ptid, NULL, &guest, NULL);
        guestThreads[gCount] = ptid;
        finishedThreads[gCount] = 0;
        sem_wait(&start);
        gCount++;
    }

    //Searches through the thread arrays for threads that are ready to join
    while (gCount > 0) {
        for (int i = 0; i < GUESTS; i++) {
            if (finishedThreads[i] == 1) {
                printf("Guest %d joined\n", i);
                pthread_join(guestThreads[i], NULL);
                finishedThreads[i] = 0;
                gCount--;
            }
        }
    }
    printf("Simulation ends\n");
}//End of main

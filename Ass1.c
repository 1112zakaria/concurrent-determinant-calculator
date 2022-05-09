#include <stdio.h>

#include <stdlib.h>

#include <unistd.h>

#include <string.h>

#include <sys/shm.h>

#include <sys/time.h>

#include <sys/types.h>

#include <errno.h>

#include <time.h>



#define MICRO_SEC_IN_SEC 1000000



struct shared_use_st {

	int M[3][3];

    //int **M;

    int D[3];

    int L[3];

    int largest, det;

};



int largest(int A, int B, int C){

    if (A >= B && A >= C)

        return A;

    if (B >= A && B >= C)

        return B;

    if (C >= A && C >= B)

        return C;

    return -1;

}







int main(){

    struct timeval start, end;

	int pid;



    //parent should fork all 3 process

    int i=1;



    while(i!=4){

        pid = fork();

        if(pid == 0){ 

            break;

        }

        i++;

    }





    //start the timer

    gettimeofday(&start, NULL);



    //allocating shared memo in every single child or only once? 

    void *shared_memory = (void *)0;

	struct shared_use_st *shared_stuff; //casting 

	int shmid;



    srand((unsigned int)getpid());    

	

	shmid = shmget((key_t)1234, sizeof(struct shared_use_st), 0666 | IPC_CREAT);

	

	if (shmid == -1) {

		fprintf(stderr, "shmget failed\n");

		exit(EXIT_FAILURE);

	}



	//make shared memo accessable 

	shared_memory = shmat(shmid, (void *)0, 0);

	if (shared_memory == (void *)-1) {

		fprintf(stderr, "shmat failed\n");

		exit(EXIT_FAILURE);

	}

	printf("Memory attached at %X\n", (struct shared_use_st*)shared_memory);



	shared_stuff = (struct shared_use_st *)shared_memory;



    //initilizing the matrix M

    shared_stuff->M[0][0]= 20;

    shared_stuff->M[1][0]= 20;

    shared_stuff->M[2][0]= 50;



    shared_stuff->M[0][1]= 10;

    shared_stuff->M[1][1]= 6;

    shared_stuff->M[2][1]= 70;



    shared_stuff->M[0][2]= 40;

    shared_stuff->M[1][2]= 3;

    shared_stuff->M[2][2]= 2;

    

    //shared_stuff->M = {{20, 20, 50}, {10,6,70}, {40, 3, 2}};





    switch(pid){

        case -1:

            perror("fork failed");

            exit(1);

        case 0:

            printf("child[%d] --> pid = %d and ppid = %d\n", i, getpid(), getppid());

            

            if(i == 1){

                printf("“Child Process [%d]: working with element [%d] of D",i, i);

                shared_stuff -> D[0]= (shared_stuff -> M[0][0]) *(shared_stuff -> M[1][1] * shared_stuff -> M[2][2] - shared_stuff -> M[1][2] * shared_stuff -> M[2][1]);

                shared_stuff -> L[0]= largest(shared_stuff->M[0][0],shared_stuff->M[0][1],shared_stuff->M[0][2]);

            }



            if(i == 2){

                printf("“Child Process [%d]: working with element [%d] of D",i, i);

                shared_stuff -> D[1]= (shared_stuff -> M[0][1]) *(shared_stuff -> M[1][2] * shared_stuff -> M[2][0] - shared_stuff -> M[1][0] * shared_stuff -> M[2][2]);

                shared_stuff -> L[1]= largest(shared_stuff->M[1][0],shared_stuff->M[1][1],shared_stuff->M[1][2]);

            }



            if(i == 3){

                printf("“Child Process [%d]: working with element [%d] of D",i, i);

                shared_stuff -> D[2]= (shared_stuff -> M[0][2]) *(shared_stuff -> M[1][0] * shared_stuff -> M[2][1] - shared_stuff -> M[1][1] * shared_stuff -> M[2][0]);

                shared_stuff -> L[2]= largest(shared_stuff->M[2][0],shared_stuff->M[2][1],shared_stuff->M[2][2]);

            }

            break;

           

        default:

            printf("This is the parent :3\n");

            printf("parent --> pid = %d\n", getpid());

            shared_stuff -> det = shared_stuff->D[0]+shared_stuff->D[1]+shared_stuff->D[2];

            shared_stuff -> largest = shared_stuff->L[0]+ shared_stuff->L[1] +shared_stuff->L[2];

            printf("The det value of the matrix M equals: %d\n", shared_stuff->det);

            printf("The largest value of the matrix M equals: %d\n", shared_stuff->largest);

    }







    //End the timer

    gettimeofday(&end, NULL);



    printf("Start Time: %lf sec from Epoch (1970‐01‐01 00:00:00 +0000 (UTC))\n", start.tv_sec + (double)start.tv_usec/MICRO_SEC_IN_SEC);



    printf("End Time: %lf sec from Epoch (1970‐01‐01 00:00:00 +0000 (UTC))\n", end.tv_sec + (double) end.tv_usec/MICRO_SEC_IN_SEC);



    printf("\nElapsed Time: %ld micro sec\n", ((end.tv_sec * MICRO_SEC_IN_SEC + end.tv_usec - (start.tv_sec * MICRO_SEC_IN_SEC + start.tv_usec))));

   

    if (shmdt(shared_memory) == -1) {	

        fprintf(stderr, "shmdt failed\n");	

        exit(EXIT_FAILURE);

    }

    if (shmctl(shmid, IPC_RMID, 0) == -1){

        fprintf(stderr, "shmctl(IPC_RMID) failed\n");

        exit(EXIT_FAILURE);	

    }



}
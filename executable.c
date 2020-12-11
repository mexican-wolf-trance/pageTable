#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <signal.h>
#include <time.h>

#define SEC_KEY 0x1234567
#define MSG_KEY 0x2345
#define RES_KEY 0x7654
#define SEM_KEY 0x1111

//Initialize the shared clock, total resource tracker, and message queue
typedef struct Clock
{
        unsigned int sec;
        unsigned int nsec;
} Clock;

struct msgbuf
{
        long mtype;
        int mpageReq;
	int mWrite;
	pid_t pid;
} message;

int main()
{
	int i, msgqid, decision, dead_flag = 0, pageCall = 0;
//	int shmid;
//	long long current_time = 0, time_check = 0;
//	struct Clock *sim_clock;
	time_t t;
	pid_t pid;
	
	//Get the keys to the queue and shared memory	
        msgqid = msgget(MSG_KEY, 0644 | IPC_CREAT);
        if (msgqid == -1)
        {
                perror("msgqid get failed");
                return 1;
        }


//	shmid = shmget(SEC_KEY, sizeof(Clock), 0644 | IPC_CREAT);
//        if (shmid == -1)
//        {
//                perror("shmid get failed");
//                return 1;
//        }
	
	//Access the shared clock
//        sim_clock = (Clock *) shmat(shmid, NULL, 0);
//        if (sim_clock == (void *) -1)
//        {
//                perror("clock get failed");
//                return 1;
//        }

	pid = getpid();	
	//Seed the random number generator and grab a number between 0 and 50,000,000)
	srand((int)time(&t) % pid);
	
	//The main show
	i = 0;
	while(!dead_flag)
	{
		pageCall++;
		decision = rand() % 32767;
		message.mtype = 3;
		message.pid = pid;
		message.mpageReq = decision;
		if(decision % 20 <= 5)
			message.mWrite = 1;
		else
			message.mWrite = 0;
		msgsnd(msgqid, &message, sizeof(message), 0);
		if(msgrcv(msgqid, &message, sizeof(message), pid, 0) > 0)
		{
			if(message.mpageReq)
			{
				printf("Child %li acknowledges page fault\n", (long) pid);
				sleep(0.5);
			}
			if(pageCall % 15 == 0)
			{
				if(rand() % 20 <= 5)
					dead_flag = 1;
			}
		}		
	}
        printf("Child %ld decision %d\n", (long) pid, decision);
        message.mtype = 2;
        message.pid = pid;
        msgsnd(msgqid, &message, sizeof(message), 0);
        exit(0);

}

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
	int i, shmid, msgqid, semid, res_no, duration, decision, request_flag = 0, decision_flag = 0, dead_flag = 0, resChoice, requested[20];
	long long current_time = 0, time_check = 0;
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
		
		
//		if(decision > 2 && decision <= 70 && ((current_time + duration) <= time_check))
//		{
//			while(1)
//			{
//				if(requested[resChoice] != allocatedRes->usedResources[resChoice] && requested[resChoice] > 0)
//				{
//					res_no = ((rand() % requested[resChoice]) + 1) - allocatedRes->usedResources[resChoice];
//					printf("Child %ld is requesting %d resource!\n", (long) pid, resChoice);
//					message.mtype = 3;
//             	        		message.pid = pid;
//					message.mresReq = resChoice;
//					message.mresNo = res_no;
//             	        		msgsnd(msgqid, &message, sizeof(message), 0);
//					if(msgrcv(msgqid, &message, sizeof(message), pid, 0) > 0)
//					{
//						allocatedRes->usedResources[resChoice] += res_no;
//						decision_flag = 0;
//						request_flag = 1;
//					}
//				}
//				if(requested[resChoice] == 0)
//					resChoice = (rand() % 20);
//				if(!decision_flag)
//					break;
//			}
//		}
//             	if(request_flag && decision > 70 && decision <= 100 && ((current_time + duration) <= time_check))
//             	{
//			while(1)
//			{
//				i = 0;
//				while(i < 20)
//				{
//					if((i == resChoice))
//					{
//             	        			printf("Child %ld is releasing resource %d!\n", (long) pid, resChoice);
//             	        			message.mtype = 4;
//             	        			message.pid = pid;
//						message.mresReq = resChoice;
//						message.mresNo = allocatedRes->usedResources[resChoice];
//             	        			msgsnd(msgqid, &message, sizeof(message), 0);
//						if(msgrcv(msgqid, &message, sizeof(message), pid, 0) > 0)
//						{
//							allocatedRes->usedResources[resChoice] = 0;
//							decision_flag = 0;
//							break;
//						}
//					}
//					i++;
//				}
//				resChoice = (rand() % 20);
//				if(!decision_flag)
//					break;
//			}
//             	}
//		decision_flag = 0;
                printf("Child %ld decision %d\n", (long) pid, decision);
                message.mtype = 2;
                message.pid = pid;
                msgsnd(msgqid, &message, sizeof(message), 0);
                exit(0);
	
	}
}


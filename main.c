#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <signal.h>
#include <time.h>

#define MAXCHILD 18
#define BUFSIZE 1024
#define SEC_KEY 0x1234567
#define MSG_KEY 0x2345
#define RES_KEY 0x7654
#define SEM_KEY 0x1111

//Initialize the message and clock structs!
struct msgbuf
{
	long mtype;
	int mpageReq;
	int mWrite;
	pid_t pid;
} message;

typedef struct PF
{
	unsigned ref;
	int page;
	pid_t pid;
	char dirty;
} PF;

typedef struct Clock
{
	double sec;
	double nsec;
} Clock;

typedef struct PCB
{
	pid_t pid;
	double access;
	double pageFault;
	double faultsPS;
	double accessSpeed;
	double accessPerSecond;
} PCB;

//I intitalize the ids and the clock pointer here so my signal handler can use them
int shmid, msgqid, semid, vector[MAXCHILD];
struct Clock *sim_clock;
struct Clock *new_proc_clock;
struct PF frame[256];
struct PCB pcb[MAXCHILD];
FILE *fp;

//set next process fork time
void newProcTime()
{
	new_proc_clock->nsec = sim_clock->nsec + ((rand() % 500) * 100000);
	new_proc_clock->sec = sim_clock->sec;
}
//check if time for a new process to begin
int checkProcTime()
{
	if((new_proc_clock->nsec + new_proc_clock->sec * 1000000000) <= (sim_clock->nsec + sim_clock->sec * 1000000000))
	{
		newProcTime();
		return 1;
	}
	return 0;
}

int findVectorIndex(pid_t pid, int vector[MAXCHILD])
{
	int i;
	for(i = 0; i < MAXCHILD; i++)
	{
		if(vector[i] == pid)
			return i;
	}
	return 0;
}

int findPFindex(PF *pg)
{
	int i;
	for(i = 0; i < 256; i++)
	{
		if(pg[i].pid == 0)
			return i;
	}
	return 0;
}

int insertPid(PCB *pcb, pid_t pid)
{
	int i;
	for(i = 0; i < MAXCHILD; i++)
	{
		if(pcb[i].pid == 0)
		{
			pcb[i].pid = pid;
			return 1;
		}
	}
	return 0;
}

int findPCBindex(PCB *pcb, pid_t pid)
{
        int i;
        for(i = 0; i < MAXCHILD; i++)
        {
		printf("Process %d pid: %li ", i, (long) pcb[i].pid);
                if(pcb[i].pid == 0)
                        return i;
        }
	printf("\n");
        return 0;
}

char *isOccupied(PF *fr, int i)
{
	char occ[5];
	if(fr[i].pid)
		return strcpy(occ, "Yes");
	return strcpy(occ, "No");
}

//The signal handler!
void sigint(int sig)
{
	if(fp != NULL)
		fclose(fp);
        if (msgctl(msgqid, IPC_RMID, NULL) == -1)
                fprintf(stderr, "Message queue could not be deleted\n");

        shmdt(sim_clock);
        shmctl(shmid, IPC_RMID, NULL);
	if(sig == SIGALRM)
		write(1, "\nAlarm! Alarm!\n", 15);
	else if(sig == SIGSEGV)
		write(1, "\nWoops! You got a segmentation fault!\n", 38);
	else if(sig == SIGINT)
		write(1, "\nCTRL C was hit!\n", 17);
	else if(sig == SIGFPE)
		write(1, "Floating point exception occurred ya dingus!\n", 46); 
	else
		write(1, "Oh no! An error!\n", 17);
	write(1, "Now killing the kiddos\n", 23);
	kill(0, SIGQUIT);
}


//The main function
int main (int argc, char **argv)
{
	//The signal handlers!
	signal(SIGINT, sigint);
	signal(SIGSEGV, sigint);
	signal(SIGFPE, sigint);
        if((fp = fopen("log.out", "w")) == 0)
        {
                perror("log.out");
                return 0;
        }
        fprintf(fp, "%s", "Program start\n");


	int i, index = 0,  option, max_time = 2, counter = 0, tot_proc = 0, vOpt = 0, pageTable[18][32], numCalls = 0, request;
	pid_t pidvector[MAXCHILD];
	char *exec[] = {"./user", NULL, NULL};
	pid_t child = 0;
	//Getopt is great!
        while ((option = getopt(argc, argv, "hvm:")) != -1)
        switch (option)
        {
                case 'h':
                        printf("This is the operating system simulator!\n");
                        printf("Usage: ./oss\n");
			printf("Use the -m to select page request complexity: 0 = simple, 1 = complex");
                        printf("For a more verbose output file, add the -v option\n");
                        printf("Enjoy!\n");
                        return 0;
                case 'v':
                        vOpt = 1;
                        break;
		case 'm':
			exec[1] = optarg;
			break;
                case '?':
                        printf("%c is not an option. Use -h for usage details\n", optopt);
                        return 0;
        }
        if (argc > 3)
        {
                printf("Invalid usage! Check the readme\nUsage: ./oss\n");
                printf("Use -h to see the correct useage\n");
                return 0;
        }

	srand(time(NULL) + getpid());
	//Get my shared memory and message queue keys
	shmid = shmget(SEC_KEY, sizeof(Clock), 0644 | IPC_CREAT);
	if (shmid == -1)
	{
		perror("shmid get failed");
		return 1;
	}
	
	//Put the clock in shared memory
	sim_clock = (Clock *) shmat(shmid, NULL, 0);
	if (sim_clock == (void *) -1)
	{
		perror("clock get failed");
		return 1;
	}

	//Message queue key
	msgqid = msgget(MSG_KEY, 0644 | IPC_CREAT);
        if (msgqid == -1)
        {
                perror("msgqid get failed");
                return 1;
        }
	
	//Begin the alarm. Goes off after the -t amount of time expires
	alarm(max_time);
	signal(SIGALRM, sigint);

	//Start the clock!
	sim_clock->sec = 0;
	sim_clock->nsec = 0;

	if((new_proc_clock = malloc(sizeof(Clock))) == NULL)
	{
        	perror("malloc failed");
        	return 0;
	}
	
	//initialize the frame pages
	for(i = 0; i < 256; i++)
	{
		frame[i].pid = 0;
		frame[i].page = 0;
		frame[i].ref = 0;
		frame[i].dirty = 0;
	}

	for(i = 0; i < MAXCHILD; i++)
	{
	        pcb[i].pid = 0;
	        pcb[i].access = 0;
	        pcb[i].pageFault = 0;
	        pcb[i].accessSpeed = 0;
		pcb[i].accessPerSecond = 0;
		pcb[i].faultsPS = 0;
	}


	int j;
	for(i = 0; i < MAXCHILD; i++)
	{
		for(j = 0; j < 32; j++)
			pageTable[i][j] = 0;
	}

	for(i = 0; i < MAXCHILD; i++)
		pidvector[i] = 0;

	newProcTime();

	if(fp == NULL)
	{
		printf("Where is the file???\n");
		fp = fopen("log.out", "w");
	}
	//Now we start the main show
	while(1)
	{
		if(counter < MAXCHILD && checkProcTime())
		{
                        if ((child = fork()) == 0)
                        {
                                execvp(exec[0], exec);
                                perror("exec failed");
                        }
        
                        if (child < 0)
                        {
                                perror("Failed to fork");
                                sigint(0);
                        }
                        counter++;
			tot_proc++;

			printf("Adding new process %ld\n", (long) child);
			for(i = 0; i < MAXCHILD; i++)
			{
				printf("P%d = %li  ", i, (long) pidvector[i]);
				if(pidvector[i] == 0)
				{
					printf("Process index %d is empty!", i);
					pidvector[i] = child;
					break;
				}
			}
			printf("\n");
			insertPid(pcb, child);		
		}
			
		if(msgrcv(msgqid, &message, sizeof(message), 2, IPC_NOWAIT) > 0)
		{
			printf("OSS acknowledges child %ld has died\n", (long) message.pid);
			if(vOpt)
				fprintf(fp, "%s %ld %s", "OSS acknowledges child", (long) message.pid, "has died\n");
			wait(NULL);	
			counter--;

			index = findVectorIndex(message.pid, pidvector);
			pidvector[index] = 0;
			for(i = 0; i < 32; i++)
				pageTable[index][i] = 0;
			printf("Process %li: Total accesses: %f, Access per second: %f, Page faults per access: %f, \n", (long) pcb[index].pid, pcb[index].access, pcb[index].accessPerSecond, pcb[index].faultsPS);
			
		}
		
                if(msgrcv(msgqid, &message, sizeof(message), 3, IPC_NOWAIT) > 0)
                {
			numCalls++;
			double req = message.mpageReq;
			request = req / 1024;
			index = findVectorIndex(message.pid, pidvector);
			printf("Process %d is requesting address %f at time %f:%f\n", index, req, sim_clock->sec, sim_clock->nsec);
			fprintf(fp, "%s %d %s %f%s", "Process", index, "is requesting address", req, "\n");
			if(!fp)
				perror("log file");
			else
				printf("What the hell?\n");
			if(pageTable[index][request])
			{
				printf("Already there! Page Fault!\n");
				if(vOpt)
					fprintf(fp, "%s", "vopt\n");
				//implement LRU policy
				index = findPCBindex(pcb, message.pid);
				pcb[index].pageFault++;
				printf("Index: %d, Accesses: %f\n", index, pcb[index].access);
				pcb[index].faultsPS = pcb[index].pageFault / pcb[index].access;

                                message.mtype = message.pid;
                                message.mpageReq = 1;
                                msgsnd(msgqid, &message, sizeof(message), 0);
			}
			else
			{
				printf("Page request granted!\n");
				if(vOpt)
					fprintf(fp, "%s", "vOpt!\n");
				pageTable[index][request] = message.mpageReq;

				index = findPFindex(frame);
				frame[index].pid = message.pid;
				frame[index].dirty = message.mWrite;
				frame[index].page = message.mpageReq;
				frame[index].ref++;
				
				index = findPCBindex(pcb, message.pid);
				pcb[index].access++;
				if(sim_clock->nsec > 0)
					pcb[index].accessPerSecond = pcb[index].access / (sim_clock->sec + (sim_clock->nsec / 1000000000));
				
				if(numCalls % 100 == 0)
				{
					printf("Current page table:\n");
					printf("\t Occupied Dirty timeStamp\n");
					for(i = 0; i < 256; i++)
					{
						printf("Frame %d: %s     %d     time!\n", i, isOccupied(frame, i), frame[i].dirty);
					}
					printf("\n");
	                                if(vOpt)
	                                {
        	                                fprintf(fp, "%s", "Current page table:\n");
                	                        fprintf(fp, "%s", "\t Occupied Dirty timeStamp\n");
                        	                for(i = 0; i < 256; i++)
                                	        {
                                        	        fprintf(fp, "%s %d%s %s     %d     %s\n", "Frame", i, ":", isOccupied(frame, i), frame[i].dirty, "time!");
                                       		}
                                        	fprintf(fp, "\n");
                                	}
				}
				message.mtype = message.pid;
				message.mpageReq = 0;
				msgsnd(msgqid, &message, sizeof(message), 0);
			}
                }
		
//                if(msgrcv(msgqid, &message, sizeof(message), 4, IPC_NOWAIT) > 0)
//                {
//
//                }	
                if(sim_clock->nsec >= 1000000000)
                {
	                sim_clock->sec++;
	                sim_clock->nsec = 0;
                }
                sim_clock->nsec += 10000;

		if(tot_proc == 100)
			break;
	}
	printf("Main finished!\n");
	fclose(fp);

	shmdt(sim_clock);
	shmctl(shmid, IPC_RMID, NULL);
        if(msgctl(msgqid, IPC_RMID, NULL) == -1)
                fprintf(stderr, "Message queue could not be deleted\n");

	kill(0, SIGQUIT);
}

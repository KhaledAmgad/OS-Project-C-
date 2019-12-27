#include <unistd.h> 
#include <signal.h>
#include <stdlib.h>  
# include <stdio.h> 
# include <string.h>
#include <sys/wait.h> 
#include <semaphore.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ipc.h> 
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h> 
#include <string.h>
#include <sys/msg.h> 
#include <stdlib.h>
#include <time.h>

//sem_t* clkMutex;
sem_t* waitRead;
sem_t* numberOfchildrenSem;
int getStatus=0;
int busy=0;
int clk = 0;
int numberOfchildren;
int clkMutexInit;
//SIGUSR1
void sendStatus(int sig)
{	
	getStatus=1;
}
//SIGUSR2
void incClock(int sig) 
{ 
	clk++;
	//sem_wait(clkMutex); 
    printf("clk = %d \n", clk);
}
//SIGALRM
void callIncClock(int sig) 
{
	printf("\n");
	/*
	int value;

    while(1)
    {
    	
    	sem_getvalue(clkMutex, &value);
    	sem_getvalue(numberOfchildrenSem, &numberOfchildren);
    	if (value==0 || clkMutexInit!=numberOfchildren+1)
    	{
    		break;
    	}
    	
    }
    sem_getvalue(numberOfchildrenSem, &numberOfchildren);
    clkMutexInit=numberOfchildren+1;
	sem_init(clkMutex, 1, clkMutexInit ); */
    killpg( 0, SIGUSR2);
    signal(SIGALRM, callIncClock);
    alarm(1);
} 
typedef struct { 
    long mesg_type; 
    char mesg_text[64]; 
}  mesg;
struct process_data{
	int clk;
	int operation;/*operation type [ADD,DEL]*/
	char data[65]; /*datat to be read from file and sent to the disk*/
};
//read file /////////////////////////////////////////////////////////////////////////////
void readdata(int j, struct process_data  ** arr,int * size)
{
	 
	FILE * fp,*copyFp;
	FILE * dummy;
    char line[256] ;
    size_t len = 0;
	//int j=0;
	char buffer[sizeof(int) * 4 + 1];
    sprintf(buffer, "%d", j);
	const char* extension = ".txt";
	char* name_with_extension;
	name_with_extension = malloc(strlen(buffer)+1+4); /* make space for the new string (should check the return value ...) */
	strcpy(name_with_extension, buffer); /* copy name into the new var */
	strcat(name_with_extension, extension); /* add the extension */
	//struct process_data * arr;
	//printf("%s",name_with_extension);
	//count lines of the file to reserve memory for it int the array*/
	dummy = fopen(name_with_extension, "r");
	int coun_lines=0;
	 char c; 

	while(!feof(dummy))
	{
	  c = fgetc(dummy);
	  if(c == '\n')
	  {
	    coun_lines++;
	  }
	}
    // Close the file 
    fclose(dummy); 
	///////////////////////////////////////////////////////////////////////////////////////////////
	*size=coun_lines;
	(*arr)= malloc( *size *  sizeof(**arr));
	fp = fopen(name_with_extension, "r");
	copyFp=fp;
	if (fp == NULL)
		{
			printf( "Can't open input file in.list!\n");
			exit(1);
		}
		char clk[3];
		char  op[3];
		char  msg[64]; 

		for(int i =0;i< *size;i++)
			{
				fscanf(fp,"%[^ ]", clk);
				fscanf(fp,"%c", &c);
				fscanf(fp,"%[^ ]", op);
				fscanf(fp,"%[^\n]", msg);

				(*arr)[i].clk=atoi(clk);
				if(op[0]=='A')
				{	(*arr)[i].operation=1;
					strcpy((*arr)[i].data,msg);
				}
				else 
				{
					(*arr)[i].operation=2;
					(*arr)[i].data[0]=msg[1]-'0';
				}
				
				//printf("%d \n",arr[i].clk);
				//printf("%d \n",arr[i].operation );
				//printf("%s \n",arr[i].data );  
			}
			
	
}//////////////////////////////////////////////////////////////////////////


int main() 
{
	numberOfchildren=5;
	clkMutexInit=numberOfchildren+1;

	signal(SIGUSR2, incClock);

	//clean
	//sem_unlink ("clkMutex");   
    //sem_close(clkMutex);
	//clkMutex = sem_open ("clkMutex", O_CREAT | O_EXCL, 0644, 0);
	//set mutex
	//sem_init(clkMutex, 1, 0); 




	//clean
	sem_unlink ("waitRead");   
    sem_close(waitRead);
	waitRead = sem_open ("waitRead", O_CREAT | O_EXCL, 0644, 0);
	//set mutex
	sem_init(waitRead, 1, 0); 


	//clean
	sem_unlink ("numberOfchildrenSem");   
    sem_close(numberOfchildrenSem);
	numberOfchildrenSem = sem_open ("numberOfchildrenSem", O_CREAT | O_EXCL, 0644, 0);
	//set mutex
	sem_init(numberOfchildrenSem, 1, numberOfchildren); 



	// ftok to generate unique key 
    key_t key = ftok("diskToKernal",65); 
  
	// msgget creates a message queue 
    // and returns identifier 
    int msgid_diskToKernal = msgget(key, 0666 | IPC_CREAT);

    mesg diskToKernal;
    // ftok to generate unique key 
    key = ftok("KernalToDisk",65); 

    int msgid_KernalToDisk = msgget(key, 0666 | IPC_CREAT);
    mesg KernalToDisk;

	//process to kernel message queue
   	key = ftok("processToKernal",50); 
  	int msgid_processToKernal = msgget(key, 0666 | IPC_CREAT);
  
	mesg processToKernal;

	int numberOfProcesses=numberOfchildren-1;

	int parentID;
	int diskPid;
	diskPid=fork();
	if(diskPid==0)
		{

			printf("Disk pid = %d , ppid = %d\n", getpid(), getppid()); 

			// ftok to generate unique key 
		    key_t key = ftok("diskToKernal",65); 
		  
			// msgget creates a message queue 
		    // and returns identifier 
		    int msgid_diskToKernal = msgget(key, 0666 | IPC_CREAT);

		    mesg diskToKernal;


		    // ftok to generate unique key 
		    key = ftok("KernalToDisk",65); 
		  
			// msgget creates a message queue 
		    // and returns identifier 
		    int msgid_KernalToDisk = msgget(key, 0666 | IPC_CREAT);

		    mesg KernalToDisk;

			
			char slots[10][64];
			for (int i = 0; i < 10; ++i)
			{
				slots[i][0]='\0';
			}
			int numberOfFree=10;
    		signal(SIGUSR2, incClock);
    		signal(SIGUSR1, sendStatus);
			while(1)
			{


				if (getStatus==1&& busy == 0)
				{
					getStatus=0;
					busy=1;
					diskToKernal.mesg_type=3;
					diskToKernal.mesg_text[0]=(char)numberOfFree;
					int j=1;
					for (int i = 0; i < 10; ++i)
					{
						if (slots[i][0]=='\0')
						{
							diskToKernal.mesg_text[j]=(char)i;
							j++;
						}
					}
					msgsnd(msgid_diskToKernal, &diskToKernal, sizeof(diskToKernal), 0);

				}
				msgrcv(msgid_KernalToDisk, &KernalToDisk, sizeof(KernalToDisk), 2, IPC_NOWAIT);
				if (KernalToDisk.mesg_type==2 && KernalToDisk.mesg_text[0]=='A')
				{  	//add
					int oldClock=clk;
					for (int i = 0; i < 10; ++i)
					{
						if (slots[i][0]=='\0')
						{
							strcpy(slots[i], &KernalToDisk.mesg_text[1]);
							numberOfFree--;
							break;
						}
					}
					while(1)
					{
						if (clk-oldClock>=3)
						{
							printf(" added %s \n", &KernalToDisk.mesg_text[1] );
							break;
						}


					}
					busy=0;
					KernalToDisk.mesg_type=-1;
					strcpy(KernalToDisk.mesg_text,"");

				}
				if (KernalToDisk.mesg_type==2 && KernalToDisk.mesg_text[0]=='D')
				{  	//delete
					int oldClock=clk;
					slots[(int)KernalToDisk.mesg_text[1]][0]='\0';
					numberOfFree++;
					while(1)
					{
						if (clk-oldClock>=1)
						{
							printf(" removed %d \n", (int)KernalToDisk.mesg_text[1]);
							break;
						}
					}
					busy=0;
					KernalToDisk.mesg_type=-1;
					strcpy(KernalToDisk.mesg_text,"");
				}
				if (KernalToDisk.mesg_type==2&& KernalToDisk.mesg_text[0]=='N' )
				{  	//nothing
					busy=0;
					KernalToDisk.mesg_type=-1;
					strcpy(KernalToDisk.mesg_text,"");
				}


			

			}
	}
	else
	{

		int pids[numberOfProcesses];
		pids[0] = fork(); 
		for(int i=0;i<numberOfProcesses;i++)
		{
		    if (pids[i] == 0)
				{ /*process code *//////////////////////////////////////////////////////////////////

		    		//process to kernel message queue
				   	key_t key = ftok("processToKernal",50); 
				  	int msgid_processToKernal = msgget(key, 0666 | IPC_CREAT);
				  
					mesg processToKernal;
					struct process_data * arr;
					int size=0 ;
					//sem_wait(waitRead);

					readdata(i,&arr,&size);



					sem_post(waitRead);
					printf("%d i = %d\n",size,i );

					int count=0;
					while(count<size)
					{
						if(arr[count].clk<=clk)
						{
							memset(processToKernal.mesg_text, 0, 65);
							processToKernal.mesg_type=1;
							if(arr[count].operation==2)
							{
								processToKernal.mesg_text[0]='D';
								processToKernal.mesg_text[1]=arr[count].data[0];

							}	
							else
							{

								processToKernal.mesg_text[0]='A';
								strcat(processToKernal.mesg_text,arr[count].data);
							}
							
							msgsnd(msgid_processToKernal, &processToKernal, sizeof(processToKernal), 0);
							count++;
						}					
						
					}
					sem_wait(numberOfchildrenSem);
				    exit(0);

						
						
				}
		
		    else
		    {
				if(i==numberOfProcesses-1)
				{
				    printf("parent pid = %d\n\n", getpid()); 
				}
				else
				{

				    	 pids[i+1] = fork();
				    	 parentID=pids[i+1];
				}
		    }
		}

	}
	if (parentID!=0)
	{
		int value;
		while(1)
	    {
	    	sem_getvalue(waitRead, &value);
	    	if (value == numberOfProcesses)
	    	{
	    		break;
	    	}
	    	
	    }
	    int status;
	    int indexTodeleteFound;
	    signal(SIGUSR2, incClock);
		signal(SIGALRM, callIncClock);
		alarm(1);
		processToKernal.mesg_type=-1;

		while (waitpid(-1, &status, WNOHANG) >= 0)
		{

			msgrcv(msgid_diskToKernal, &diskToKernal, sizeof(diskToKernal), 3, IPC_NOWAIT);
			if (processToKernal.mesg_type==-1)
			{
				msgrcv(msgid_processToKernal, &processToKernal, sizeof(processToKernal), 1, IPC_NOWAIT);
			}

			if (processToKernal.mesg_type==1 && processToKernal.mesg_text[0]=='A')
			{

				kill(diskPid, SIGUSR1);


				if (diskToKernal.mesg_type==3)//stats
				{
					diskToKernal.mesg_type=-1;
					printf("number of free slots = %d\n", diskToKernal.mesg_text[0]);
					printf("free index = ");

					for (int i = 0; i < diskToKernal.mesg_text[0]; ++i)
					{
						printf(" %d ", diskToKernal.mesg_text[i+1]);
					}
					printf("\n");
					if ((int)diskToKernal.mesg_text[0]!=0)
					{

						strcpy(KernalToDisk.mesg_text,processToKernal.mesg_text);
						KernalToDisk.mesg_type = 2 ;
						// msgsnd to send message 
	    				msgsnd(msgid_KernalToDisk, &KernalToDisk, sizeof(KernalToDisk), 0); 
	    				printf("%s (will be added) \n",&processToKernal.mesg_text[1]);

					}
					else
					{
						strcpy(KernalToDisk.mesg_text,"N");
						KernalToDisk.mesg_type = 2 ;//do nothing
						// msgsnd to send message 
	    				msgsnd(msgid_KernalToDisk, &KernalToDisk, sizeof(KernalToDisk), 0); 
						printf("(can not add) %s \n",&processToKernal.mesg_text[1]);
					}

					processToKernal.mesg_type=-1;

				}



			}
			if (processToKernal.mesg_type==1 && processToKernal.mesg_text[0]=='D')
			{

				kill(diskPid, SIGUSR1);

				if (diskToKernal.mesg_type==3)//stats
				{
					diskToKernal.mesg_type=-1;
					printf("number of free slots = %d\n", diskToKernal.mesg_text[0]);
					indexTodeleteFound=0;
					printf("free index = ");

					for (int i = 0; i < diskToKernal.mesg_text[0]; ++i)
					{
						printf(" %d ", diskToKernal.mesg_text[i+1]);
						if (processToKernal.mesg_text[1]==(int)diskToKernal.mesg_text[i+1])
						{
							printf("\n");
							strcpy(KernalToDisk.mesg_text,"N");
							KernalToDisk.mesg_type = 2 ;//do nothing
							// msgsnd to send message 
		    				msgsnd(msgid_KernalToDisk, &KernalToDisk, sizeof(KernalToDisk), 0); 
							printf("can not delete index = %d \n",processToKernal.mesg_text[1]);
							indexTodeleteFound=1;
							break;
						}
					}

					if(indexTodeleteFound==0)
					{
						printf("\n");
						KernalToDisk.mesg_type = 2 ;
						strcpy(KernalToDisk.mesg_text,"");
						KernalToDisk.mesg_text[0]=processToKernal.mesg_text[0];
						KernalToDisk.mesg_text[1]=processToKernal.mesg_text[1];
						printf("index = %d will be deleted \n", processToKernal.mesg_text[1]);
						// msgsnd to send message 
	    				msgsnd(msgid_KernalToDisk, &KernalToDisk, sizeof(KernalToDisk), 0); 
 
					}

					processToKernal.mesg_type=-1;

				}
				

			}
			sem_getvalue(numberOfchildrenSem,&numberOfchildren);
			if (numberOfchildren==1)
			{
				kill(diskPid, SIGUSR1);
				if (diskToKernal.mesg_type==3)//stats
				{
					kill(diskPid,SIGKILL);
				}

			}
			
			

			



    		//printf("parentID = %d clock = %d\n",getpid(),clk);

		}
		printf("parentID = %d exit\n",getpid());

		//clean
		//sem_unlink ("clkMutex");   
        //sem_close(clkMutex); 

        //clean
		sem_unlink ("waitRead");   
        sem_close(waitRead); 
        //clean
		sem_unlink ("numberOfchildrenSem");   
        sem_close(numberOfchildrenSem); 
 
		// to destroy the message queue 
    	msgctl(msgid_KernalToDisk, IPC_RMID, NULL);
    	// to destroy the message queue 
    	msgctl(msgid_diskToKernal, IPC_RMID, NULL);
    	// to destroy the message queue 
    	msgctl(msgid_processToKernal, IPC_RMID, NULL);

	}

	exit(0);

} 







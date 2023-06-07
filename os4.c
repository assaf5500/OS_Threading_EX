
#include <string.h>
#include <sys/wait.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <stdint.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>



/*~~~~~ DEFINES~~~~~*/
#define	INTER_MEM_ACCS_T	1000000
#define	MEM_WR_T 		10000
#define	HD_ACCS_T		1000

#define N_OF_P			4
#define WS			4
#define SIM_TIME 		3

typedef struct msg//msgp
{
	long int	mtype;
	char		pid;
	char		page;
	char		operation;
} msg;

typedef struct phys_table
{
	int dirty_bit;
	clock_t create_time;
	
} phys_table;




int page;
int ack[5];
int sms[N_OF_P], pids[N_OF_P], page_table[2*WS];
extern int errno;
struct timespec sleep_mmu;
struct timespec sleep_write;
struct timespec sleep_p;
struct timespec sleep_hd;
sem_t mutex;
phys_table mem_table[5];
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_t evicter_th;
pthread_t print_th;
pthread_mutex_t mem_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t evicter_mutex = PTHREAD_MUTEX_INITIALIZER;


void* page_evicter(void* n)
{
	//printf("EVICTING\n");
	int i, p_to_evic;
	msg msg_m = {0};
	msg_m.mtype = 1;
	clock_t min = clock();
	for (i = 0; i < 5; i++)
	{
		if (mem_table[i].create_time < min)
		{
			min = mem_table[i].create_time;
			p_to_evic = i;
		}
	}
	if (mem_table[p_to_evic].dirty_bit == 1)
	{								// CALL HD
					
		if (msgsnd(sms[3], &msg_m, sizeof(msg_m) - sizeof(msg_m.mtype), 0) == -1) {perror(strerror(errno));}//sms send

		if (msgrcv(sms[3], &msg_m, sizeof(msg_m) - sizeof(msg_m.mtype), 0, 0) == -1) {perror(strerror(errno));}//sms receive
	}
	mem_table[p_to_evic].dirty_bit = 0;
	mem_table[p_to_evic].create_time = clock();
	page_table[page] = p_to_evic;
	
	for (i = 0; i < 2 * WS; i++) {
		if (i != page && page_table[i] == p_to_evic) {
			page_table[i] = -1;
			break;
		}
	}
	return EXIT_SUCCESS;
}

void HD(int sms) {
	sleep_hd.tv_nsec = HD_ACCS_T;
	sleep_hd.tv_sec = 0;
	//printf("~~~HD\n");
	msg msg_m = {0};
	msg_m.mtype =1;
	
	while (1)
	{
		if (msgrcv(sms, &msg_m, sizeof(msg_m) - sizeof(msg_m.mtype), 0, 0) == -1) {perror(strerror(errno));}
		
		nanosleep(&sleep_hd, NULL);
		
		if (msgsnd(sms, &msg_m, sizeof(msg_m) - sizeof(msg_m.mtype), 0) == -1) {perror(strerror(errno));}
	}
}

void* print_mem(void* n)
{	
	int i;
	printf("	~~~print\n");
	while (1)
	{
		if (pthread_cond_wait(&cond, &mem_mutex)) {perror(strerror(errno));}//mutex
		printf("~~Page table~~\n");
		for (i = 0 ; i < 2 * WS ; i++) {
			printf("%d 	| 	%d\n", i, page_table[i]);
		}
		printf("\n~~Memory~~\n");
		for (i = 0 ; i < 5 ; i++) {
			printf("%d 	| 	", i);
			
			(mem_table[i].dirty_bit == -1) ? printf("- \n") : printf("%d \n", mem_table[i].dirty_bit);
		}
		printf("\n");
		pthread_cond_signal(&cond);//mutex done
	} 
	printf("	~~~end print\n");
}

void n_sleep(long n_sec)
{
	//printf("~~~sleep\n");
	struct timespec tmp;
	tmp.tv_sec = 0;
	tmp.tv_nsec = n_sec;
	nanosleep(&tmp,NULL);
}

void p_oneORtwo(int one_or_two, int * msgs) // WHILE RUNNING 'visit' one time as p1, one time as p2 and done.....
{
	//printf("~~~one or two\n");
	int tmp;
	while (1)
	{
		msg tmp_m;
		n_sleep(INTER_MEM_ACCS_T);//nanosleep(&sleep_time_p, NULL);@@@@@@@@@@@@@@@@@

		tmp_m.mtype = 1;
		tmp_m.operation = (char)rand() % 2;
		tmp_m.pid = one_or_two;
		tmp = rand() % WS;
		if(one_or_two == 2)
		{
			tmp += WS;
		}
		else
		{
			printf("BOOM\n\n\nTRAH\n");
		}
		tmp_m.page = tmp;	//(rand() % WS) + (one_or_two == 2 ? WS : 0);
		//printf("~~111	%d\n",(tmp_m.page > 3) ? 0 : tmp_m.page);
		//message to
		if (msgsnd(msgs[0], &tmp_m, sizeof(tmp_m) - sizeof(tmp_m.mtype), 0) ==-1) {perror(strerror(errno));}
		//message back
		
		if (msgrcv(msgs[one_or_two], &tmp_m, sizeof(tmp_m) - sizeof(tmp_m.mtype), 0, 0) == -1) {perror(strerror(errno));}

	}
}
int is_mem_full()
{
	//printf("~~~is full???\n");
	int cntr=0;
	for (int i = 0; i < 5; i++)
	{
		if (mem_table[i].dirty_bit == -1) { cntr++; }
	}
	return (cntr>1 ? 1 : 0 );		
}


//global variables



int main(void)
{
	
	time_t time_to_finish = time(NULL) + SIM_TIME;
	int pid,pid_hd,pid_p1,pid_p2;
	int flag, i, rORw, status;

	//message queues ; //if mgsget fail, sms[i] == '-1'
	for (i = 0; i < N_OF_P ; i++) {sms[i] = msgget(IPC_PRIVATE, 0600 | IPC_CREAT);}
	//create virtual memory table and page table
	for (i = 0; i < 2 * WS; i++) {page_table[i] = -1;}
	
	for(i=0; i < N_OF_P ; i++) {mem_table[i].dirty_bit = -1; mem_table[i].create_time = 0;}
	
	
	if (pthread_mutex_lock(&mem_mutex)) {perror(strerror(errno));}
	//create "print thread"
	if (pthread_create(&print_th, NULL, print_mem, NULL)) {perror(strerror(errno));}
	
	/* create proccesses */
	if ((pid_hd = fork()) == -1) {perror(strerror(errno));}
	
	else if (!pid_hd) {HD(sms[3]);}//HD
	
	else // MMU
	{ 
		pids[0] = getpid();
		pids[1] = pid_hd;
		pid_p1 = fork();

		if (pid_p1 == -1) 
		{
			perror(strerror(errno));
		}
		else if (!pid_p1)
		{
			pids[2] = getpid();
			p_oneORtwo(1, sms);
		}//P1
		else
		{ //MMU
			//pids[2] = pid_p1;
			n_sleep(INTER_MEM_ACCS_T * 0.5);//nanosleep(&sleep_time_mmu, NULL);@@@@@@@@@@@@@
	
			if ((pid_p2 = fork()) == -1) {perror(strerror(errno));}
			
			if (!pid_p2) {pids[2] = getpid();  p_oneORtwo(2, sms);}//P2
			
			else {pids[3] = pid_p2;}//MMU
		}
	}
	
	/* MMU */			
	if(getpid() == pids[0])
	{
		flag =1;
		msg tmp_m;
		tmp_m.mtype = 1;
		tmp_m.operation = 0;
		tmp_m.pid = 0;
		while (time(NULL) < time_to_finish && flag)//no evicter needed yet
		{
			if ((msgrcv(sms[0], &tmp_m, sizeof(tmp_m) - sizeof(tmp_m.mtype), 0, 0)) == -1) {perror(strerror(errno));}
			
			page = tmp_m.page;/// PROBLEM HERE!!!! PAGE ALWAYS THE SAME@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
			pid  = tmp_m.pid;
			rORw = tmp_m.operation;
			//printf("%d	%d	%d	%d\n",ack[0],ack[1],ack[2],ack[3]);
			if(page_table[page] != -1) //'hit'
			{
				if(rORw)// 'wr' mode
				{
					n_sleep(MEM_WR_T);
					//0printf("~~~WR MODE - empty\n");
					mem_table[page_table[page]].dirty_bit = 1;
					mem_table[page_table[page]].create_time = clock();					
				}
				//else -> 'read only' -> immediate
			}
			else // page miss - call HD
			{
				flag = is_mem_full(mem_table);
				
				if ((msgsnd(sms[3], &tmp_m, sizeof(tmp_m) - sizeof(tmp_m.mtype), 0)) == -1) {perror(strerror(errno));}
				
				if ((msgrcv(sms[3], &tmp_m, sizeof(tmp_m) - sizeof(tmp_m.mtype), 0, 0)) == -1) {perror(strerror(errno));}
				
				for (i = 0; i < 5; i++)
				{
					if (mem_table[i].dirty_bit == -1)
					{
						mem_table[i].dirty_bit = 0;
						mem_table[i].create_time = clock();
						break;
					}
				}
				page_table[page] = i;
				pthread_cond_signal(&cond);
				pthread_cond_wait(&cond, &mem_mutex);	
			}
			
			if ((msgsnd(sms[pid], &tmp_m, sizeof(tmp_m) - sizeof(tmp_m.mtype), 0)) == -1) {perror(strerror(errno));}
			//printf("276~~~one or two\n");	
		}	
		
		while(time(NULL) < time_to_finish)//memory table is full -> evicter is needed	
		{	
			if ((msgrcv(sms[0], &tmp_m, sizeof(tmp_m) - sizeof(tmp_m.mtype), 0, 0)) == -1) {perror(strerror(errno));}
			
			page = tmp_m.page;/// PROBLEM HERE!!!! PAGE ALWAYS THE SAME@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
			pid  = tmp_m.pid;
			rORw = tmp_m.operation;			
			if(page_table[page] != -1) //'hit'
			{
				if(rORw)// 'wr' mode
				{
					n_sleep(MEM_WR_T);
					//printf("~~~WR MODE - full\n");
					mem_table[page_table[page]].dirty_bit = 1;
					mem_table[page_table[page]].create_time = clock();					
				}
				//else -> 'read only' -> immediate
			}
			else  //page miss - call evicter
			{
				if (pthread_create(&evicter_th, NULL, page_evicter, sms) == -1) {perror(strerror(errno));}
				
				if (pthread_join(evicter_th, NULL) == -1) {perror(strerror(errno));}
				
				pthread_cond_signal(&cond);
				pthread_cond_wait(&cond, &mem_mutex);
	
			}
			if ((msgsnd(sms[pid], &tmp_m, sizeof(tmp_m) - sizeof(tmp_m.mtype), 0)) == -1) {perror(strerror(errno));}	
		}
		
		for( i=1 ; i<N_OF_P ; i++ )
		{
			kill(pids[i], SIGKILL);
			while (wait(&status) != pids[i]);
		}
		printf("Successfully finished sim\n");
	}
	return 0;
}





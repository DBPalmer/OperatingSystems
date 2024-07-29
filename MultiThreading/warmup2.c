/**
 * Created by Lucas (Deuce) Palmer
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include "cs402.h"
#include "my402list.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
sigset_t set;

//create threads
pthread_t tokenThread;   
pthread_t packetThread;
pthread_t server1Thread;
pthread_t server2Thread;
pthread_t signalThread;

//start time of emulation
struct timeval start;

//structure to store a packet
typedef struct {
	int label;
    int int_arrival;
	int req_tokens;
	int serv_rate;
	struct timeval arrive;
	struct timeval arrQ1;
	struct timeval arrQ2;
	struct timeval leavQ1;
	struct timeval leavQ2;
	struct timeval startserv;
	struct timeval endserv;
} Packet;

//setting variables to their default values
double lambda = 1.0, mu = 0.35, r = 1.5;
int B = 10, P = 3, num = 20;
char *fileName;
FILE *fp;

int tokenBucket = 0;//number of tokens in bucket
int processed = 0;//number of packets processed
int serviced = 0;//number of packets serviced
int cnt = 0;//used for standard dev calc
int quit = 0;//tells server to quit. Can be signalled by <Ctrl+C>

//values used for final print
double inter_time = 0, service_time = 0, Q1_time = 0, Q2_time = 0, S1_time = 0, S2_time = 0, total_time = 0;
int tokens_dropped = 0, total_tokens = 0, packets_dropped = 0, packets_removed = 0;
double avg = 0, avg2 = 0;

My402List Q1;
My402List Q2;

//function declarations
void ReadInput(int, char *[]);
Packet* GetPacket();
void PrintCurrTime(struct timeval);
void PrintEmParam();
void *TokenProc(void*);
void *PacketProc(void*);
void *Server(void*);
void *Monitor(void*);
void FinalPrint();

int main(int argc, char *argv[])
{
	//initialize Q1 and Q2
	if (!My402ListInit(&Q1) || !My402ListInit(&Q2)) {
        fprintf(stderr, "Failed to initialize lists.\n");
        exit(1);
    } 
	//read commandline input
	ReadInput(argc, argv);
    //initialize mutex
    pthread_mutex_init(&mutex, NULL);
	sigemptyset(&set);
 	sigaddset(&set, SIGINT);
 	sigprocmask(SIG_BLOCK, &set, 0);
	//create threads
    pthread_create(&tokenThread, NULL, TokenProc, NULL);
    pthread_create(&packetThread, NULL, PacketProc, NULL);
    pthread_create(&server1Thread, NULL, Server, (void*)1);
    pthread_create(&server2Thread, NULL, Server, (void*)2);
	pthread_create(&signalThread, NULL, Monitor, NULL);
    //beginning parameters print
	PrintEmParam();
	//run threads in parallel
    pthread_join(tokenThread, NULL);
    pthread_join(packetThread, NULL);
    pthread_join(server1Thread, NULL);
    pthread_join(server2Thread, NULL);
	pthread_join(signalThread, NULL);
    //close file
	if (fp != NULL){
		fclose(fp);
	}
	//final Statistics print
    FinalPrint();
    return 0;
}

void ReadInput(int argc, char* argv[]){
	int i = 1;
	while (i < argc){//reads each argv[] item and checks for errors
		if (strcmp(argv[i], "-lambda") == 0){
			if (argc == i+1){
				fprintf(stderr, "Error. Value for \"-lambda\" is not given\n");
				fprintf(stderr, "usage: ./warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
				exit(1);
			}
			if (argv[i+1][0] == '-'){
					fprintf(stderr, "Error. Value for \"-lambda\" is not given\n");
					fprintf(stderr, "usage: ./warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
					exit(1);
			} else {
				lambda = atof(argv[i+1]);
			}
		} else if (strcmp(argv[i], "-mu") == 0){
			if (argc == i+1){
				fprintf(stderr, "Error. Value for \"-mu\" is not given\n");
				fprintf(stderr, "usage: ./warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
				exit(1);
			}
			if (argv[i+1][0] == '-'){
				fprintf(stderr, "Error. Value for \"-mu\" is not given\n");
				fprintf(stderr, "usage: ./warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
				exit(1);
			} else {
				mu = atof(argv[i+1]);
			}
		} else if (strcmp(argv[i], "-r") == 0){
			if (argc == i+1){
				fprintf(stderr, "Error. Value for \"-r\" is not given\n");
				fprintf(stderr, "usage: ./warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
				exit(1);
			}
			if (argv[i+1][0] == '-'){
				fprintf(stderr, "Error. Value for \"-r\" is not given\n");
				fprintf(stderr, "usage: ./warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
				exit(1);
			} else {
				r = atof(argv[i+1]);
			}
		} else if (strcmp(argv[i], "-B") == 0){
			if (argc == i+1){
				fprintf(stderr, "Error. Value for \"-B\" is not given\n");
				fprintf(stderr, "usage: ./warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
				exit(1);
			}
			if (argv[i+1][0] == '-'){
				fprintf(stderr, "Error. Value for \"-B\" is not given\n");
				fprintf(stderr, "usage: ./warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
				exit(1);
			} else {
				B = atof(argv[i+1]);
			}
		} else if (strcmp(argv[i], "-P") == 0){
			if (argc == i+1){
				fprintf(stderr, "Error. Value for \"-P\" is not given\n");
				fprintf(stderr, "usage: ./warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
				exit(1);
			}
			if (argv[i+1][0] == '-'){
				fprintf(stderr, "Error. Value for \"-P\" is not given\n");
				fprintf(stderr, "usage: ./warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
				exit(1);
			} else {
				P = atof(argv[i+1]);
			}
		} else if (strcmp(argv[i], "-n") == 0){
			if (argc == i+1){
				fprintf(stderr, "Error. Value for \"-n\" is not given\n");
				fprintf(stderr, "usage: ./warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
				exit(1);
			}
			if (argv[i+1][0] == '-'){
				fprintf(stderr, "Error. Value for \"-n\" is not given\n");
				fprintf(stderr, "usage: ./warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
				exit(1);
			} else {
				num = atof(argv[i+1]);
			}
		} else if (strcmp(argv[i], "-t") == 0){
			if (argc == i+1){
				fprintf(stderr, "Error. Value for \"-t\" is not given\n");
				fprintf(stderr, "usage: ./warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
				exit(1);
			}
			if (argv[i+1][0] == '-'){
				fprintf(stderr, "Error. Value for \"-t\" is not given\n");
				fprintf(stderr, "usage: ./warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
				exit(1);
			} else {
				fileName = argv[i+1];
			}
			fp = fopen(argv[i+1], "r");
			if (fp == NULL){
				fprintf(stderr, "Failed to open input file.\n");
				perror(argv[i+1]);
				exit(1);
			}
			char buf[2000];
			fgets(buf, sizeof(buf), fp);
			if (strlen(buf) > 1024){//a line longer than 1024 characters is not exceptable
				fprintf(stderr, "Invalid input file format, first line of input file is too long.\n");
				exit(1);
			}
			if (buf[0] == ' ' || buf[0] == '\t'){//no lead space/tab characters
				fprintf(stderr, "Invalid input file format, first line of input file has leading space/tab character(s).\n");
				exit(1);
			}
			if (buf[strlen(buf)-1] == ' ' || buf[strlen(buf)-1] == '\t'){//no trailing space/tab characters
				fprintf(stderr, "Invalid input file format, first line of input file has trailing space/tab character(s).\n");
				exit(1);
			}
			for (int i=0; i<strlen(buf)-1; i++){
				if (buf[i] < '0' || buf[i] > '9'){//checks to make sure "num" is just an integer
					fprintf(stderr, "malformed input - line 1 is not just a number.\n");
					exit(1);
				}
			}
			num = atoi(buf);
		} else {//user did not enter valid option
			fprintf(stderr, "Incorrect command line input. \"%s\" is not a valid option.\n", argv[i]);
			fprintf(stderr, "usage: ./warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
			exit(1);
		}
		i+=2;//plus 2, since we also read i+1 for the corresponding value
	}
}

Packet* GetPacket(int i){
	if (fp == NULL){//no file
		Packet *packet = (Packet*)malloc(sizeof(Packet));
		if (1/lambda > 10){//can't wait more that 10 seconds
			packet->int_arrival = 10 * 1000;
		} else {
			packet->int_arrival = (int)1/lambda*1000;//seconds to milliseconds
		}
		packet->req_tokens = P;
		if (1/mu > 10){//can't wait more that 10 seconds
			packet->serv_rate = 10 * 1000;
		} else {
			packet->serv_rate = (int)1/mu*1000;//seconds to milliseconds
		}
		packet->label = i;
		return packet;
	} else {
		char buf[2000];//read from file
		fgets(buf, sizeof(buf), fp);
		if (strlen(buf) > 1024){//a line longer than 1024 characters is not exceptable
			fprintf(stderr, "Invalid input file format, line of input file is too long.\n");
			exit(1);
		}
		if (buf[0] == ' ' || buf[0] == '\t'){//no lead space/tab characters
			fprintf(stderr, "Invalid input file format, line of input file has leading space/tab character(s).\n");
			exit(1);
		}
		if (buf[sizeof(buf)-1] == ' ' || buf[sizeof(buf)-1] == '\t'){//no trailing space/tab characters
			fprintf(stderr, "Invalid input file format, line of input file has trailing space/tab character(s).\n");
			exit(1);
		}
		double int_arr, serv_time;
		int fields = 0;
		char* token = strtok(buf, " \t");
		while (token != NULL){//grabs next value after spaces and tabs
			if (fields == 0){
				int_arr = atof(token);
			} else if (fields == 1){
				P = atoi(token);
			} else if (fields == 2){
				serv_time = atof(token);
			}
			token = strtok(NULL, " \t");					
			fields++;
		}
		if (fields != 3){//a line of input must have 3 fields
			fprintf(stderr, "Invalid input file format, line of input file does not have the correct amount of fields.\n");
			exit(1);
			}				
		Packet *packet = (Packet*)malloc(sizeof(Packet));
		packet->label = i;
		packet->int_arrival = int_arr;
		packet->req_tokens = P;
		packet->serv_rate = serv_time;
		return packet;
	}
}

void PrintCurrentTime(struct timeval end){
	struct timeval currTime;
	timersub(&end, &start, &currTime);
	printf("%012.3fms: ", (currTime.tv_sec*1000)+currTime.tv_usec*0.001);
}

void PrintEmParam(){
	printf("Emulation Parameters:\n\tnumber to arrive = %d\n", num);
	if (fp == NULL){//only print if no file specified
		printf("\tlambda = %.6g\n\tmu = %.6g\n", lambda, mu);
	} 
	printf("\tr = %.6g\n\tB = %d\n", r, B);
	if (fp == NULL){//only print if no file specified
		printf("\tP = %d\n", P);
	} else {//only print if file is specified
		printf("\ttsfile = %s\n\n", fileName);
	}
	gettimeofday(&start, NULL);
	PrintCurrentTime(start);//will return 0.000ms since start - start is 0
	printf("emulation begins\n");
}

void *TokenProc(void* arg){
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
	int cnt = 1;
	while (processed < num){
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);//only enable cancellation while alseep
		if (1/r > 10){//can't wait more that 10 seconds
			usleep(10*1000000);
		} else {
			usleep(((int)(1/r*1000))*1000);
		}
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
		pthread_mutex_lock(&mutex);
		total_tokens++;
		struct timeval token;
		gettimeofday(&token, NULL);
		PrintCurrentTime(token);//print token arrival time
		if (tokenBucket >= B){//token bucket is full
			printf("token t%d arrives, dropped\n", cnt);
			tokens_dropped++;
		} else {
			tokenBucket++;
			if (tokenBucket == 0){
				printf("token t%d arrives, token bucket now has %d token\n", cnt, tokenBucket);
			} else {
				printf("token t%d arrives, token bucket now has %d tokens\n", cnt, tokenBucket);
			}
			if (Q1.num_members > 0){//if there are packets in Q1
				My402ListElem elem = *My402ListFirst(&Q1);
				Packet* pack = (Packet*)elem.obj;
				if (tokenBucket >= pack->req_tokens){//do we have enough tokens to service packet?
					My402ListUnlink(&Q1, My402ListFirst(&Q1));//remove packet from Q1
					gettimeofday(&pack->leavQ1, NULL);
					struct timeval inQ1;
					timersub(&pack->leavQ1, &pack->arrQ1, &inQ1);
					struct timeval currTime;
					timersub(&pack->leavQ1, &start, &currTime);
					double q1 = (inQ1.tv_sec*1000)+inQ1.tv_usec*0.001;
					Q1_time += q1;
					tokenBucket -= pack->req_tokens;
					PrintCurrentTime(pack->leavQ1);
					if (tokenBucket == 1){
						printf("p%d leaves Q1, time in Q1 = %.3fms, token bucket now has %d token\n", pack->label, q1, tokenBucket);
					} else {
						printf("p%d leaves Q1, time in Q1 = %.3fms, token bucket now has %d tokens\n", pack->label, q1, tokenBucket);
					}
					My402ListAppend(&Q2, (void*)pack);//add packet to Q2
					gettimeofday(&pack->arrQ2, NULL);
					PrintCurrentTime(pack->arrQ2);
					printf("p%d enters Q2\n", pack->label);
					processed++;
					pthread_cond_broadcast(&cv);//notify servers that packets are ready to be served
				}
			}
		}
		pthread_mutex_unlock(&mutex);
		cnt++;
	}
	return NULL;
}

void *PacketProc(void* arg){
	pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
	struct timeval lastPacket = start;
	for (int i=1; i<=num; i++){
		//take first packet and sleep
		Packet* pack = GetPacket(i);//generate packet
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);//only enable cancellation while alseep
		usleep(pack->int_arrival*1000);
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, 0);
		pthread_mutex_lock(&mutex);
		gettimeofday(&pack->arrive, NULL);
		struct timeval intar;
		timersub(&pack->arrive, &lastPacket, &intar);
		double x = (intar.tv_sec*1000)+intar.tv_usec*0.001;
		inter_time += x;
		PrintCurrentTime(pack->arrive);
		printf("p%d arrives, needs %d tokens, inter-arrival time = %.3fms", pack->label, pack->req_tokens, x);
		lastPacket = pack->arrive;
		if (pack->req_tokens <= B){//otherwise drop it
			printf("\n");
			My402ListAppend(&Q1, (void*)pack);//add packet to Q1
			gettimeofday(&pack->arrQ1, NULL);
			PrintCurrentTime(pack->arrQ1);
			printf("p%d enters Q1\n", pack->label);
			if (Q1.num_members <= 1 && tokenBucket >= pack->req_tokens){//if the packet is first in line and the token bucket has enough tokens
				My402ListUnlink(&Q1, My402ListFirst(&Q1));//remove packet from Q1
				gettimeofday(&pack->leavQ1, NULL);
				struct timeval inQ1;
				timersub(&pack->leavQ1, &pack->arrQ1, &inQ1);
				double q1 = (inQ1.tv_sec*1000)+inQ1.tv_usec*0.001;
				Q1_time += q1;
				tokenBucket -= pack->req_tokens;
				PrintCurrentTime(pack->leavQ1);
				if (tokenBucket == 1){
					printf("p%d leaves Q1, time in Q1 = %.3fms, token bucket now has %d token\n", pack->label, q1, tokenBucket);
				} else {
					printf("p%d leaves Q1, time in Q1 = %.3fms, token bucket now has %d tokens\n", pack->label, q1, tokenBucket);
				}
				My402ListAppend(&Q2, (void*)pack);//add packet to Q2
				gettimeofday(&pack->arrQ2, NULL);
				PrintCurrentTime(pack->arrQ2);
				printf("p%d enters Q2\n", pack->label);
				processed++;
				pthread_cond_broadcast(&cv);//notify servers that packets are ready to be served
			}
		} else {
			packets_dropped++;
			processed++;
			serviced++;
			printf(", dropped\n");
			pthread_cond_broadcast(&cv);//still must notify servers, since the value of serviced has changed
		}
		pthread_mutex_unlock(&mutex);
	}
	return NULL;
}

void *Server(void* arg){
	for (;;){
		pthread_mutex_lock(&mutex);
		while (Q2.num_members < 1 && serviced < num && quit == 0){//Q2 is empty, we havent serviced all packets, or <Ctrl+C> has not been pressed
			pthread_cond_wait(&cv, &mutex);
		}
		if (serviced == num){//if all packets have been serviced
			pthread_cancel(signalThread);//cancel signal thread, otherwise it will run forever
			pthread_cond_broadcast(&cv);//wakeup other server, incase it is still sleeping
			pthread_mutex_unlock(&mutex);
			return NULL;
		} else if (quit == 1){//no need to cancel signal thread, since it terminates when <Ctrl+C> is pressed
			pthread_cond_broadcast(&cv);
			pthread_mutex_unlock(&mutex);
			return NULL;
		}
		My402ListElem* elem = My402ListFirst(&Q2);
		Packet pack = *(Packet*)elem->obj;
		My402ListUnlink(&Q2, My402ListFirst(&Q2));//remove packet from Q2
		gettimeofday(&pack.leavQ2, NULL);
		struct timeval inQ2;
		timersub(&pack.leavQ2, &pack.arrQ2, &inQ2);
		double q2 = (inQ2.tv_sec*1000)+inQ2.tv_usec*0.001;
		Q2_time += q2;
		PrintCurrentTime(pack.leavQ2);
		printf("p%d leaves Q2, time in Q2 = %.3fms\n", pack.label, q2);
		gettimeofday(&pack.startserv, NULL);
		PrintCurrentTime(pack.startserv);
		printf("p%d begins service at S%d, requesting %dms of service\n", pack.label, (int)arg, pack.serv_rate);
		pthread_mutex_unlock(&mutex);
		usleep(pack.serv_rate*1000);//service packet with mutex unlocked
		pthread_mutex_lock(&mutex);
		gettimeofday(&pack.endserv, NULL);
		struct timeval service;
		timersub(&pack.endserv, &pack.startserv, &service);
		struct timeval total;
		timersub(&pack.endserv, &pack.arrive, &total);
		double x = (service.tv_sec*1000)+service.tv_usec*0.001;
		if ((int)arg == 1){
			S1_time += x;
		} else {
			S2_time += x;
		}
		service_time += x;
		double y = (total.tv_sec*1000)+total.tv_usec*0.001;
		total_time += y;
		avg = ((cnt*avg)+y)/(cnt + 1);//sd calcs
		avg2 = ((cnt*avg2)+(y*y))/(cnt + 1);//sd calcs
		cnt++;
		PrintCurrentTime(pack.endserv);
		printf("p%d departs from S%d, service time = %.3fms, time in system = %.3fms\n", pack.label, (int)arg, x, y);
		if (++serviced == num){//if all packets have been served, terminate and wake up other server
			pthread_cond_broadcast(&cv);
			pthread_mutex_unlock(&mutex);
			return NULL;
		} else {
			pthread_mutex_unlock(&mutex);
		}
	}
	return NULL;
}

void *Monitor(void* arg) {
 int sig;
 while (1) {
	sigwait(&set, &sig);
	pthread_mutex_lock(&mutex);
	quit = 1;//value used to notify server threads
	struct timeval time;
	printf("SIGINT caught, no new packets or tokens will be allowed\n");
	//cancel packet & token
	pthread_cancel(packetThread);
	pthread_cancel(tokenThread);
	//remove packest from Q1 & Q2
	My402ListElem *iter = Q1.anchor.next;
    while (iter != &Q1.anchor){
    	Packet* pack = iter->obj;
        My402ListUnlink(&Q1, iter);
		gettimeofday(&time, NULL);
		PrintCurrentTime(time);
		printf("p%d removed from Q1\n", pack->label);
		packets_removed++;
        iter = iter->next;
    }
	iter = Q2.anchor.next;
    while (iter != &Q2.anchor){
		Packet* pack = iter->obj;
        My402ListUnlink(&Q2, iter);
		gettimeofday(&time, NULL);
		PrintCurrentTime(time);
		printf("p%d removed from Q2\n", pack->label);
		//since Q1_time only accounts for completed packets, and we already added time spent of this packet into Q1_time, it must be removed
		struct timeval inQ1;
		timersub(&pack->leavQ1, &pack->arrQ1, &inQ1);
		double q1 = (inQ1.tv_sec*1000)+inQ1.tv_usec*0.001;
		Q1_time -= q1;
		packets_removed++;
        iter = iter->next;
    }
	pthread_cond_broadcast(&cv);//notify servers that it is time to finish service on current packet and terminate
	pthread_mutex_unlock(&mutex);
	pthread_exit(0);
 }
 return(0);
}

void FinalPrint(){
	//calculate total time of emulation
	struct timeval totalTime;
	struct timeval end;
	gettimeofday(&end, NULL);
	timersub(&end, &start, &totalTime);
	double endTime = (totalTime.tv_sec*1000)+totalTime.tv_usec*0.001;
	printf("%012.3fms: emulation ends\n", endTime);

	printf("\nStatistics:\n\n");
	//printout for each value, each checking for "N/A" situations
	printf("\taverage packet inter-arrival time = ");
	if (num == 0){
		printf("N/A (no packets entered system)\n");
	} else {
		printf("%.6gs\n", inter_time/num/1000);
	}
    printf("\taverage packet service time = ");
	if (num-packets_dropped-packets_removed == 0){
		printf("N/A (no packets served)\n\n");
	} else {
		printf("%.6gs\n\n", service_time/(num-packets_dropped-packets_removed)/1000);
	}
    printf("\taverage number of packets in Q1 = ");
	if (endTime == 0){
		printf("N/A (emulation time was zero)\n");
	} else {
		printf("%.6g\n", Q1_time/endTime);
	}
	printf("\taverage number of packets in Q2 = ");
	if (endTime == 0){
		printf("N/A (emulation time was zero)\n");
	} else {
		printf("%.6g\n", Q2_time/endTime);
	}
	printf("\taverage number of packets in S1 = ");
	if (endTime == 0){
		printf("N/A (emulation time was zero)\n");
	} else {
		printf("%.6g\n", S1_time/endTime);
	}
	printf("\taverage number of packets in S2 = ");
	if (endTime == 0){
		printf("N/A (emulation time was zero)\n\n");
	} else {
		printf("%.6g\n\n", S2_time/endTime);
	}
    printf("\taverage time a packet spent in system = ");
	if (num-packets_dropped-packets_removed == 0){
		printf("N/A (no packets were served)\n");
		printf("\tstandard deviation for time spent in system = N/A (no packets were served)\n\n");
	} else {
		printf("%.6gs\n\n", total_time/(num-packets_dropped-packets_removed)/1000);
		printf("\tstandard deviation for time spent in system = %.6gs\n\n", sqrt(avg2-(avg*avg))/1000);
	}
	printf("\ttoken drop probability = ");
	if (total_tokens == 0){
		printf("N/A (no tokens entered the emulation)\n");
	} else {
		printf("%.6g\n", (double)tokens_dropped/total_tokens);
	}
	printf("\tpacket drop probability = ");
	if (total_tokens == 0){
		printf("N/A (no packets entered the emulation)\n");
	} else {
		printf("%.6g\n", (double)packets_dropped/num);
	}
}
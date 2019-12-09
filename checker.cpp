#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/time.h>
#include <unordered_set>
using namespace std;

#define DEFAULT_THREADCOUNT 1
#define MAX_THREADS 1000
#define MAX_THREAD_TESTS 213700000

typedef struct threadArg{
	pthread_t tid;
	unsigned int seed;
	unsigned int *workingThreads;
	pthread_mutex_t *workingThreadsMx;
	long long *testsDone;
	pthread_mutex_t *testsDoneMx;
	char solvFilename[256];
	char brutFilename[256];
	char genFilename[256];
}threadArg_t;

typedef struct cleanupArg{
	threadArg *threadArgs;
	unsigned int tempRand;
}cleanupArg_t;

void clean(void *argsPtr);
void *threadWork(void *argsPtr);
void readArguments(int argc, char **argv, char *solvFilename, char *brutFilename, char *genFilename, unsigned int *threadCount);

void readArguments(int argc, char **argv, char *solvFilename, char *brutFilename, char *genFilename, unsigned int *threadCount) {
    *threadCount = DEFAULT_THREADCOUNT;
	if(argc >= 2){
		strcpy(solvFilename, argv[1]);
	}
	if(argc >= 3){
		strcpy(brutFilename, argv[2]);
	}
	if(argc >= 4){
		strcpy(genFilename, argv[3]);
	}
	if(argc >= 5){
		*threadCount = atoi(argv[4]);
        if (*threadCount <= 0) {
            printf("Invalid value for 'threadCount'");
            exit(EXIT_FAILURE);
		}
	}
}

int main(int argc, char **argv){
	cout << "Usage: ./skrypt $SOLUTION_FILENAME $BRUTE_FILENAME $TEST_GENERATOR_FILENAME $NUMBER_OF_THREADS." << endl;
	cout << "REMEMBER TO COMPILE WITH A FLAG -lpthread." << endl;
	cout << endl;

	unsigned int threadCount, workingThreads = 0;
	long long testsDone = 0;
	char solvFilename[256], genFilename[256], brutFilename[256];
    readArguments(argc, argv, solvFilename, brutFilename, genFilename,  &threadCount);
	
	
	pthread_mutex_t workingThreadsMx = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_t testsDoneMx = PTHREAD_MUTEX_INITIALIZER;
	threadArg_t args[MAX_THREADS];
	
	timeval t1;
	gettimeofday(&t1, NULL);
	srand(t1.tv_usec * t1.tv_sec);
	
	for(unsigned int i = 0; i < threadCount; i++){
		args[i].workingThreads = &workingThreads;
		args[i].workingThreadsMx = &workingThreadsMx;
		args[i].testsDone = &testsDone;
		args[i].testsDoneMx = &testsDoneMx;
		args[i].seed = rand();
		strcpy(args[i].solvFilename, solvFilename);
		strcpy(args[i].genFilename, genFilename);
		strcpy(args[i].brutFilename, brutFilename);
	}
	for(unsigned int i = 0; i < threadCount; i++)
		pthread_create(&(args[i].tid), NULL, threadWork, args+i);
	
	while(true){
		sleep(1);
		pthread_mutex_lock(&testsDoneMx);
		cout << "Tests done " << testsDone << endl;
		pthread_mutex_unlock(&testsDoneMx);
		pthread_mutex_lock(&workingThreadsMx);
		cout << "Working threads " << workingThreads << endl;
		if(workingThreads == 0) break;
		pthread_mutex_unlock(&workingThreadsMx);
	}
	return 0;
}

void *threadWork(void *argsPtr){
	threadArg_t *args = (threadArg_t*)argsPtr;
	
	pthread_mutex_lock(args->workingThreadsMx);
	(*(args->workingThreads))++;
	pthread_mutex_unlock(args->workingThreadsMx);
    
	char buf[256];
	char *solvFilename = args->solvFilename;
	char *genFilename = args->genFilename;
	char *brutFilename = args->brutFilename;
	int tempRand = rand_r(&(args->seed));
	
	cleanupArg_t cleanArgs;
	cleanArgs.threadArgs = args;
	cleanArgs.tempRand = tempRand;

	pthread_cleanup_push(clean, &cleanArgs);

	sprintf(buf, "g++ %s -o %s_%d", genFilename, genFilename, tempRand);
    if(system(buf) != 0) pthread_exit(NULL);	
	sprintf(buf, "g++ %s -o %s_%d", solvFilename, solvFilename, tempRand);
    if(system(buf) != 0) pthread_exit(NULL);	
	sprintf(buf, "g++ %s -o %s_%d", brutFilename, brutFilename, tempRand);
    if(system(buf) != 0) pthread_exit(NULL);	

	for(int i = 1; i <= MAX_THREAD_TESTS; i++){
		int genRand = rand_r(&(args->seed));
		pthread_mutex_lock(args->testsDoneMx);
		(*(args->testsDone))++;
		pthread_mutex_unlock(args->testsDoneMx);
		sprintf(buf, "./%s_%d %d > test_%d.in", genFilename, tempRand, genRand, tempRand);
        if(system(buf) != 0) pthread_exit(NULL);
   		sprintf(buf, "./%s_%d < test_%d.in > test_%s_%d.out", solvFilename, tempRand, tempRand, solvFilename, tempRand);
        if(system(buf) != 0) pthread_exit(NULL);
        sprintf(buf, "./%s_%d < test_%d.in > test_%s_%d.out", brutFilename, tempRand, tempRand, brutFilename, tempRand);
        if(system(buf) != 0) pthread_exit(NULL);
        sprintf(buf, "diff -b test_%s_%d.out test_%s_%d.out", brutFilename, tempRand, solvFilename, tempRand);
        if(system(buf) != 0){
            printf("ERROR test_%d.in\n", tempRand);
			sprintf(buf, "cp test_%d.in invalid_test_%d.in", tempRand, tempRand);
			if(system(buf) != 0) pthread_exit(NULL);
            break;
        }
	}

	pthread_cleanup_pop(1);

	pthread_mutex_lock(args->workingThreadsMx);
	(*(args->workingThreads))--;
	pthread_mutex_unlock(args->workingThreadsMx);	
}

void clean(void *argsPtr){
	cleanupArg_t *args = (cleanupArg_t *)argsPtr;
	
	char buf[256];
	char *solvFilename = args->threadArgs->solvFilename;
	char *genFilename = args->threadArgs->genFilename;
	char *brutFilename = args->threadArgs->brutFilename;
	unsigned int tempRand = args->tempRand;

	sprintf(buf, "rm test_%d.in test_%s_%d.out test_%s_%d.out %s_%d %s_%d %s_%d",
		tempRand, 
		solvFilename, tempRand, 
		brutFilename, tempRand, 
		genFilename, tempRand, 
		solvFilename, tempRand, 
		brutFilename, tempRand);
	system(buf);
}

#include "mpi.h"
#include "vector.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
//#define  ARRAYSIZE	16000000
#define  MASTER		0

#ifndef PRINT_STUB
#define PRINT_STUB(){printf ("Stub at %s, line %d.\n", __FILE__, __LINE__);}
#endif 



#ifndef TIMER_START
#define TIMER_START(timer){ timer = clock(); }
#endif

#ifndef TIMER_STOP
#define TIMER_STOP(timer)\
{ \
	double elapsed = (double) (clock() - timer)/CLOCKS_PER_SEC; \
	printf("Function %s took %f s\n", __func__, elapsed);  \
}
#endif
/*Declarations (Might move to header)*/

//Flow
void mpi_array_init(Vector * data, int arraysize);
void mpi_start_tasks(Vector* data, int arraysize, MPI_Status * status, int * offset, int chunksize, int tag1, int tag2, int taskid, int numtasks, float * mysum, float * sum);

//Do work
float update( Vector* data, int myoffset, int chunk, int myid);

//Master node functions
void master_task(Vector* data, int arraysize, MPI_Status * status, int * offset, int chunksize, int tag1, int tag2, int taskid, int numtasks, float * mysum, float * sum);
void master_initialize_array(Vector* data, int arraysize, float* sum);
void master_delegate_tasks(Vector* data, MPI_Status * status, int * offset, int chunksize, int numtasks, int tag1, int tag2);
void master_collect_results(Vector* data, MPI_Status * status, int * offset, int chunksize, int numtasks, int tag1, int tag2);
void master_show_results(Vector* data, int sum, int* offset, int chunksize, int numtasks);

//Worker node functions
void worker_task(Vector* data, MPI_Status * status, int * offset, int chunksize, int tag1, int tag2, int taskid, int numtasks, float * mysum, float * sum);
void worker_receive(Vector* data, MPI_Status * status, int * offset, int chunksize, int tag1, int tag2 );
void worker_reply(Vector* data, MPI_Status * status, int * offset, int chunksize, int tag1, int tag2 );

//MAIN
int main(int argc, char ** argv)
{
	clock_t t;
	TIMER_START(t)
	//initialize variables
	Vector data;

	int   numtasks, taskid, rc, offset, tag1, tag2, chunksize, arraysize;
	float mysum, sum;
	MPI_Status status;

	/***** Initializations *****/
	
	MPI_Init(&argc, &argv);

	/// Start MPI Code
	MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
	if (numtasks % 4 != 0) {
		printf("Quitting. Number of MPI tasks must be divisible by 4. Actual: %d\n", numtasks);
		MPI_Abort(MPI_COMM_WORLD, rc);
		exit(0);
	}
	MPI_Comm_rank(MPI_COMM_WORLD, &taskid);
	printf("MPI task %d has started...\n", taskid);

	//Init vars
	
	arraysize = atoi(argv[1]);
	chunksize = (arraysize / numtasks);
	printf("Chunk sizes: %d\n", chunksize);
	tag2 = 1;
	tag1 = 2;

	//Init array and zero out
	vector_init(&data);


	mpi_start_tasks(&data, arraysize, &status, &offset, chunksize, tag1, tag2, taskid, numtasks, &mysum, &sum);
	///End MPI Code
	//Free
	MPI_Finalize();
	vector_free(&data);

	TIMER_STOP(t)
}   /* end of main */


void mpi_start_tasks(Vector* data, int arraysize, MPI_Status * status, int * offset, int chunksize, int tag1, int tag2, int taskid, int numtasks, float * mysum, float * sum) {

	clock_t t;
	TIMER_START(t)
	/***** Master task only ******/
	if (taskid == MASTER) {

		master_task(data, arraysize, status, offset, chunksize, tag1, tag2, taskid, numtasks, mysum, sum);

	}  /* end of master section */
	else if (taskid > MASTER) { /***** Non-master tasks only *****/

		worker_task(data, status, offset, chunksize, tag1, tag2, taskid, numtasks, mysum, sum);

	} /* end of non-master */
	TIMER_STOP(t)
}

void master_task(Vector* data, int arraysize, MPI_Status * status, int * offset, int chunksize, int tag1, int tag2, int taskid, int numtasks, float * mysum, float * sum)
{
	clock_t t;
	TIMER_START(t)

	printf("Starting master task ID: %d\n", MASTER);
	/* Initialize the array */

	master_initialize_array(data,arraysize,sum);
	
	/* Send each task its portion of the Vector - master keeps 1st part */
	master_delegate_tasks(data,status,offset,chunksize, numtasks, tag1,tag2);
	
	/* Master does its part of the work */
	*offset = 0;
	*mysum = update(data, *offset, chunksize, taskid);

	/* Wait to receive results from each task */
	master_collect_results(data,status,offset,chunksize,numtasks,tag1,tag2);

	/* Get final sum and print sample results */
	MPI_Reduce(mysum, sum, 1, MPI_FLOAT, MPI_SUM, MASTER, MPI_COMM_WORLD);
	
	master_show_results(data,*sum,offset,chunksize,numtasks);
	
	TIMER_STOP(t)
}
void worker_task(Vector* data, MPI_Status * status, int * offset, int chunksize, int tag1, int tag2, int taskid, int numtasks, float * mysum, float * sum)
{
	clock_t t;
	TIMER_START(t)

	printf("Starting worker task. ID: %d\n",taskid );

	vector_set(data,chunksize * numtasks,0);

	/* Receive my portion of Vector from the master task */
	worker_receive(data, status, offset, chunksize, tag1, tag2 );
	
	/* Do Work */
	*mysum = update(data, *offset, chunksize, taskid);

	/* Send my results back to the master task */
	worker_reply(data, status, offset, chunksize, tag1, tag2 );


	MPI_Reduce(mysum, sum, 1, MPI_FLOAT, MPI_SUM, MASTER, MPI_COMM_WORLD);

	TIMER_STOP(t)
}


float update(Vector* data, int myoffset, int chunk, int myid ) {
	
	clock_t t;
	TIMER_START(t)

	printf("ID %d is updating array\n", myid);

	int source, i, j;
	float mysum;
	/* Perform addition to each of my Vector elements and keep my sum */
	mysum = 0;
	for (i = myoffset; i < myoffset + chunk; i++) {
		float addition = vector_get(data, i) + i * 1.0;
		vector_set(data,i, addition);
		mysum = mysum + vector_get(data,i);
	}
	printf("Task %d mysum = %f\n", myid, mysum);
	return(mysum);

	TIMER_STOP(t)
}

void master_initialize_array(Vector* data, int arraysize, float* sum){
	
	clock_t t;
	TIMER_START(t)

	int i;
	*sum = 0;
	for (i = 0; i<arraysize; i++) {
	
		vector_set(data, i, i * 1.0 );
	
		float element = vector_get(data,i);
	
		*sum = *sum + element;
	
	}
	printf("Initialized array sum = %f\n", *sum);

	TIMER_STOP(t)
}
void master_delegate_tasks(Vector* data, MPI_Status * status, int * offset, int chunksize, int numtasks, int tag1, int tag2)
{
	clock_t t;
	TIMER_START(t)

	*offset = chunksize;
	int dest;
	for (dest = 1; dest<numtasks; dest++) {
		MPI_Send(offset, 1, MPI_INT, dest, tag1, MPI_COMM_WORLD);
		MPI_Send(vector_get_pointer(data,*offset), chunksize, MPI_FLOAT, dest, tag2, MPI_COMM_WORLD);
		printf("Sent %d elements to task %d offset= %d\n", chunksize, dest, *offset);
		*offset = *offset + chunksize;
	}

	TIMER_STOP(t)
}
void master_collect_results(Vector* data, MPI_Status * status, int * offset, int chunksize, int numtasks, int tag1, int tag2)
{
	clock_t t;
	TIMER_START(t)

	int i,source;
	printf("Recieving results... \n");
	for (i = 1; i<numtasks; i++) {
		source = i;
		MPI_Recv(offset, 1, MPI_INT, source, tag1, MPI_COMM_WORLD, status);

		MPI_Recv(vector_get_pointer(data,*offset), chunksize, MPI_FLOAT, source, tag2,
			MPI_COMM_WORLD, status);
	}
	printf(" Resulte received, now reducing... \n");

	TIMER_STOP(t)
}

void master_show_results(Vector* data, int sum, int* offset, int chunksize, int numtasks)
{
	clock_t t;
	TIMER_START(t)

	int i,j;
	printf("Sample results: \n");
	*offset = 0;
	for (i = 0; i<numtasks; i++) {
		for (j = 0; j<5; j++)
			printf("  %f", vector_get(data,*offset + j));
		printf("\n");
		*offset = *offset + chunksize;
	}
	printf("*** Final sum= %f ***\n", sum);

	TIMER_STOP(t)
}

void worker_receive(Vector* data, MPI_Status * status, int * offset, int chunksize, int tag1, int tag2 )
{
	clock_t t;
	TIMER_START(t)

	int source = MASTER;
	MPI_Recv(offset, 1, MPI_INT, source, tag1, MPI_COMM_WORLD, status);
	MPI_Recv(vector_get_pointer(data,*offset), chunksize, MPI_FLOAT, source, tag2, MPI_COMM_WORLD, status);

	TIMER_STOP(t)
}
void worker_reply(Vector* data, MPI_Status * status, int * offset, int chunksize, int tag1, int tag2 )
{
	clock_t t;
	TIMER_START(t)

	int dest = MASTER;
	MPI_Send(offset, 1, MPI_INT, dest, tag1, MPI_COMM_WORLD);
	MPI_Send(vector_get_pointer(data,*offset), chunksize, MPI_FLOAT, MASTER, tag2, MPI_COMM_WORLD);

	TIMER_STOP(t)
}


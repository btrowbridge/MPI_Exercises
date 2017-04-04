#include "mpi.h"

#include <stdio.h>
#include <stdlib.h>

//#define  ARRAYSIZE	16000000
#define  MASTER		0


/*http://stackoverflow.com/questions/3536153/c-dynamically-growing-array*/
typedef struct {
	float * arrayf;
	size_t used;
	size_t size;
} Arrayf;

void initArrayf(Arrayf *a, size_t initialSize) {
	a->arrayf = (float *)malloc(initialSize * sizeof(float));
	a->used = 0;
	a->size = initialSize;
}

void insertArrayf(Arrayf *a, float element) {
	if (a->used == a->size) {
		a->size *= 2;
		a->arrayf = (float *)realloc(a, a->size * sizeof(float));
	}
	a->arrayf[a->used++] = element;
}

void freeArrayf(Arrayf *a) {
	free(a);
	a = NULL;
	a->used = a->size = 0;
}

 ////DELETE THIS: for visual studio debugging
 //typedef MPI_Status;
 //#define MPI_INT 0
 //#define MPI_COMM_WORLD 0
 //#define MPI_FLOAT 0
 //#define MPI_SUM 0

void mpi_array_init(Arrayf * data, int * argc, char *** argv, int * tag1, int * tag2, int * taskid, int * chunksize, int rc, int* numtasks);
void mpi_start_tasks(Arrayf* data, int arraysize, MPI_Status * status, int * offset, int chunksize, int tag1, int tag2, int taskid, int numtasks, float * mysum, float * sum);

float update(int myoffset, int chunk, int myid, Arrayf* data);

void master_task(Arrayf* data, int arraysize, MPI_Status * status, int * offset, int chunksize, int tag1, int tag2, int taskid, int numtasks, float * mysum, float * sum);
void non_master_task(Arrayf* data, MPI_Status * status, int * offset, int chunksize, int tag1, int tag2, int taskid, int numtasks, float * mysum, float * sum);

int main(int argc, char ** argv)
{

	//initialize variables
	Arrayf* data;
	initArrayf(data, 0);

	int   numtasks, taskid, rc, offset, tag1, tag2, chunksize;
	float mysum, sum;
	MPI_Status status;

	/***** Initializations *****/
	int arraysize = (size_t)atoi(argv[1]);

	mpi_array_init(data, &argc, &argv, &tag1, &tag2, &taskid, &chunksize, rc, &numtasks);

	mpi_start_tasks(data, arraysize, &status, &offset, chunksize, tag1, tag2, taskid, numtasks, &mysum, &sum);

	//close
	MPI_Finalize();
	freeArrayf(data);
}   /* end of main */

void mpi_array_init(Arrayf * data, int * argc, char *** argv, int * tag1, int * tag2, int * taskid, int * chunksize, int rc, int* numtasks)
{
	MPI_Init(argc, argv);
	MPI_Comm_size(MPI_COMM_WORLD, numtasks);
	if (*numtasks % 4 != 0) {
		printf("Quitting. Number of MPI tasks must be divisible by 4. Actual: %d\n", *numtasks);
		MPI_Abort(MPI_COMM_WORLD, rc);
		exit(0);
	}
	MPI_Comm_rank(MPI_COMM_WORLD, taskid);
	printf("MPI task %d has started...\n", *taskid);
	*chunksize = (data->size / *numtasks);
	*tag2 = 1;
	*tag1 = 2;
}

void mpi_start_tasks(Arrayf* data, int arraysize, MPI_Status * status, int * offset, int chunksize, int tag1, int tag2, int taskid, int numtasks, float * mysum, float * sum) {
	/***** Master task only ******/
	if (taskid == MASTER) {

		master_task(data, arraysize, status, offset, chunksize, tag1, tag2, taskid, numtasks, mysum, sum);

	}  /* end of master section */
	else if (taskid > MASTER) { /***** Non-master tasks only *****/

		non_master_task(data, status, offset, chunksize, tag1, tag2, taskid, numtasks, mysum, sum);

	} /* end of non-master */
}


float update(int myoffset, int chunk, int myid, Arrayf* data) {
	int source, i, j;
	float mysum;
	/* Perform addition to each of my Arrayf elements and keep my sum */
	mysum = 0;
	for (i = myoffset; i < myoffset + chunk; i++) {
		data->arrayf[i] = (data->arrayf[i] + i * 1.0);
		mysum = mysum + data->arrayf[i];
	}
	printf("Task %d mysum = %e\n", myid, mysum);
	return(mysum);
}

void master_task(Arrayf* data, int arraysize, MPI_Status * status, int * offset, int chunksize, int tag1, int tag2, int taskid, int numtasks, float * mysum, float * sum)
{

	/* Initialize the array */
	sum = 0;
	int source,i,j;
	for (i = 0; i<arraysize; i++) {
		insertArrayf(data, i * 1.0);
		float element = data->arrayf[i];
		*sum = *sum + element;
	}
	printf("Initialized array sum = %e\n", sum);
	;
	/* Send each task its portion of the Arrayf - master keeps 1st part */
	*offset = chunksize;
	int dest;
	for (dest = 1; dest<numtasks; dest++) {
		MPI_Send(offset, 1, MPI_INT, dest, tag1, MPI_COMM_WORLD);
		MPI_Send(&data->arrayf[*offset], chunksize, MPI_FLOAT, dest, tag2, MPI_COMM_WORLD);
		printf("Sent %d elements to task %d offset= %d\n", chunksize, dest, *offset);
		*offset = *offset + chunksize;
	}

	/* Master does its part of the work */
	*offset = 0;
	*mysum = update(*offset, chunksize, taskid, data);

	/* Wait to receive results from each task */


	for (i = 1; i<numtasks; i++) {
		source = i;
		MPI_Recv(offset, 1, MPI_INT, source, tag1, MPI_COMM_WORLD, status);
		MPI_Recv(&data[*offset], chunksize, MPI_FLOAT, source, tag2,
			MPI_COMM_WORLD, status);
	}

	/* Get final sum and print sample results */
	MPI_Reduce(mysum, sum, 1, MPI_FLOAT, MPI_SUM, MASTER, MPI_COMM_WORLD);
	printf("Sample results: \n");
	*offset = 0;
	for (i = 0; i<numtasks; i++) {
		for (j = 0; j<5; j++)
			printf("  %e", data[*offset + j]);
		printf("\n");
		offset = offset + chunksize;
	}
	printf("*** Final sum= %e ***\n", sum);

}

void non_master_task(Arrayf* data, MPI_Status * status, int * offset, int chunksize, int tag1, int tag2, int taskid, int numtasks, float * mysum, float * sum)
{
	/* Receive my portion of Arrayf from the master task */
	int source = MASTER;
	MPI_Recv(offset, 1, MPI_INT, source, tag1, MPI_COMM_WORLD, status);
	MPI_Recv(&data->arrayf[*offset], chunksize, MPI_FLOAT, source, tag2,
		MPI_COMM_WORLD, status);

	*mysum = update(*offset, chunksize, taskid, data);

	/* Send my results back to the master task */
	int dest = MASTER;
	MPI_Send(offset, 1, MPI_INT, dest, tag1, MPI_COMM_WORLD);
	MPI_Send(&data->arrayf[*offset], chunksize, MPI_FLOAT, MASTER, tag2, MPI_COMM_WORLD);

	MPI_Reduce(mysum, sum, 1, MPI_FLOAT, MPI_SUM, MASTER, MPI_COMM_WORLD);
}
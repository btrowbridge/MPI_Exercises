#!/bin/bash

NODES=$1
ARRAYSIZE=$2
MPI_RUN="$(which mpirun) -lsf -vapi"
SCRIPT="$(pwd)/my_mpi_array_lab.o $ARRAYSIZE"

echo "CALLING: bsub -n $NODES -R \"span[ptile=1]\" -o %J_$1.txt \"$MPI_RUN $SCRIPT; wait\""

bsub -n $NODES -R "span[ptile=1]" -o ${NODES}nodes_${ARRAYSIZE}size_%J.txt "$MPI_RUN $SCRIPT; wait; echo 'DONE'"

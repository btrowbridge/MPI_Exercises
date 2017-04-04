#!/bin/bash

NODES=20
MPI_RUN="$(which mpirun) -lsf -vapi"
SCRIPT="$(pwd)/$*"

echo "CALLING: bsub -n $NODES -R \"span[ptile=1]\" -o %J_$1.txt \"$MPI_RUN $SCRIPT; wait\""

bsub -n $NODES -R "span[ptile=1]" -o %J_$1.txt "$MPI_RUN $SCRIPT; wait"

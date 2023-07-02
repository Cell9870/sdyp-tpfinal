#!/bin/bash
for ((i=1;i<=3;i++))
do
    echo "iteration $i: openmp"
    ./automaton_omp.exe > ./out/remote/openmp/scenario2/$i.txt
done

for ((i=1;i<=3;i++))
do
    echo "iteration $i: mixto"
    mpirun -np 4 ./automaton_mpi_omp.exe -machinefile hostfile.txt > ./out/remote/mixto/scenario2/$i.txt
done
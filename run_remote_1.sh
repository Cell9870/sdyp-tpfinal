#!/bin/bash
for ((i=1;i<=2;i++))
do
    echo "iteration $i: mpi"
    mpirun -np 4 ./automaton_mpi.exe -machinefile hostfile.txt > ./out/remote/mpi/scenario1/$i.txt
done
for ((i=1;i<=2;i++))
do
    echo "iteration $i: openmp"
    ./automaton_omp.exe > ./out/remote/openmp/scenario1/$i.txt
done
for ((i=1;i<=2;i++))
do
    echo "iteration $i: mixto"
    mpirun -np 4 ./automaton_mpi_omp.exe -machinefile hostfile.txt > ./out/remote/mixto/scenario1/$i.txt
done
for ((i=1;i<=2;i++))
do
    echo "iteration $i: secuencial"
    ./automaton.exe > ./out/remote/secuencial/scenario1/$i.txt
done

#!/bin/bash

for ((i=1;i<=20;i++))
do
    .\automaton.exe > .\out\remote\secuencial\scenario1\$i.txt
done

for ((i=1;i<=20;i++))
do
    mpirun -np 4 .\automaton_mpi.exe -machinefile hostfile.txt > .\out\remote\mpi\scenario1\$i.txt
done

for ((i=1;i<=20;i++))
do
    .\automaton_omp.exe > .\out\remote\openmp\scenario1\$i.txt
done

for ((i=1;i<=20;i++))
do
    mpirun -np 4 .\automaton_mpi_omp.exe -machinefile hostfile.txt > .\out\remote\mixto\scenario1\$i.txt
done
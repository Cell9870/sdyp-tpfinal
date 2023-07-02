for /l %%x in (1, 1, 20) do (.\automaton.exe > .\out\remote\secuencial\scenario1\%%x.txt) 
for /l %%x in (1, 1, 20) do (mpiexec -np 4 .\automaton_mpi.exe > .\out\remote\mpi\scenario1\%%x.txt)
for /l %%x in (1, 1, 20) do (.\automaton_omp.exe > .\out\remote\openmp\scenario1\%%x.txt)
for /l %%x in (1, 1, 20) do (mpiexec -np 4 .\automaton_mpi_omp.exe > .\out\remote\mixto\scenario1\%%x.txt)
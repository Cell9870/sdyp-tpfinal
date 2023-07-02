for /l %%x in (1, 1, 20) do (.\automaton.exe > .\out\local\secuencial\scenario3\%%x.txt) 
for /l %%x in (1, 1, 20) do (mpiexec -np 4 .\automaton_mpi.exe > .\out\local\mpi\scenario3\%%x.txt)
for /l %%x in (1, 1, 20) do (.\automaton_omp.exe > .\out\local\openmp\scenario3\%%x.txt)
for /l %%x in (1, 1, 20) do (mpiexec -np 4 .\automaton_mpi_omp.exe > .\out\local\mixto\scenario3\%%x.txt)
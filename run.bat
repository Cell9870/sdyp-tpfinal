for /l %%x in (1, 1, 100) do (.\automaton.exe > .\out\secuencial\scenario0\%%x.txt) 
for /l %%x in (1, 1, 100) do (mpiexec -np 4 .\automaton_mpi.exe > .\out\mpi\scenario0\%%x.txt)
for /l %%x in (1, 1, 100) do (.\automaton_omp.exe > .\out\openmp\scenario0\%%x.txt)
for /l %%x in (1, 1, 100) do (mpiexec -np 4 .\automaton_mpi_omp.exe > .\out\mixto\scenario0\%%x.txt)
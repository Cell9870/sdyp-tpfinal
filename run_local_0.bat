for /l %%x in (1, 1, 20) do (.\automaton.exe > .\out\local\secuencial\scenario0\%%x.txt) 
for /l %%x in (1, 1, 20) do (mpiexec -np 4 .\automaton_mpi.exe > .\out\local\mpi\scenario0\%%x.txt)
for /l %%x in (1, 1, 20) do (.\automaton_omp.exe > .\out\local\openmp\scenario0\%%x.txt)
for /l %%x in (1, 1, 20) do (mpiexec -np 4 .\automaton_mpi_omp.exe > .\out\local\mixto\scenario0\%%x.txt)
### frequency_attack.c

- mpicc -o frequency_attack frequency_attack.c -lcrypto
- mpirun -np <number_of_processes> ./frequency_attack <key> <file.txt>
- mpirun -np 4 ./frequency_attack 123456 input.txt

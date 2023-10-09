## Instrucciones de Compilación

### des_bruteforce.c

- gcc -o des_bruteforce des_bruteforce.c -lcrypto
- ./des_bruteforce

### bruteforce.c

- mpicc -o bruteforce bruteforce.c -lcrypto
- mpirun -np 4 ./bruteforce

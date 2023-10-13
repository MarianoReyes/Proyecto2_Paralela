## Instrucciones de Compilación

### des_bruteforce.c

- gcc -o des_bruteforce des_bruteforce.c -lcrypto
- ./des_bruteforce

### bruteforce.c

- mpicc -o bruteforce bruteforce.c -lcrypto
- mpirun -np 4 ./bruteforce

### bruteforce2.c

- mpicc bruteforce2.c -o bruteforce2 -lssl -lcrypto
- mpirun -np 4 ./bruteforce2 <key> <text_input.txt>
- mpirun -np 4 ./bruteforce2 18014398509481984L input.txt

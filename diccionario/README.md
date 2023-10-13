# Compilacion

mpicc code.c -o descifrador -lcrypto -lm

# Ejecución

mpiexec -n 4 ./descifrador 17684ff text.txt
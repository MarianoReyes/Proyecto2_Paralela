// bruteforce2_dd.c
// nota: el key usado es bastante pequeño, cuando sea random speedup variará

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <openssl/des.h>

// Función para descifrar utilizando el algoritmo DES
void decrypt(long key, char *ciph, int len)
{
    DES_key_schedule schedule;
    DES_cblock key_block;
    memcpy(&key_block, &key, sizeof(DES_cblock));
    DES_set_key_unchecked(&key_block, &schedule);
    DES_ecb_encrypt((DES_cblock *)ciph, (DES_cblock *)ciph, &schedule, DES_DECRYPT);
}

// Función para cifrar utilizando el algoritmo DES
void encrypt(long key, char *ciph, int len)
{
    DES_key_schedule schedule;
    DES_cblock key_block;
    memcpy(&key_block, &key, sizeof(DES_cblock));
    DES_set_key_unchecked(&key_block, &schedule);
    DES_ecb_encrypt((DES_cblock *)ciph, (DES_cblock *)ciph, &schedule, DES_ENCRYPT);
}

// Función para probar una clave y buscar una subcadena en el texto descifrado
int tryKey(long key, char *ciph, int len, char *search)
{
    char temp[len + 1];
    memcpy(temp, ciph, len);
    temp[len] = 0;
    decrypt(key, temp, len);
    return strstr((char *)temp, search) != NULL;
}

// Función para dividir dinámicamente las claves entre los nodos MPI
void dynamic_key_division(int id, int N, long upper, long *mylower, long *myupper)
{
    long chunk_size = upper / N;
    *mylower = id * chunk_size;
    *myupper = *mylower + chunk_size - 1;
    if (id == N - 1)
    {
        *myupper = upper;
    }
}

int main(int argc, char *argv[])
{
    int N, id;
    long upper = (1L << 56); // Límite superior de claves DES 2^56
    long mylower, myupper;
    MPI_Status st;
    MPI_Request req;
    FILE *file;
    char cipher[1000];
    double start, end;

    if (argc < 3)
    {
        printf("Uso: %s <clave> <archivo.txt>\n", argv[0]);
        exit(1);
    }

    file = fopen(argv[2], "r");
    if (file == NULL)
    {
        printf("Error al abrir el archivo\n");
        exit(1);
    }
    fgets(cipher, sizeof(cipher), file);
    fclose(file);

    cipher[strcspn(cipher, "\n")] = 0;    // Elimina el salto de línea
    long key = strtol(argv[1], NULL, 10); // Convierte la cadena de clave a long

    // Cifra el texto con la clave
    encrypt(key, cipher, strlen(cipher));

    printf("Texto cifrado: %s\n", cipher);

    int flag;
    int ciphlen = strlen(cipher);

    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Init(&argc, &argv);
    start = MPI_Wtime();
    MPI_Comm_size(comm, &N);
    MPI_Comm_rank(comm, &id);

    long found = 0;

    MPI_Irecv(&found, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &req);

    // Realiza la división dinámica de claves otorgando un rango en el cual buscar a cada nodo
    dynamic_key_division(id, N, upper, &mylower, &myupper);

    // Realiza la búsqueda de la clave
    for (long i = mylower; i < myupper; ++i)
    {
        MPI_Test(&req, &flag, MPI_STATUS_IGNORE);
        if (flag)
            break;

        if (tryKey(i, (char *)cipher, ciphlen, search))
        {
            found = i;

            // Notifica a otros nodos
            for (int node = 0; node < N; node++)
            {
                MPI_Send(&found, 1, MPI_LONG, node, 0, MPI_COMM_WORLD);
            }
            break;
        }
    }

    // El nodo maestro maneja el resultado
    if (id == 0)
    {
        MPI_Wait(&req, &st);
        if (found > 0)
        {
            decrypt(found, (char *)cipher, ciphlen);
            printf("Clave encontrada: %li\n", found);
            printf("Término buscado: %s\n", search);
            printf("Texto descifrado: %s\n", cipher);
        }
        else
        {
            printf("Clave no encontrada.\n");
        }
        end = MPI_Wtime();
        printf("El programa MPI tomó %f segundos en ejecutarse.\n", end - start);
    }

    MPI_Finalize();
}

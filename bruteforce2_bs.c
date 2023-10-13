// bruteforce_binary_search.c
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

int count_words(const char *text)
{
    int count = 0;
    int in_word = 0;

    while (*text)
    {
        if (*text == ' ' || *text == '\t' || *text == '\n')
        {
            in_word = 0;
        }
        else if (!in_word)
        {
            in_word = 1;
            count++;
        }
        text++;
    }

    return count;
}

// Devuelve una palabra aleatoria de la cadena
char *random_word(char *text)
{
    int n_words = count_words(text);
    int random_idx = rand() % n_words;

    int current_word = 0;
    char *start = NULL, *end = NULL;

    while (*text)
    {
        if (*text == ' ' || *text == '\t' || *text == '\n')
        {
            if (current_word == random_idx)
            {
                break;
            }
            current_word++;
        }
        else if (current_word == random_idx && start == NULL)
        {
            start = text;
        }

        if (current_word == random_idx)
        {
            end = text;
        }

        text++;
    }

    int length = end - start + 1;
    char *word = malloc(length + 1);
    strncpy(word, start, length);
    word[length] = '\0';

    return word;
}

// Función para realizar una búsqueda binaria en un rango de claves
long binarySearchForCipherKey(long left, long right, char *ciph, int ciphlen, char *search)
{
    while (left <= right)
    {
        long mid = left + (right - left) / 2;

        if (tryKey(mid, ciph, ciphlen, search))
        {
            return mid;
        }

        if (mid < right)
            left = mid + 1;
        else
            right = mid - 1;
    }

    return -1; // Retorna -1 si la clave no se encuentra en el rango
}

int main(int argc, char *argv[])
{
    int N, id;
    long upper = (1L << 56); // Límite superior de claves DES (2^56)
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

    char *search = random_word(cipher);

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

    long mylower = (id * upper) / N;
    long myupper = ((id + 1) * upper) / N;

    // Realiza la búsqueda de la clave utilizando la búsqueda binaria
    long result = binarySearchForCipherKey(mylower, myupper - 1, (char *)cipher, ciphlen, search);

    if (result != -1)
    {
        found = result;

        // Notifica a otros nodos
        for (int node = 0; node < N; node++)
        {
            MPI_Send(&found, 1, MPI_LONG, node, 0, MPI_COMM_WORLD);
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

// bruteforce.c
// nota: el key usado es bastante pequenio, cuando sea random speedup variara
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <openssl/des.h>

void decrypt(long key, char *ciph, int len)
{
    DES_key_schedule schedule;
    DES_cblock key_block;
    memcpy(&key_block, &key, sizeof(DES_cblock));
    DES_set_key_unchecked(&key_block, &schedule);
    DES_ecb_encrypt((DES_cblock *)ciph, (DES_cblock *)ciph, &schedule, DES_DECRYPT);
}

void encrypt(long key, char *ciph, int len)
{
    DES_key_schedule schedule;
    DES_cblock key_block;
    memcpy(&key_block, &key, sizeof(DES_cblock));
    DES_set_key_unchecked(&key_block, &schedule);
    DES_ecb_encrypt((DES_cblock *)ciph, (DES_cblock *)ciph, &schedule, DES_ENCRYPT);
}

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
// Recibe una cadena de varias palabras separadas por espacios
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

// unsigned char cipher[] = {110, 171, 24, 144, 61, 251, 15, 43, 97, 114, 116, 101, 32, 49};

int main(int argc, char *argv[])
{ // char **argv
    int N, id;
    long upper = (1L << 56); // upper bound DES keys 2^56
    long mylower, myupper;
    MPI_Status st;
    MPI_Request req;
    FILE *file;
    char cipher[1000];
    double start, end;

    if (argc < 3)
    {
        printf("Usage: %s <key> <file.txt>\n", argv[0]);
        exit(1);
    }

    file = fopen(argv[2], "r");
    if (file == NULL)
    {
        printf("Error opening file\n");
        exit(1);
    }
    fgets(cipher, sizeof(cipher), file);
    fclose(file);

    cipher[strcspn(cipher, "\n")] = 0; // Elimina el salto de línea
    // printf("Texto original: %s\n", cipher);

    long key = strtol(argv[1], NULL, 10); // convierte la cadena de la clave a long

    char *word = random_word(cipher);
    char *search = random_word(cipher);
    // char search[] = " Parte ";

    // encriptar el texto con la clave
    encrypt(key, cipher, strlen(cipher));

    int flag;
    int ciphlen = strlen(cipher);

    // Hacer que text sea el nuevo cipher

    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Init(&argc, &argv);
    start = MPI_Wtime();
    MPI_Comm_size(comm, &N);
    MPI_Comm_rank(comm, &id);

    int range_per_node = upper / N;

    mylower = (upper / N) * id;
    myupper = (upper / N) * (id + 1) - 1;
    // printf("Node %d: L: %li - U: %li \n", id, mylower, myupper);

    if (id == 0)
    {
        printf("Key: %li\n", key);
        printf("Search: %s\n", search);
        printf("Encrypted text: ");
        for (int i = 0; i < strlen(cipher); i++)
        {
            printf("%d, ", (unsigned char)cipher[i]);
        }
        printf("\n");
    }
    if (id == N - 1)
    {

        myupper = upper;
    }
    long found = 0;
    MPI_Irecv(&found, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &req);
    printf("Node %d: L: %li - U: %li \n", id, mylower, myupper);
    for (long i = mylower; i < myupper; ++i)
    {

        MPI_Test(&req, &flag, MPI_STATUS_IGNORE);
        if (flag)
            break;

        if (tryKey(i, (char *)cipher, ciphlen, search))
        {
            found = i;
            printf("Node %d: %li - %li - FOUND - %li\n", id, mylower, myupper, found);

            // printf("Node %d: %li - %li - FOUND - %li\n", id, mylower, myupper, found);
            for (int node = 0; node < N; node++)
            {
                // printf("Node %d: AVISANDO NODE %d - FOUND - %li\n", id, node, found);
                MPI_Send(&found, 1, MPI_LONG, node, 0, MPI_COMM_WORLD);
            }
            break;
        }
    }
    if (id == 0)
    {
        MPI_Wait(&req, &st);
        decrypt(found, (char *)cipher, ciphlen);
        printf("%li %s\n", found, cipher);
    }
    end = MPI_Wtime();
    if (id == 0)
    {
        printf("El programa MPI tardó %f segundos en ejecutarse.\n", end - start);
    }
    MPI_Finalize();
}
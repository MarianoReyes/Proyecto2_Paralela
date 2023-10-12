#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <openssl/des.h>

#define TABLE_SIZE 1000000 // This is a hypothetical size, you can adjust it

struct rainbow_entry
{
    long key;
    unsigned char encrypted[8]; // assuming 64-bit DES block size
};

struct rainbow_entry table[TABLE_SIZE];

void build_rainbow_table()
{
    char plaintext[] = "HELLO"; // This is a constant plaintext used to build the table

    for (long i = 0; i < TABLE_SIZE; ++i)
    {
        table[i].key = i;
        encrypt(i, plaintext, strlen(plaintext));
        memcpy(table[i].encrypted, plaintext, 8); // Store the first 8 bytes (64 bits)
    }
}

long lookup_rainbow_table(unsigned char *encrypted_value)
{
    for (long i = 0; i < TABLE_SIZE; ++i)
    {
        if (memcmp(table[i].encrypted, encrypted_value, 8) == 0)
        {
            return table[i].key;
        }
    }
    return -1; // Not found in the table
}

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

int main(int argc, char *argv[])
{
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

    long key = strtol(argv[1], NULL, 10); // convierte la cadena de la clave a long

    // Build the rainbow table
    build_rainbow_table();

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

    if (id == N - 1)
    {
        myupper = upper;
    }

    long found_key = lookup_rainbow_table((unsigned char *)cipher);

    if (found_key != -1)
    {
        printf("Node %d found the key: %li\n", id, found_key);
        decrypt(found_key, cipher, ciphlen);
        printf("Decrypted text: %s\n", cipher);
    }
    else
    {
        printf("Node %d did not find the key in the rainbow table\n", id);
    }

    end = MPI_Wtime();
    if (id == 0)
    {
        printf("El programa MPI tardó %f segundos en ejecutarse.\n", end - start);
    }
    MPI_Finalize();
}

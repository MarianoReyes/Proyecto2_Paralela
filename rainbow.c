#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <openssl/des.h>

#define TABLE_SIZE 1000000 // Este es un tamaño hipotético, puedes ajustarlo

struct rainbow_entry
{
    long key;
    unsigned char encrypted[8]; // Suponemos un tamaño de bloque DES de 64 bits
};

struct rainbow_entry table[TABLE_SIZE];

// Función para construir la tabla de arcoíris
void build_rainbow_table()
{
    char plaintext[] = "HELLO"; // Este es un texto plano constante utilizado para construir la tabla

    for (long i = 0; i < TABLE_SIZE; ++i)
    {
        table[i].key = i;
        encrypt(i, plaintext, strlen(plaintext));
        memcpy(table[i].encrypted, plaintext, 8); // Almacenamos los primeros 8 bytes (64 bits)
    }
}

// Función para buscar en la tabla de arcoíris
long lookup_rainbow_table(unsigned char *encrypted_value)
{
    for (long i = 0; i < TABLE_SIZE; ++i)
    {
        if (memcmp(table[i].encrypted, encrypted_value, 8) == 0)
        {
            return table[i].key;
        }
    }
    return -1; // No encontrado en la tabla
}

// Función para descifrar utilizando la clave DES
void decrypt(long key, char *ciph, int len)
{
    DES_key_schedule schedule;
    DES_cblock key_block;
    memcpy(&key_block, &key, sizeof(DES_cblock));
    DES_set_key_unchecked(&key_block, &schedule);
    DES_ecb_encrypt((DES_cblock *)ciph, (DES_cblock *)ciph, &schedule, DES_DECRYPT);
}

// Función para cifrar utilizando la clave DES
void encrypt(long key, char *ciph, int len)
{
    DES_key_schedule schedule;
    DES_cblock key_block;
    memcpy(&key_block, &key, sizeof(DES_cblock));
    DES_set_key_unchecked(&key_block, &schedule);
    DES_ecb_encrypt((DES_cblock *)ciph, (DES_cblock *)ciph, &schedule, DES_ENCRYPT);
}

// Función para contar palabras en un texto
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

// Función para obtener una palabra aleatoria de un texto
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
    long upper = (1L << 56); // Límite superior de las claves DES (2^56)
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

    cipher[strcspn(cipher, "\n")] = 0; // Eliminar el salto de línea

    long key = strtol(argv[1], NULL, 10); // Convierte la cadena de la clave a long

    // Construir la tabla de arcoíris
    build_rainbow_table();

    int flag;
    int ciphlen = strlen(cipher);

    // empezar MPI
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

    // buscar en la tabla rainbow
    long found_key = lookup_rainbow_table((unsigned char *)cipher);

    if (found_key != -1) // si se encontro
    {
        printf("Nodo %d encontró la clave: %li\n", id, found_key);
        decrypt(found_key, cipher, ciphlen);
        printf("Texto descifrado: %s\n", cipher);
    }
    else // en caso no se encontro
    {
        printf("Nodo %d no encontró la clave en la tabla de arcoíris\n", id);
    }

    // imprimir tiempo de ejecucion
    end = MPI_Wtime();
    if (id == 0)
    {
        printf("El programa MPI tardó %f segundos en ejecutarse.\n", end - start);
    }

    // finalizar MPI
    MPI_Finalize();
}

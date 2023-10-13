#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <openssl/des.h>
#include <limits.h>

// Función para cifrar un mensaje con una clave
void encrypt(long key, char *message, int len)
{
    DES_key_schedule schedule;
    DES_cblock key_block;
    memcpy(&key_block, &key, sizeof(DES_cblock));
    DES_set_key_unchecked(&key_block, &schedule);
    DES_ecb_encrypt((DES_cblock *)message, (DES_cblock *)message, &schedule, DES_ENCRYPT);
}

// Función para descifrar un mensaje con una clave
void decrypt(long key, char *message, int len)
{
    DES_key_schedule schedule;
    DES_cblock key_block;
    memcpy(&key_block, &key, sizeof(DES_cblock));
    DES_set_key_unchecked(&key_block, &schedule);
    DES_ecb_encrypt((DES_cblock *)message, (DES_cblock *)message, &schedule, DES_DECRYPT);
}

// Función para contar la frecuencia de cada letra en un mensaje
void count_letter_frequency(char *message, int len, int *frequency)
{
    for (int i = 0; i < len; i++)
    {
        if (message[i] >= 'A' && message[i] <= 'Z')
        {
            frequency[message[i] - 'A']++;
        }
        else if (message[i] >= 'a' && message[i] <= 'z')
        {
            frequency[message[i] - 'a']++;
        }
    }
}
void reset_frequency(int *frequency, int len)
{
    for (int i = 0; i < len; i++)
    {
        frequency[i] = 0;
    }
}
void print_progress(long current, long total, int id)
{
    int barWidth = 50; // Width of the progress bar in console characters

    printf("\r["); // Carriage return to stay on the same line

    int position = barWidth * current / total; // Calculate the position of the 'pointer' in the bar
    for (int i = 0; i < barWidth; i++)
    {
        if (i < position)
            printf("=");
        else if (i == position)
            printf(">");
        else
            printf(" ");
    }

    printf("] %ld%% - Node %d", 100 * current / total, id);
    fflush(stdout); // Force flush the output buffer to display the message immediately
}

int main(int argc, char *argv[])
{
    int N, id;
    long mylower, myupper;
    MPI_Status st;
    MPI_Request req;
    FILE *file;
    char message[1000];
    char encrypted_message[1000];
    int letter_frequency[26] = {0};
    int global_letter_frequency[26] = {0};
    double start, end;

    if (argc < 3)
    {
        printf("Usage: %s <key> <input.txt>\n", argv[0]);
        exit(1);
    }

    file = fopen(argv[2], "r");
    if (file == NULL)
    {
        printf("Error opening file\n");
        exit(1);
    }
    fgets(message, sizeof(message), file);
    fclose(file);

    message[strcspn(message, "\n")] = 0;

    long key = strtol(argv[1], NULL, 10);

    int ciphlen = strlen(message);

    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Init(&argc, &argv);
    start = MPI_Wtime();
    MPI_Comm_size(comm, &N);
    MPI_Comm_rank(comm, &id);

    // Cifrar el mensaje con la clave
    strcpy(encrypted_message, message);
    encrypt(key, encrypted_message, ciphlen);

    // Distribuir el rango de claves
    long total_keys = 1L << 30;
    long keys_per_node = total_keys / N;
    mylower = id * keys_per_node;
    myupper = (id + 1) * keys_per_node - 1;

    // Calcular la frecuencia de letras en el mensaje cifrado
    count_letter_frequency(encrypted_message, ciphlen, letter_frequency);

    // Recopilar las frecuencias de letras globales
    MPI_Allreduce(letter_frequency, global_letter_frequency, 26, MPI_INT, MPI_SUM, comm);

    // Encontrar la clave más probable
    long best_key = 0;
    int best_score = INT_MAX;
    printf("Node %d: L: %li - U: %li \n", id, mylower, myupper);

    for (long i = mylower; i <= myupper; i++)
    {
        reset_frequency(letter_frequency, 26); // Resetting the frequency array before each encryption attempt

        encrypt(i, encrypted_message, ciphlen);
        count_letter_frequency(encrypted_message, ciphlen, letter_frequency);

        int score = 0;
        for (int j = 0; j < 26; j++)
        {
            score += abs(global_letter_frequency[j] - letter_frequency[j]);
        }

        if (score < best_score || i == mylower)
        {
            best_score = score;
            best_key = i;
        }
        if ((i - mylower) % 100000 == 0)
        {
            print_progress(i - mylower, myupper - mylower, id);
            // printf(" - Node %d - iKey: %li\n", id, i);
        }
    }

    // Encontrar la mejor clave global
    MPI_Allreduce(&best_key, &key, 1, MPI_LONG, MPI_MIN, comm);

    if (id == 0)
    {
        decrypt(key, encrypted_message, ciphlen);
        printf("Clave encontrada: %li\n", key);
        printf("Mensaje descifrado: %s\n", encrypted_message);
    }

    // Imprimir información por nodo
    for (int i = 0; i < N; i++)
    {
        MPI_Barrier(comm);
        if (id == i)
        {
            printf("Node %d: L: %li - U: %li\n", id, mylower, myupper);
            if (id == 0)
            {
                printf("Key: %li\n", key);
                printf("Search: podemos\n");
                printf("Encrypted text: ");
                for (int j = 0; j < ciphlen; j++)
                {
                    printf("%d, ", (unsigned char)encrypted_message[j]);
                }
                printf("\n");
            }
        }
    }

    end = MPI_Wtime();
    if (id == 0)
    {
        printf("El programa MPI tardó %f segundos en ejecutarse.\n", end - start);
    }

    MPI_Finalize();
}

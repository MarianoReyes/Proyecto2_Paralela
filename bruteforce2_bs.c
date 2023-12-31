// bruteforce.c
// Nota: la clave utilizada es bastante pequeña; cuando sea aleatoria, la velocidad variará
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>
#include <openssl/des.h>

// Función para desencriptar datos utilizando DES
void decrypt(long key, char *ciph, int len)
{
    DES_key_schedule schedule;
    DES_cblock key_block;
    memcpy(&key_block, &key, sizeof(DES_cblock));
    DES_set_key_unchecked(&key_block, &schedule);
    DES_ecb_encrypt((DES_cblock *)ciph, (DES_cblock *)ciph, &schedule, DES_DECRYPT);
}

// Función para encriptar datos utilizando DES
void encrypt(long key, char *ciph, int len)
{
    DES_key_schedule schedule;
    DES_cblock key_block;
    memcpy(&key_block, &key, sizeof(DES_cblock));
    DES_set_key_unchecked(&key_block, &schedule);
    DES_ecb_encrypt((DES_cblock *)ciph, (DES_cblock *)ciph, &schedule, DES_ENCRYPT);
}

// Función para probar una clave en el cifrado
int tryKey(long key, char *ciph, int len, char *search)
{
    char temp[len + 1];
    memcpy(temp, ciph, len);
    temp[len] = 0;
    decrypt(key, temp, len);
    return strstr((char *)temp, search) != NULL;
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

// unsigned char cipher[] = {110, 171, 24, 144, 61, 251, 15, 43, 97, 114, 116, 101, 32, 49};

int main(int argc, char *argv[])
{
    int N, id;
    long upper = (1L << 56); // Límite superior de claves DES: 2^56
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

    cipher[strcspn(cipher, "\n")] = 0; // Elimina el salto de línea
    // printf("Texto original: %s\n", cipher);

    // Convertir la clave en un número largo y comprobar errores y comprobar que sea un numero
    char *endptr;
    long key = strtol(argv[1], &endptr, 10);
    if (*endptr != '\0' || endptr == argv[1]) // Si la conversión no consumió toda la cadena o no consumió nada
    {
        fprintf(stderr, "Error: La clave proporcionada no es un número válido.\n");
        exit(1);
    }

    char *word = random_word(cipher);
    char *search = random_word(cipher);

    // Encriptar el texto con la clave
    encrypt(key, cipher, strlen(cipher));

    int flag;
    int ciphlen = strlen(cipher);

    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Init(&argc, &argv);
    start = MPI_Wtime();
    MPI_Comm_size(comm, &N);
    MPI_Comm_rank(comm, &id);

    long found = -1; // Inicializar la clave encontrada en -1, indicando que no se encontró.

    // Determinar el rango de claves para cada proceso
    int range_per_node = upper / N;
    mylower = (upper / N) * id;
    myupper = (upper / N) * (id + 1) - 1;

    if (id == N - 1)
    {
        myupper = upper;
    }

    if (id == 0)
    {
        printf("Clave: %li\n", key);
        printf("Búsqueda: %s\n", search);
        printf("Texto encriptado: ");
        for (int i = 0; i < strlen(cipher); i++)
        {
            printf("%d, ", (unsigned char)cipher[i]);
        }
        printf("\n");
    }

    // Realizar la búsqueda binaria

    // Inicializa un bucle que realiza una búsqueda binaria en el rango de claves asignado a este nodo.
    while (mylower <= myupper && found == -1)
    {
        // Calcula el punto medio del rango actual.
        long mid = (mylower + myupper) / 2;

        // Intenta la clave del punto medio en el cifrado para verificar si es la clave correcta.
        if (tryKey(mid, (char *)cipher, ciphlen, search))
        {
            // Si la clave en el punto medio desencripta con éxito el texto, establece 'found' en el valor de 'mid'.
            found = mid;
        }
        else if (key < mid)
        {
            // Si la clave a buscar es menor que el valor en el punto medio,
            // ajusta el límite superior del rango para buscar en la mitad inferior del rango actual.
            myupper = mid - 1;
        }
        else
        {
            // Si la clave a buscar es mayor que el valor en el punto medio,
            // ajusta el límite inferior del rango para buscar en la mitad superior del rango actual.
            mylower = mid + 1;
        }
    }

    // Difundir la clave encontrada a todos los procesos
    MPI_Bcast(&found, 1, MPI_LONG, 0, comm);

    if (found != -1)
    {
        printf("Nodo %d: Clave encontrada: %li\n", id, found);
        decrypt(found, (char *)cipher, ciphlen);
        if (id == 0)
        {
            printf("Mensaje desencriptado: %s\n", cipher);
        }
    }
    else
    {
        printf("Nodo %d: Clave no encontrada.\n", id);
    }

    end = MPI_Wtime();
    if (id == 0)
    {
        printf("MPI se ejecutó en %f segundos.\n", end - start);
    }

    MPI_Finalize();
}

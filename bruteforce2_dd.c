// bruteforce.c
// nota: el key usado es bastante pequenio, cuando sea random speedup variara
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

int main(int argc, char *argv[])
{
    // atributos iniciales
    int N, id;
    long upper = (1L << 56);
    long mylower, myupper;
    MPI_Status st;
    MPI_Request req;
    FILE *file;
    char cipher[1000];
    double start, end;

    if (argc < 3)
    {
        // Verifica si se proporcionaron suficientes argumentos en la línea de comandos.
        // Deben proporcionarse al menos 3 argumentos: el nombre del programa, la clave y el archivo de texto.
        printf("Usage: %s <key> <file.txt>\n", argv[0]);
        exit(1); // Sale del programa si no se proporcionaron suficientes argumentos.
    }

    file = fopen(argv[2], "r");
    if (file == NULL)
    {
        // Abre el archivo especificado en modo lectura ("r").
        // Si no puede abrir el archivo, muestra un mensaje de error y sale del programa.
        printf("Error opening file\n");
        exit(1);
    }
    fgets(cipher, sizeof(cipher), file); // Lee el contenido del archivo y lo almacena en la variable 'cipher'.
    fclose(file);                        // Cierra el archivo después de leerlo.

    cipher[strcspn(cipher, "\n")] = 0; // Elimina cualquier carácter de salto de línea al final del texto.

    long key = strtol(argv[1], NULL, 10); // Convierte la clave (argv[1]) en un número largo.

    char *word = random_word(cipher);   // Obtiene una palabra aleatoria del texto.
    char *search = random_word(cipher); // Obtiene otra palabra aleatoria del texto.

    encrypt(key, cipher, strlen(cipher)); // Encripta el texto con la clave proporcionada.

    int flag;
    int ciphlen = strlen(cipher); // Obtiene la longitud del texto encriptado.

    MPI_Comm comm = MPI_COMM_WORLD;
    MPI_Init(&argc, &argv); // Inicializa MPI y obtiene el número de nodos y el ID de cada nodo.
    start = MPI_Wtime();    // Registra el tiempo de inicio de la ejecución.

    MPI_Comm_size(comm, &N);  // Obtiene el número total de nodos.
    MPI_Comm_rank(comm, &id); // Obtiene el ID (rango) del nodo actual.

    int range_per_node = upper / N; // Calcula el rango de claves a procesar por cada nodo.

    mylower = (upper / N) * id;           // Establece el límite inferior del rango de claves para el nodo actual.
    myupper = (upper / N) * (id + 1) - 1; // Establece el límite superior del rango de claves para el nodo actual.

    if (id == N - 1)
    {
        // Si es el último nodo, ajusta el límite superior al valor máximo de claves (upper).
        myupper = upper;
    }

    if (id == 0)
    {
        printf("Encrypted text: ");
        for (int i = 0; i < strlen(cipher); i++)
        {
            printf("%d, ", (unsigned char)cipher[i]);
        }
        printf("\n");
    }

    long found = 0; // Variable para almacenar la clave encontrada.
    MPI_Irecv(&found, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &req);
    // Inicia una recepción asincrónica para recibir una clave encontrada desde cualquier otro nodo.
    printf("Node %d: L: %li - U: %li \n", id, mylower, myupper); // Muestra información sobre el rango de claves procesadas por el nodo.

    for (long i = mylower; i < myupper; ++i)
    {
        MPI_Test(&req, &flag, MPI_STATUS_IGNORE);
        if (flag)
            break; // Sale del bucle si se encuentra una clave o se recibe un mensaje de otro nodo.

        if (tryKey(i, (char *)cipher, ciphlen, search))
        {
            found = i;                                                                 // Si se encuentra una clave que desencripta el texto con éxito, almacena la clave.
            printf("Node %d: %li - %li - FOUND - %li\n", id, mylower, myupper, found); // Muestra un mensaje de clave encontrada.

            for (int node = 0; node < N; node++)
            {
                MPI_Send(&found, 1, MPI_LONG, node, 0, MPI_COMM_WORLD);
                // Envía la clave encontrada a todos los demás nodos.
            }
            break; // Sale del bucle después de encontrar una clave.
        }
    }

    // Implementación de división dinámica
    if (found == 0)
    {
        long task = mylower;
        while (found == 0)
        {
            MPI_Test(&req, &flag, MPI_STATUS_IGNORE);
            if (flag)
                break;

            if (tryKey(task, (char *)cipher, ciphlen, search))
            {
                found = task;
                printf("Node %d: %li - %li - FOUND - %li\n", id, mylower, myupper, found);

                for (int node = 0; node < N; node++)
                {
                    MPI_Send(&found, 1, MPI_LONG, node, 0, MPI_COMM_WORLD);
                }
            }

            task++;
            if (task >= myupper)
                task = mylower;
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

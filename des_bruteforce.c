#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/des.h>
#include <time.h>

#define TEXT_SIZE 8      // Tamaño del texto cifrado (en bytes)
#define MAX_KEY_LENGTH 8 // Longitud máxima de la llave (en bytes)

void decrypt_des(const unsigned char *ciphertext, const unsigned char *key, unsigned char *plaintext)
{
    DES_key_schedule key_schedule;
    DES_set_key_unchecked((const_DES_cblock *)key, &key_schedule);
    DES_ecb_encrypt((const_DES_cblock *)ciphertext, (DES_cblock *)plaintext, &key_schedule, DES_DECRYPT);
}

int main()
{
    unsigned char ciphertext[TEXT_SIZE] = {0x85, 0xE7, 0x82, 0x6A, 0xE3, 0x58, 0x8D, 0x75}; // Texto cifrado
    unsigned char plaintext[TEXT_SIZE];                                                     // Texto descifrado
    unsigned char key[MAX_KEY_LENGTH] = {0};                                                // Llave para descifrar, inicializada a 0

    clock_t start, end;
    double cpu_time_used;

    printf("Texto cifrado: ");
    for (int i = 0; i < TEXT_SIZE; i++)
    {
        printf("%02X ", ciphertext[i]);
    }
    printf("\n");

    for (int key_length = 1; key_length <= MAX_KEY_LENGTH; key_length++)
    {
        start = clock();

        // Realiza la búsqueda de llave por fuerza bruta
        for (unsigned int i = 0; i < (1 << (key_length * 8)); i++)
        {
            memcpy(key, &i, key_length);
            decrypt_des(ciphertext, key, plaintext);

            // Verifica si el texto descifrado contiene una palabra/frase clave de búsqueda
            if (strstr((const char *)plaintext, "clave_busqueda") != NULL)
            {
                printf("Llave encontrada: ");
                for (int j = 0; j < key_length; j++)
                {
                    printf("%02X ", key[j]);
                }
                printf("\n");

                end = clock();
                cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
                printf("Tiempo de búsqueda: %f segundos\n", cpu_time_used);

                return 0; // Termina el programa si se encuentra la llave
            }
        }

        end = clock();
        cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC;
        printf("Longitud de llave %d: Tiempo de búsqueda: %f segundos\n", key_length, cpu_time_used);
    }

    printf("Llave no encontrada.\n");

    return 0;
}

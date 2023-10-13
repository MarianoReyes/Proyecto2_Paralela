#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <openssl/des.h>
#include <math.h>

#define MAX_PASSWORDS 100000
#define PASSWORD_LENGTH 100

char passwords[MAX_PASSWORDS][PASSWORD_LENGTH];
int password_count = 0;

void load_passwords() {
    FILE *file = fopen("psswrds.txt", "r");
    if (!file) {
        printf("Error al abrir el archivo passwords.txt\n");
        exit(1);
    }

    while (fgets(passwords[password_count], PASSWORD_LENGTH, file)) {
        passwords[password_count][strcspn(passwords[password_count], "\n")] = 0;
        password_count++;
    }
    fclose(file);
}

int contains_common_words(char *text) {
    int valid_word_count = 0;
    for (int i = 0; i < password_count; i++) {
        if (strstr(text, passwords[i])) {
            valid_word_count++;
        }
    }
    return valid_word_count >= 2;
}

void convertKeyToBlock(long key, DES_cblock *key_block) {
    char keyStr[9];
    sprintf(keyStr, "%08ld", key);
    memcpy(key_block, keyStr, 8);
}

void encryptBlockKey(long key, char *block, int len, char *output) {
    DES_key_schedule schedule;
    DES_cblock key_block;
    convertKeyToBlock(key, &key_block);
    DES_set_key_unchecked(&key_block, &schedule);
    DES_ecb_encrypt((DES_cblock *)block, (DES_cblock *)output, &schedule, DES_ENCRYPT);
}

int tryDecryptBlockKey(long key, char *block, int len, char *output, char *originalText) {
    DES_key_schedule schedule;
    DES_cblock key_block;
    convertKeyToBlock(key, &key_block);
    DES_set_key_unchecked(&key_block, &schedule);
    DES_ecb_encrypt((DES_cblock *)block, (DES_cblock *)output, &schedule, DES_DECRYPT);
    return strcmp(output, originalText) == 0;
}

int isValidKey(long key) {
    char keyStr[9];
    sprintf(keyStr, "%08lx", key);
    for (int i = 0; i < 8; i++) {
        // Comprobamos si es un número, una letra minúscula o una letra mayúscula
        if (!((keyStr[i] >= '0' && keyStr[i] <= '9') || 
              (keyStr[i] >= 'a' && keyStr[i] <= 'z') ||
              (keyStr[i] >= 'A' && keyStr[i] <= 'Z'))) {
            return 0;
        }
    }
    return 1;
}

int main(int argc, char *argv[]) {
    int N, id;
    MPI_Status st;
    FILE *file;
    char originalText[1000], cipher[1000];
    double startTime, endTime;

    if (argc < 3) {
        printf("Uso: %s <key> <file.txt>\n", argv[0]);
        exit(1);
    }

    load_passwords();

    long lower_limit = pow(10, 8 - 1);
    long upper_limit = pow(10, 8) - 1;

    file = fopen(argv[2], "r");
    if (!file) {
        printf("Error al abrir el archivo\n");
        exit(1);
    }
    fgets(originalText, sizeof(originalText), file);
    fclose(file);

    originalText[strcspn(originalText, "\n")] = 0;

    int blockSize = 8;
    char block[blockSize];
    memcpy(block, originalText, blockSize);

    // Ciframos usando la clave proporcionada
    long encryption_key = strtol(argv[1], NULL, 10);
    encryptBlockKey(encryption_key, block, blockSize, cipher);

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &N);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);

    startTime = MPI_Wtime();  // Iniciar el tiempo

    int range_per_node = (upper_limit - lower_limit + 1) / N;
    long mylower = lower_limit + range_per_node * id;
    long myupper = lower_limit + range_per_node * (id + 1) - 1;

    if (id == N - 1) {
        myupper = upper_limit;
    }

    for (long i = mylower; i <= myupper; ++i) {
        char decrypted[blockSize + 1];
        if (isValidKey(i) && tryDecryptBlockKey(i, cipher, blockSize, decrypted, originalText)) {
            decrypted[blockSize] = '\0';
            printf("Nodo %d: Clave encontrada: %lx\n", id, i);
            printf("Texto descifrado: %s\n\n", decrypted);
            break;
        }
    }

    endTime = MPI_Wtime();  // Finalizar el tiempo

    if (id == 0) {
        printf("Tiempo de ejecución: %f segundos\n", endTime - startTime);
    }

    MPI_Finalize();
    return 0;
}

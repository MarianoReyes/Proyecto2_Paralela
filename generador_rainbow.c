#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/md5.h>

// Estructura para almacenar pares de hash y contraseñas
typedef struct RainbowTableEntry
{
    char hash[33];    // Hash MD5 (32 caracteres más el carácter nulo).
    char password[7]; // Suponemos contraseñas de 6 caracteres más el carácter nulo.
} RainbowTableEntry;

// Función para calcular el hash MD5 de una cadena
void calculateMD5Hash(const char *str, char *hash)
{
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, str, strlen(str));
    unsigned char md5[MD5_DIGEST_LENGTH];
    MD5_Final(md5, &ctx);

    for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
    {
        snprintf(&hash[i * 2], 3, "%02x", md5[i]);
    }
    hash[32] = '\0';
}

int main()
{
    RainbowTableEntry entry;
    strcpy(entry.password, "123456");
    calculateMD5Hash(entry.password, entry.hash);

    FILE *file = fopen("rainbowtable.csv", "w");
    if (file == NULL)
    {
        printf("Error al abrir el archivo de Rainbow Table.\n");
        return 1;
    }

    fprintf(file, "%s,%s\n", entry.hash, entry.password);
    fclose(file);

    printf("Entrada de Rainbow Table generada y guardada en rainbowtable.csv.\n");

    return 0;
}

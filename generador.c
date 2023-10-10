#include <stdio.h>
#include <string.h>
#include <openssl/des.h>

void encrypt(long key, char *ciph, int len)
{
    DES_cblock keyBlock;
    DES_key_schedule schedule;

    // Copia la llave en keyBlock
    memcpy(&keyBlock, &key, 8);

    // Establecer la llave
    DES_set_key_unchecked(&keyBlock, &schedule);

    // Cifrar usando DES en modo ECB
    DES_ecb_encrypt((DES_cblock *)ciph, (DES_cblock *)ciph, &schedule, DES_ENCRYPT);
}

int main()
{
    char text[100]; // suponiendo que el texto no excede los 100 caracteres
    long key;

    // Solicitar al usuario el texto a cifrar
    printf("Introduce el texto a cifrar: ");
    fgets(text, sizeof(text), stdin);
    text[strcspn(text, "\n")] = 0; // Elimina el salto de línea

    // Solicitar al usuario la clave
    printf("Introduce una llave numérica (hasta 8 bytes/64 bits): ");
    scanf("%ld", &key);

    // Cifrar el texto
    encrypt(key, text, strlen(text));

    // Imprimir texto cifrado como decimal
    printf("Texto cifrado: ");
    for (int i = 0; i < strlen(text); i++)
    {
        printf("%d, ", (unsigned char)text[i]);
    }
    printf("\n");

    return 0;
}

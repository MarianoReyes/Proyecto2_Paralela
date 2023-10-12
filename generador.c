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

int main()
{
    char text[100]; // suponiendo que el texto no excede los 100 caracteres
    long key;
    FILE *file;

    file = fopen("input.txt", "r");
    if (file == NULL)
    {
        printf("Error al abrir el archivo\n");
        return 1;
    }

    fgets(text, sizeof(text), file);
    fclose(file);

    text[strcspn(text, "\n")] = 0; // Elimina el salto de línea

    printf("Texto original: %s\n", text);

    // Solicitar al usuario el texto a cifrar
    // printf("Introduce el texto a cifrar: ");
    // fgets(text, sizeof(text), stdin);
    // text[strcspn(text, "\n")] = 0; // Elimina el salto de línea

    // Solicitar al usuario la clave
    printf("Introduce una llave numérica (hasta 8 bytes/64 bits): ");
    scanf("%ld", &key);

    // ramdom word

    char *word = random_word(text);
    printf("Palabra aleatoria: %s\n", word);
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

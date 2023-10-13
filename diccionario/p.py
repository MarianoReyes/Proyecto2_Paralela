# Abre el archivo de entrada en modo lectura y el archivo de salida en modo escritura
with open('passwords.txt', 'r') as infile, open('output.txt', 'w') as outfile:
    # Itera sobre cada línea en el archivo de entrada
    for line in infile:
        # Remueve espacios en blanco y caracteres de nueva línea
        word = line.strip()
        # Si la longitud de la palabra es 7 o menos, escríbela en el archivo de salida
        if len(word) <= 7:
            outfile.write(word + '\n')

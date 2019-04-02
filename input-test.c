#include<stdio.h>
#include<stdlib.h>

int main()
{
    printf("WHERE TO? > ");

    char *buffer;
    size_t bufferSize = 10;
    size_t characters;

    buffer = (char *)malloc(bufferSize * sizeof(char));

    if (buffer == NULL)
    {
        perror("unable to allocate dynamic buffer");
        exit(1);
    }

    characters = getline(&buffer, &bufferSize, stdin);
    printf("num entered: %zu\n", characters);
    printf("you entered: %s\n", buffer);

}

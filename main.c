#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
 
/*function to get size of the file.*/
long int findSize(const char *file_name)
{
    struct stat st; /*declare stat variable*/
     
    /*get the size using stat()*/
     
    if (stat(file_name,&st) == 0)
        return st.st_size;
    else
        return -1;
}

char *book;
int size;

int shuffle_cmp(const void * a, const void * b) {
    int ind1 = *((int*)a);
    int ind2 = *((int*)b);
    
    return strcmp(&book[ind1], &book[ind2]);
}

// 1. Узнать размер файла
// 2. Зарезервировать память под весь файл
// 3. Посчитать количество строчек = n
// 4. Сделать массив [1, ..., n]
// 5. Отсортировать его, используя свой компаратор. В результате получить массив перетстановок
// 6. Вывести отсортированный файл

int main(int argc, char const *argv[])
{
    const char *filename = "./onegin.txt";
    size = findSize(filename);
    printf("%d\n", size);

    book = calloc(size, sizeof(char));
    FILE *file = fopen(filename, "rb");
    if (fread(book, sizeof(*book), size, file) != size) {
        puts("Something wrong");
        if (feof(file))
            printf("Error reading: unexpected end of file\n");
        else if (ferror(file)) {
            perror("Error reading");
        }
        free(book);
        fclose(file);
        return 1;
    }
    fclose(file);

    int linesCount = 1; // First line doesn't need \n
    for (int i = 1; i < size; ++i) {
        if (book[i] == '\n') {
            ++linesCount;
            book[i] = '\0';
        }
    }
    printf("%d\n", linesCount);

    int *shuffle = calloc(linesCount, sizeof(int));
    int ptr = 1;
    for (int i = 0; i + 1 < size; ++i) {
        if (book[i] == '\0') {
            shuffle[ptr] = i + 1;
            ++ptr;
        }
    }

    qsort(shuffle, linesCount, sizeof(int), shuffle_cmp);

    FILE *outfile = fopen("./onegin_out.txt", "w");
    for (int i = 0; i < linesCount; ++i) {
        const char *str = &book[shuffle[i]];
        fwrite(str, sizeof(char), strlen(str), outfile);
        fwrite("\n", sizeof(char), 1, outfile);
    }
    fclose(outfile);


    free(book);
    free(shuffle);
    return 0;
}

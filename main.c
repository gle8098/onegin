#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <assert.h>
 
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

char *book = 0;
int size = 0;
int linesCount = 0;
int *lineStartPointer = 0;

int IsMiddleByteInUtf8(unsigned char byte) {
    return byte >= 0x80 && byte < 0xc0;
}

//! Возвращает 1, если при сравнении строк данный символ учитывать не надо (если он является знаком препинания)
int SkipCharacter(int character) {
    return (character >= 0x20 && character <= 0x40) || (character >= 0x5b && character <= 0x60) || (character >= 0x7b && character <= 0x7e);
}

//! Читает полностью символ UTF-8.
//! @param startingPos Ссылка на начальную позицию строки
//! @param direction 1 или -1. Задает направление, в какую сторону от startingPos читать символ.
//! @param value Ссылка на int, в который будет записан символ.
//! @result Возвращает количество прочитанных символов (если direction < 0, то значение отрицательное)
int ReadUtf8Char(const char* staringPos, int direction, int * value) { // Direction should be 1 or -1
    const unsigned char *pos = (const unsigned char *) staringPos;
    int result = 1;
    *value = *pos;

    int continueReading = direction == 1 || IsMiddleByteInUtf8(*pos);
    while (continueReading) {
        pos += direction;
        int isMiddleByte = IsMiddleByteInUtf8(*pos);

        if (direction == 1 && isMiddleByte) {
            *value = *value * 0x100 + *pos;
            ++result;
        } else if(direction == -1) {
            *value = 0x100 * *pos + *value;
            ++result;
        }

        continueReading = isMiddleByte;
    }

    return result * (direction > 0 ? 1 : -1);
}

void test_ReadUtf8Char() {
    int value, bytes;
    char * text;

    // 1
    bytes = ReadUtf8Char("Dz", 1, &value);
    assert(bytes == 1 && value == 0x44);

    // 2
    bytes = ReadUtf8Char("пф", 1, &value);
    assert(bytes == 2 && value == 0xd0bf);

    // 3
    text = "ф\0";
    bytes = ReadUtf8Char(text + 1, -1, &value);
    assert(bytes == -2 && value == 0xd184);

    // 4
    bytes = ReadUtf8Char(text + 2, -1, &value);
    printf("%x\n", value);
    assert(bytes == -1 && value == 0x0);

    // 5
    bytes = ReadUtf8Char("\0abcd", 1, &value);
    assert(bytes == 1 && value == 0x0);
}

int shuffle_direction = 1;
int shuffle_cmp(const void * a, const void * b) {
    int ind1 = *((int*)a);
    int ind2 = *((int*)b);
    //printf("shuffle_cmp: %d %d\n", ind1, ind2);

    // Сейчас ind -- это номер строки, которую надо отсортировать.
    // Запишем в ind номер байта: начала или конца строки
    if (shuffle_direction == -1) {
        // Заменим ind на указатель на последний символ в этой строке
        ind1 = ind1 + 1 == linesCount ? size - 1 : lineStartPointer[ind1 + 1] - 2;
        ind2 = ind2 + 1 == linesCount ? size - 1 : lineStartPointer[ind2 + 1] - 2;
    } else {
        ind1 = lineStartPointer[ind1];
        ind2 = lineStartPointer[ind2];
    }

    while (1) {
        int char1 = 0, char2 = 0;

        do {
            ind1 += ReadUtf8Char(&book[ind1], shuffle_direction, &char1);
        } while (SkipCharacter(char1) && ind1 >= 0);
        if (ind1 < 0) {
            char1 = '\0';
        }

        do {
            ind2 += ReadUtf8Char(&book[ind2], shuffle_direction, &char2);
        } while (SkipCharacter(char2) && ind2 >= 0);
        if (ind2 < 0) {
            char2 = '\0';
        }
        
        //printf("%x %x\n", char1, char2);

        if (char1 != char2) {
            return char1 < char2 ? -1 : 1;
        }

        if (char1 == '\0') {
            return 0;
        }
    }
}

void test_shuffle_cmp() {
    int ind1 = 0;
    int ind2;
    shuffle_direction = 1;

    // 1
    book = "Helao\0Hello";
    ind2 = 6;
    assert(shuffle_cmp(&ind1, &ind2) == -1);

    // 2
    book = "Ab c\0A bb";
    ind2 = 5;
    assert(shuffle_cmp(&ind1, &ind2) == 1);

    // 3
    book = "A'B...C\0A\"B...C";
    ind2 = 8;
    assert(shuffle_cmp(&ind1, &ind2) == 0);

    // 4
    book = "авб\0авг";
    ind2 = 7;
    assert(shuffle_cmp(&ind1, &ind2) == -1);

    // 5
    book = "авя\0авг";
    ind2 = 7;
    assert(shuffle_cmp(&ind1, &ind2) == 1);

    // 6
    shuffle_direction = -1;
    book = "\0я . ба\0г . ба";
    ind1 = 9;
    ind2 = 19;
    assert(shuffle_cmp(&ind1, &ind2) == 1);

    // 7
    shuffle_direction = -1;
    book = "\0гdwф\0яdwф";
    ind1 = 6;
    ind2 = 13;
    assert(shuffle_cmp(&ind1, &ind2) == -1);

    // 8
    shuffle_direction = -1;
    book = "\0\0Сосед наш неуч; сумасбродит;";
    ind1 = 0;
    ind2 = 23 * 2 + 6;
    assert(shuffle_cmp(&ind1, &ind2) == -1);
}

// 1. Узнать размер файла
// 2. Зарезервировать память под весь файл
// 3. Посчитать количество строчек = n
// 4. Сделать массив [1, ..., n]
// 5. Отсортировать его, используя свой компаратор. В результате получить массив перетстановок
// 6. Вывести отсортированный файл

char* ReadBook(const char* filename, int size) {
    char *book = calloc(size, sizeof(char));
    FILE *file = fopen(filename, "rb");
    if (file == 0 || fread(book, sizeof(*book), size, file) != size) {
        puts("Something wrong");
        if (file != 0 && feof(file)) {
            printf("Error reading: unexpected end of file\n");
        } else if (ferror(file)) {
            perror("Error reading");
        }
        free(book);
        fclose(file);
        exit(1);
    }
    fclose(file);

    return book;
}

int* CreateDefaultSortingShuffle() {
    int *shuffle = calloc(linesCount, sizeof(int));
    for (int i = 0; i < linesCount; ++i) {
        shuffle[i] = i;
    }

    shuffle_direction = 1;
    qsort(shuffle, linesCount, sizeof(int), shuffle_cmp);
    return shuffle;
}

int* CreateReverseSortingShuffle() {
    int *shuffle = calloc(linesCount, sizeof(int));
    for (int i = 0; i < linesCount; ++i) {
        shuffle[i] = i;
    }

    shuffle_direction = -1;
    qsort(shuffle, linesCount, sizeof(int), shuffle_cmp);

    // Сейчас все указатели в shuffle указывают на последний символ в строке. Подвинем его на начало строк
    /*for (int i = 0; i < linesCount; ++i) {
        shuffle[i] = lineStartPointer[shuffle[i]];
    }*/
    return shuffle;
}

int main(int argc, char const *argv[])
{
    const char *filename = "./onegin.txt";
    size = findSize(filename);
    printf("%d\n", size);

    book = ReadBook(filename, size);
    
    linesCount = 1; // First line doesn't need \n
    for (int i = 1; i < size; ++i) {
        if (book[i] == '\n') {
            ++linesCount;
            book[i] = '\0';
        }
    }
    printf("%d\n", linesCount);

    lineStartPointer = calloc(linesCount, sizeof(int));
    int ptr = 1;
    for (int i = 0; i + 1 < size; ++i) {
        if (book[i] == '\0') {
            lineStartPointer[ptr] = i + 1;
            ++ptr;
        }
    }


    FILE *outfile = fopen("./onegin_out.txt", "w");


    int *shuffle = CreateDefaultSortingShuffle();    
    for (int i = 0; i < linesCount; ++i) {
        const char *str = &book[lineStartPointer[shuffle[i]]];
        fwrite(str, sizeof(char), strlen(str), outfile);
        fwrite("\n", sizeof(char), 1, outfile);
    }
    free(shuffle);


    fclose(outfile);
    free(book);
    free(lineStartPointer);
    return 0;
}

int main_test() {
    printf("Running tests\n");
    printf("Test for ReadUtf8Char\n");
    test_ReadUtf8Char();
    printf("Test for shuffle_cmp\n");
    test_shuffle_cmp();
    printf("All tests passed\n");
}

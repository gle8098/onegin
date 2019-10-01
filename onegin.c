#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "onegin.h"

long FindFileSize(const char *filename) {
    struct stat st = {};
     
    if (stat(filename, &st) == 0) {
        return st.st_size;
    } else {
        return -1;
    }
}

int IsMiddleByteInUtf8(unsigned char byte) {
    // Проверяет, является ли байт продолжением (а не началом) последовательности символа в UTF-8
    // Проверяем, что старшие два бита = 10
    return byte >= 0x80 && byte < 0xc0;
}

int SkipCharacter(int character) {
    // Выкидывает символы, которые не должны сравниваться во время сортировки
    return (character >= 0x20 && character <= 0x40) // Символы пробел..@
        || (character >= 0x5b && character <= 0x60) // Символы [..`
        || (character >= 0x7b && character <= 0x7e); // Символы {..~
}

int ReadUtf8Char(const char *staringPos, Direction direction, int *value) {
    assert(direction == FORWARD || direction == BACKWARD);

    const unsigned char *pos = (const unsigned char *) staringPos;
    int result = 1;
    *value = *pos;

    int continueReading = direction == FORWARD || IsMiddleByteInUtf8(*pos);
    if (direction == FORWARD) {
        while (continueReading) {
            pos += direction;
            int isMiddleByte = IsMiddleByteInUtf8(*pos);

            if (isMiddleByte) {
                // Припишем прочитанный байт справа от числа
                *value = *value * 0x100 + *pos;
            }

            ++result;
            continueReading = isMiddleByte;
        }
    } else {
        while (continueReading) {
            pos += direction;
            int isMiddleByte = IsMiddleByteInUtf8(*pos);

            // Припишем прочитанный байт слева от числа
            *value = 0x100 * *pos + *value;

            ++result;
            continueReading = isMiddleByte;
        }
    }

    return result * (direction == FORWARD ? 1 : -1);
}


int ShuffleCmp(const StringView *str1, const StringView *str2, Direction shuffle_direction) {
    assert(shuffle_direction == FORWARD || shuffle_direction == BACKWARD);

    const char *str_ptr1 = str1->str;
    const char *str_ptr2 = str2->str;

    if (shuffle_direction == BACKWARD) {
        // Сделаем str_ptr указателем на последний символ в строке
        str_ptr1 += str1->len - 1;
        str_ptr2 += str2->len - 1;
    }

    while (1) {
        int char1 = '\0', char2 = '\0';

        do {
            str_ptr1 += ReadUtf8Char(str_ptr1, shuffle_direction, &char1);
        } while (SkipCharacter(char1) && str_ptr1 >= str1->str);

        do {
            str_ptr2 += ReadUtf8Char(str_ptr2, shuffle_direction, &char2);
        } while (SkipCharacter(char2) && str_ptr2 >= str2->str);
        
        //printf("%x %x\n", char1, char2);

        if (char1 != char2) {
            return char1 - char2;
        }

        if (char1 == '\0') {
            return 0;
        }
    }
}

int ForwardShuffleCmp(const void * a, const void * b) {
    return ShuffleCmp((const StringView *) a, (const StringView *) b, FORWARD);
}
int BackwardShuffleCmp(const void * a, const void * b) {
    return ShuffleCmp((const StringView *) a, (const StringView *) b, BACKWARD);
}

String ReadFileFully(const char *filename) {
    int size = FindFileSize(filename);
    char *book = calloc(size, sizeof(char));
    FILE *file = fopen(filename, "rb");

    if (!file) {
        return (String) {NULL, 0};
    }
    if (fread(book, sizeof(*book), size, file) != size) {
        fclose(file);
        return (String) {NULL, 0};
    }

    fclose(file);
    return (String) {book, size};
}

Book ReadBook(const char *filename) {
    Book book = {NULL, 0, 0, NULL};
    String read_file = ReadFileFully(filename);

    if (read_file.text == NULL) {
        // Error reading file
        return book;
    }
    char *text = book.full_text = read_file.text;
    int size = book.full_text_len = read_file.len;

    int lines_count = 0; // First line doesn't have \n before it
    for (int i = 1; i < size; ++i) {
        if (text[i] == '\n') {
            ++lines_count;
            text[i] = '\0'; // Also replace every \n to \0 so functions with strings will work
        }
    }
    book.lines_count = lines_count;

    StringView *lines = calloc(lines_count, sizeof(StringView));
    int lines_ptr = 0;
    int line_length = 0;
    for (int i = 0; i < size; ++i) {
        if (text[i] == '\0') {
            lines[lines_ptr] = (StringView) { &text[i - line_length], line_length };
            ++lines_ptr;
            line_length = 0;
        } else {
            ++line_length;
        }
    }
    book.lines = lines;

    return book;
}

void FreeBook(const Book *book) {
    free(book->full_text);
    free(book->lines);
}

StringView* CreateSortingShuffle(const Book *book, Direction shuffle_direction) {
    StringView *shuffle = calloc(book->lines_count, sizeof(StringView));
    for (int i = 0; i < book->lines_count; ++i) {
        shuffle[i] = book->lines[i];
    }

    qsort(shuffle, book->lines_count, sizeof(StringView), (shuffle_direction == FORWARD) ? ForwardShuffleCmp : BackwardShuffleCmp);
    return shuffle;
}

void WriteTitleAndContentIntoFile(FILE *file, const char *title, const StringView *content, int lines_count) {
    fwrite(title, sizeof(char), strlen(title), file);
    for (int i = 0; i < lines_count; ++i) {
        fwrite(content[i].str, sizeof(char), content[i].len, file);
        fwrite("\n", sizeof(char), 1, file);
    }
}

void HandleIOError() {
    if (errno == 0) {
        printf("Errno is 0. Unknown error. Exiting.\n");
    } else {
        perror("Error reading");
    }
}

int SortOnegin() {
    Book book = ReadBook("./onegin.txt");

    if (book.full_text == NULL) {
        HandleIOError();
        puts("Unable to read file\n");
        FreeBook(&book);
        return 1;
    }


    FILE *outfile = fopen("./onegin_out.txt", "w");
    if (outfile == NULL) {
        HandleIOError();
        puts("Unable to open output file");
        FreeBook(&book);
        return 1;
    }

    StringView *shuffle = CreateSortingShuffle(&book, FORWARD);
    WriteTitleAndContentIntoFile(outfile, "\tSORTING ONEGIN IN DEFAULT ORDER\n", shuffle, book.lines_count);
    free(shuffle);

    shuffle = CreateSortingShuffle(&book, BACKWARD);
    WriteTitleAndContentIntoFile(outfile, "\n\tSORTING ONEGIN IN REVERSE ORDER\n", shuffle, book.lines_count);
    free(shuffle);

    WriteTitleAndContentIntoFile(outfile, "\n\tORIGINAL ONEGIN\n\n", book.lines, book.lines_count);

    fclose(outfile);
    FreeBook(&book);
    return 0;
}
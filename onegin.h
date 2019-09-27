#ifndef _ONEGIN_H
#define _ONEGIN_H

#include <stdio.h>

/* Structures */

//! Структура для неизменяемой строки
typedef struct {
    const char* str;
    int len;
} StringView;


//! Структура для неконстантной строки
typedef struct {
    char *text;
    int len;
} String;

//! Структура, описывающая книгу.
typedef struct {
	//! Полносью прочитанная книга. С одним изменением: каждая строчка кончается '\0' вместо '\n'.
    char *full_text;
    //! Количество символом в книге
    int full_text_len;
    //! Количество строк в книге
    int lines_count;
    //! Строки книги
    StringView *lines;
} Book;


/* UTF-8 related functions */

//! Принимает байт из текста в UTF-8 и возвращает 0, если с этого байта начинается (и, возможно, кончается)
//! новый символ в кодировке. Иначе возвращает 1.
int IsMiddleByteInUtf8(unsigned char byte);

//! Возвращает 1, если при сравнении строк данный символ учитывать не надо (если он является знаком препинания)
int SkipCharacter(int character);

//! Читает полностью символ UTF-8.
//! @param startingPos Ссылка на начальную позицию строки
//! @param direction 1 или -1. Задает направление, в какую сторону от startingPos читать символ.
//! @param value Ссылка на int, в который будет записан символ.
//! @result Возвращает количество прочитанных символов (если direction < 0, то значение отрицательное)
int ReadUtf8Char(const char *staringPos, int direction, int *value);


/* Sorting functions */

//! Сравнивает две строки.
//! @param str1 Первая строка типа StringView
//! @param str2 Вторая строка типа StringView
//! @param shuffle_direction Равен 1, если требуется сравнить строки с начала, или равен -1 для их сравнения с конца.
//! @result Число меньше 0, если str1 < str2; больше 0, если str1 > str2; равное 0, если str1 == str2.
int ShuffleCmp(const StringView *str1, const StringView *str2, int shuffle_direction);

//! Компаратор для qsort для сравнения StringView с начала.
int ForwardShuffleCmp(const void *, const void *);

//! Компаратор для qsort для сравнения StringView с конца.
int BackwardShuffleCmp(const void *, const void *);

//! Генерирует перестановку строк в книге.
//! @param book Книга
//! @param shuffle_direction 1 или -1: определяет тип сортировки. См. ShuffleCmp
//! @result Массив строк типа StringView в отсортированном порядке.
StringView* CreateSortingShuffle(const Book *book, int shuffle_direction);


/* I/O functions */

//! Возвращает размер файла в байтах.
long FindFileSize(const char *filename);

//! Возвращает весь файл в виде одной строки. Выделяет память под всю строку единожды.
String ReadFileFully(const char *filename);

//! Читает книгу из файла: записывает все ее содержимое в память, выделяет строки в книге и их длины,
//! заменяет '\n' на '\0', чтобы со строками могли работать функции для строк из стандартной библиотеки.
Book ReadBook(const char *filename);

//! Освобождает память, выделенную под отдельные поля структуры Book.
void FreeBook(const Book *book);

//! Записывает в файл file раздел книги с заголовком title и содержимым content размером lines_count строк.
void WriteTitleAndContentIntoFile(FILE *file, const char *title, const StringView *content, int lines_count);

//! Выводит на экран сообщение об ошибке последней вызванной файловой операции.
void HandleIOError();

//! Сортирует Онегина.
int SortOnegin();

#endif

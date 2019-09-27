#include "onegin.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

StringView CreateStringView(const char* str) {
    return (StringView) {str, strlen(str)};
}

void test_ReadUtf8Char() {
    int value, bytes;
    char *text;

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

void test_ShuffleCmp() {
    StringView str1, str2;

    // 1
    str1 = CreateStringView(" ");
    str2 = CreateStringView("I");
    assert(ShuffleCmp(&str1, &str2, -1) < 0);

    // 2
    str1 = CreateStringView("A./bcdф{}гйц?");
    str2 = CreateStringView("Abc*(d)фг ""  й  ц");
    assert(ShuffleCmp(&str1, &str2, 1) == 0);

    // 3
    str1 = CreateStringView("Как Сон младенца, как луна");
    str2 = CreateStringView("Как Сади некогда сказал.");
    assert(ShuffleCmp(&str1, &str2, 1) > 0);

    // 4
    str1 = CreateStringView("Ах, няня, няня, я тоскую,");
    str2 = CreateStringView("Песенку заветную,");
    assert(ShuffleCmp(&str1, &str2, -1) < 0);
}

void test_ReadBook() {
    Book book = ReadBook("./onegin.txt");

    printf("Testing ReadBook function.\nThe first 20 lines of onegin.txt:\n");
    for (int i = 0; i < 20; ++i) {
        printf("\t%d %s\n", book.lines[i].len, book.lines[i].str);
    }

    printf("The last 20 lines of onegin:\n");
    for (int i = book.lines_count - 20; i < book.lines_count; ++i) {
        printf("\t%d %s\n", book.lines[i].len, book.lines[i].str);
    }

    FreeBook(&book);
}

int test_all() {
    printf("Running tests\n");
    printf("Test for ReadUtf8Char\n");
    test_ReadUtf8Char();
    printf("Test for ShuffleCmp\n");
    test_ShuffleCmp();
    printf("Test for ReadBook\n");
    test_ReadBook();
    printf("All tests passed\n");
}

int main() {
    test_all();
    return 0;
}

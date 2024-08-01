#include "spieldata.h"
#include "bibs.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>


spielfeld* spielfeld_init (unsigned long width, unsigned long height) {
    spielfeld* sf = malloc(sizeof(spielfeld));
    if (sf == NULL)
        return NULL;
    memset(sf, 0, sizeof(spielfeld));
    sf->width = width;
    sf->height = height;
    sf->field = malloc(sizeof(width * height));
    if (sf->field == NULL)
        return NULL;
    return sf;
}

bool spielfeld_update(spielfeld* sf, char* new_field) {
    if (sf == 0 || sf->height == 0 || sf->width == 0 || sf->field == NULL) {
        printf("SF :%p\n", sf);
        printf("SF->HEIGHT :%ld\n", sf->height);
        printf("SF->WIDTH :%ld\n", sf->width);
        printf("SF->FIELD :%p\n", sf->field);
        perror("Wrong spielfeld instance!\n");
        return false;
    }
    assert((sf->height * sf->width) <= sf->capacity);
    memcpy(sf->field, new_field, sf->width * sf->height);
    return true;
}

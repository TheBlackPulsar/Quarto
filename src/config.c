#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

typedef struct {
    char *name;
    char *value;
} config_item;

typedef struct {
    config_item **vector;
    unsigned long length;
    unsigned long capacity;
} config_vector;

config_item *config_item_new(char *name, char*value) {
    config_item *item = malloc(sizeof(config_item));
    if (item == NULL) {
        return NULL;
    }
    memset(item, 0, sizeof(config_item));
    item->name = malloc(strlen(name)+1);
    if (item->name == NULL) {
        free(item);
        return NULL;
    }
    memset(item->name, 0, strlen(name)+1);
    memcpy(item->name, name, strlen(name));
    item->value = malloc(strlen(value)+1);
    if (item->value == NULL) {
        free(item->name);
        free(item);
        return NULL;
    }
    memset(item->value, 0, strlen(value)+1);
    memcpy(item->value, value, strlen(value));
    return item;
}

void config_item_free(config_item *item) {
    free(item->name);
    free(item->value);
    free(item);
}

config_vector* config_vector_new() {
    unsigned long init_capacity = 4;
    config_vector *cv = malloc(sizeof(config_vector));
    if (cv == NULL) {
        return NULL;
    }
    memset(cv, 0, sizeof(config_vector));
    cv->capacity = init_capacity;
    cv->length = 0;
    cv->vector = malloc(init_capacity * sizeof(config_item));
    if (cv->vector == NULL) {
        free(cv);
        return NULL;
    }
    memset(cv->vector, 0, init_capacity * sizeof(config_item));
    return cv;
}

void config_vector_print(config_vector *cv) {
    for (unsigned int i = 0; i < cv->length; i++) {
        config_item *item = cv->vector[i];
        printf("key: %s, value: %s\n", item->name, item->value);
    }
}

config_item* config_vector_find(config_vector *cv, char *name) {
    for (unsigned int i = 0; i < cv->length; i++) {
        config_item *item = cv->vector[i];
        if (strcmp(item->name, name) == 0) {
            return item;
        }
    }
    return NULL;
}

void config_vector_append(config_vector *cv, config_item *new_item) {
    if (cv->length + 1 > cv->capacity) {
        cv->capacity *= 2;
        cv->vector = realloc(cv->vector, cv->capacity * sizeof(config_item));
        if (cv->vector == NULL) {
            printf("Unable to expand the config vector.\n");
            return;
        }
    }
    // memcpy(cv->vector[cv->length], new_item, sizeof(config_item));
    cv->vector[cv->length] = new_item;
    cv->length++;
}

void config_vector_clear(config_vector *cv) {
    for (unsigned int i = 0; i < cv->length; i++) {
        config_item_free(cv->vector[i]);
    }
    free(cv->vector);
    free(cv);
}

config_vector* read_config(char *config_path) {
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    if (config_path == NULL || strlen(config_path) <= 0) {
        config_path = "client.conf";
    }
    fp = fopen(config_path, "r");
    if (fp == NULL)
        return NULL;

    char * regexString = "^[ ]*([#]?)[ ]*([A-Za-z0-9_.]+)[ ]*=[ ]*([A-Za-z0-9_.]+)"; //Die Situationen mit "Parametervariable = Parameterwert"
    char * regex_emtpy_line = "^[ ]+[^A-Z^a-z^0-9^_.]"; //Die andere Situationen ausser "Parametervariable = Parameterwert"
    size_t max_groups = 4;

    regex_t regex_compiled;
    regex_t regex_empty;
    regmatch_t group_array[max_groups];

    if (regcomp(&regex_compiled, regexString, REG_EXTENDED) || regcomp(&regex_empty, regex_emtpy_line, REG_EXTENDED))
    {
        printf("Could not compile regular expression.\n");
        return NULL;
    };

    config_vector *cv = config_vector_new();

    unsigned long line_cnt = 0;
    while ((read = getline(&line, &len, fp)) != -1) {
        line_cnt++;
        if (regexec(&regex_compiled, line, max_groups, group_array, 0) == 0)
        {
            unsigned int g = 0;
            for (g = 0; g < max_groups; g++)
            {
                if (group_array[g].rm_so == -1)
                break;  
            }
            if (g <= 3){
                printf("config file error at line %ld\n", line_cnt);
                continue;
            }
            if (group_array[1].rm_eo - group_array[1].rm_so != 0) {
                continue;
            } 
            char key[group_array[2].rm_eo - group_array[2].rm_so + 1];
            strncpy(key, line + group_array[2].rm_so, sizeof(key));
            key[sizeof(key)-1] = '\0';
            // key[group_array[2].rm_eo - group_array[2].rm_so] = '\0';
            char val[group_array[3].rm_eo - group_array[3].rm_so + 1];
            strncpy(val, line + group_array[3].rm_so, sizeof(val));
            val[sizeof(val)-1] = '\0';
            // val[group_array[3].rm_eo - group_array[3].rm_so] = '\0';
            config_item *item = config_item_new(key, val);
            config_vector_append(cv, item);
        } else if (regexec(&regex_empty, line, max_groups, group_array, 0) == 0 || strlen(line) <= 2){
            continue;
        } else {
            printf("Error parsing config file at line %ld\n", line_cnt);
            printf("%s\n", line);
        }

    }

    regfree(&regex_empty);
    regfree(&regex_compiled);
    fclose(fp);
    if (line)
        free(line);
    return cv;
}

int test_main() {
    config_vector *cv = read_config(NULL);
    // config_vector_print(cv);
    config_item *ci;
    if ((ci = config_vector_find(cv, "ip")) != NULL)
        printf("Find ip: %s\n", ci->value);
    if ((ci = config_vector_find(cv, "port_nummer")) != NULL)
        printf("Find port_nummer: %s\n", ci->value);
    if ((ci = config_vector_find(cv, "HOSTNAME1")) != NULL)
        printf("Find HOSTNAME1: %s\n", ci->value);
    else
        printf("Error finding HOSTNAME1\n");
    config_vector_clear(cv);
    return 0;
}

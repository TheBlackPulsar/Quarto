#ifndef __QUARTO_CONFIG_H__
#define __QUARTO_CONFIG_H__

typedef struct {
    char *name;
    char *value;
} config_item;

typedef struct {
    config_item **vector;
    unsigned long length;
    unsigned long capacity;
} config_vector;

config_item *config_item_new(char *name, char*value);
void config_item_free(config_item *item);
config_vector* config_vector_new();
void config_vector_print(config_vector *cv);
config_item* config_vector_find(config_vector *cv, char *name);
void config_vector_append(config_vector *cv, config_item *new_item);
void config_vector_clear(config_vector *cv);
config_vector* read_config(char *config_path);


#endif

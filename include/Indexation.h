#ifndef INDEXATION_H
#define INDEXATION_H

#include <glib.h>

// Estrutura para guardar a informação de um documento
typedef struct DocumentIndex DocumentIndex;

// Função para inicializar a HashTable
void indexation_init();

// Função para inicializar a HashTable com parâmetros específicos
void indexation_init_with_params(const char *document_folder, int cache_size);

// Função para destruir a HashTable
void indexation_destroy();

// Função para imprimir todos os documentos na tabela
void indexation_printAll();

// Função para remover um documento da tabela
int indexation_remove(const int key);

// Função para verificar se um documento existe na tabela
int indexation_exists(const int key);

// Getters
char *getTitle(const int key);
char **getAuthors(const int key);
int getYear(const int key);
char *getPath(const int key);

// Função para adicionar um documento à tabela
int indexation_add(const char *title, const char *authors[], int year,
                   const char *path);

// Função para contar o número de linhas que contêm uma keyword
int indexation_count_lines_with_keyword(const int key, const char *keyword);

// Função para listar documentos que contêm uma keyword
int *indexation_list_by_keyword(const char *keyword, int *r);

// Função para salvar os metadados
void save_metadata();

// Função para carregar os metadados
void load_metadata();

#endif
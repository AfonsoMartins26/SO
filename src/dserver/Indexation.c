#include "../../include/Indexation.h"

#include <fcntl.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER_SIZE 512
#define METADATA_FILE "/tmp/document_metadata.txt"

typedef struct DocumentIndex {
  char *title;
  char **authors;
  int year;
  char *path;
} DocumentIndex;

// Tabela de hash para armazenar a indexação (chave -> documento)
static GHashTable *document_table;

// Variável para gerar chaves únicas e incrementais para cada indexação
static int id = 1;

// Store document folder path globally
static char *global_document_folder = NULL;

static int global_cache_size = 0;


void indexation_init_with_params(const char *document_folder, int cache_size) {
  if (document_table != NULL) {
    g_hash_table_destroy(document_table);
  }
  
  document_table = g_hash_table_new_full(g_int_hash, g_int_equal, free, free);
  
  if (global_document_folder != NULL) {
    free(global_document_folder);
  }
  global_document_folder = strdup(document_folder);

  global_cache_size = cache_size;
}

// Função para limpar a memória
void indexation_destroy() { 
  g_hash_table_destroy(document_table); 
  
  if (global_document_folder != NULL) {
    free(global_document_folder);
    global_document_folder = NULL;
  }
}

void indexation_printAll() {
  GHashTableIter iter;
  gpointer key, value;
  g_hash_table_iter_init(&iter, document_table);
  while (g_hash_table_iter_next(&iter, &key, &value)) {
    printf("Key: %d\n", *(int *)key);
    DocumentIndex *doc = (DocumentIndex *)value;
    printf("Title: %s\n", doc->title);
    for (int i = 0; doc->authors[i] != NULL; i++) {
      printf("Author %d: %s\n", i, doc->authors[i]);
    }
    printf("Year: %d\n", doc->year);
    printf("Path: %s\n", doc->path);
  }
}

int indexation_exists(const int key) {
  DocumentIndex *doc =
      (DocumentIndex *)g_hash_table_lookup(document_table, (gpointer)&key);
  if (!doc) return 0;
  return 1;
}

char *getTitle(const int key) {
  DocumentIndex *doc =
      (DocumentIndex *)g_hash_table_lookup(document_table, (gpointer)&key);
  if (!doc) return NULL;
  return doc->title;
}

char **getAuthors(const int key) {
  DocumentIndex *doc =
      (DocumentIndex *)g_hash_table_lookup(document_table, (gpointer)&key);
  if (!doc) return NULL;
  return doc->authors;
}

int getYear(const int key) {
  DocumentIndex *doc =
      (DocumentIndex *)g_hash_table_lookup(document_table, (gpointer)&key);
  if (!doc) return -1;
  return doc->year;
}

char *getPath(const int key) {
  DocumentIndex *doc =
      (DocumentIndex *)g_hash_table_lookup(document_table, (gpointer)&key);
  if (!doc) return NULL;
  return doc->path;
}

int indexation_add(const char *title, const char *authors[], int year,
                  const char *path) {
  if (id > global_cache_size) {
    perror("Erro: Cache size excedido");
    return -1;  // Retorna um valor indicando falha
  }
  
  DocumentIndex *doc = malloc(sizeof(DocumentIndex));
  doc->title = strdup(title);
  
  int num_authors = 0;
  while (authors[num_authors] != NULL) num_authors++;
  doc->authors = malloc(sizeof(char *) * (num_authors + 1));
  for (int i = 0; i < num_authors; i++) {
    doc->authors[i] = strdup(authors[i]);
  }
  doc->authors[num_authors] = NULL;
  
  doc->year = year;
  doc->path = malloc(strlen(global_document_folder) + strlen(path) + 1);
  strcpy(doc->path, global_document_folder);
  strcat(doc->path, path);
  
  int *key_ptr = malloc(sizeof(int));
  *key_ptr = id++;
  
  if (g_hash_table_contains(document_table, key_ptr)) {
    perror("Erro: Chave duplicada detectada");
    free(key_ptr);
    free(doc->title);
    for (int i = 0; doc->authors[i] != NULL; i++) {
      free(doc->authors[i]);
    }
    free(doc->authors);
    free(doc->path);
    free(doc);
    return -1;
  }

  if (*key_ptr == -1) {
    perror("Erro:Cache size excedido");
  } else {
    g_hash_table_insert(document_table, key_ptr, doc);
  }

  return *key_ptr;
}

// Função para remover um documento
int indexation_remove(const int key) {
  if (!g_hash_table_remove(document_table, &key)) {
    return 0;
  }
  return 1;
}

int indexation_count_lines_with_keyword(const int key, const char *keyword) {
  DocumentIndex *doc =
      (DocumentIndex *)g_hash_table_lookup(document_table, (gpointer)&key);
  if (doc == NULL) {
    perror("Erro: Documento não encontrado");
    return -1;
  }

  int pipe_fds[2];
  if (pipe(pipe_fds) == -1) {
    perror("Erro ao criar o pipe");
    return -1;
  }

  pid_t pid = fork();
  if (pid == -1) {
    perror("Erro ao fazer fork");
    return -1;
  }

  if (pid == 0) {
    // Processo filho
    close(pipe_fds[0]);
    dup2(pipe_fds[1], STDOUT_FILENO);
    close(pipe_fds[1]);

    // The path is already complete (with document_folder) from the structure
    execlp("grep", "grep", "-c", keyword, doc->path, (char *)NULL);
    perror("Erro ao executar grep");
    _exit(1);
  } else {
    // Processo pai
    close(pipe_fds[1]);

    char buffer[BUFFER_SIZE];
    int bytes_read = read(pipe_fds[0], buffer, sizeof(buffer) - 1);
    close(pipe_fds[0]);

    if (bytes_read == -1) {
      perror("Erro ao ler do pipe");
      return -1;
    }

    buffer[bytes_read] = '\0';
    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
      return atoi(buffer);
    } else {
      perror("Erro ao executar grep");
      return -1;
    }
  }
}

int *indexation_list_by_keyword(const char *keyword, int *N) {
  GHashTableIter iter;
  gpointer key, value;
  int *results = malloc(sizeof(int) * g_hash_table_size(document_table));
  int result_count = 0;

  g_hash_table_iter_init(&iter, document_table);

  while (g_hash_table_iter_next(&iter, &key, &value)) {
    DocumentIndex *doc = (DocumentIndex *)value;
    // Cria o pipe para a comunicação entre grep e o processo principal
    int pipe_fds[2];
    if (pipe(pipe_fds) == -1) {
      perror("Erro ao criar o pipe");
      free(results);
      return 0;
    }

    pid_t pid = fork();
    if (pid == -1) {
      perror("Erro ao fazer fork");
      free(results);
      return 0;
    }

    if (pid == 0) {
      // Processo FILHO - Executar grep
      close(pipe_fds[0]);
      dup2(pipe_fds[1], STDOUT_FILENO);
      close(pipe_fds[1]);

      // The path is already complete (with document_folder) from the structure
      execlp("grep", "grep", "-q", keyword, doc->path, (char *)NULL);
      perror("Erro ao executar grep");
      exit(1);
    } else {
      // Processo PAI
      close(pipe_fds[1]);

      int status;
      waitpid(pid, &status, 0);
      // Verifica se o grep encontrou a palavra
      if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        results[result_count++] = *((int *)key);
      }

      close(pipe_fds[0]);
    }
  }

  results = realloc(results, sizeof(int) * result_count);
  if (result_count == 0) {
    free(results);
    return 0;
  }

  *N = result_count;

  return results;
}

void save_metadata() {
  int fd = open(METADATA_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd == -1) {
    perror("Erro ao abrir arquivo de metadados para escrita");
    return;
  }

  GHashTableIter iter;
  gpointer key, value;
  g_hash_table_iter_init(&iter, document_table);

  char buffer[BUFFER_SIZE];
  while (g_hash_table_iter_next(&iter, &key, &value)) {
    DocumentIndex *doc = (DocumentIndex *)value;
    int doc_key = *(int *)key;

    // Serialize authors
    char authors_str[BUFFER_SIZE] = {0};
    for (int i = 0; doc->authors[i] != NULL; i++) {
      strcat(authors_str, doc->authors[i]);
      if (doc->authors[i + 1] != NULL) {
        strcat(authors_str, ";");
      }
    }

    // Format document metadata into buffer
    int len = snprintf(buffer, BUFFER_SIZE, "%d|%s|%s|%d|%s\n", doc_key,
                       doc->title, authors_str, doc->year, doc->path);

    if (len < 0 || write(fd, buffer, len) != len) {
      perror("Erro ao escrever no arquivo de metadados");
      close(fd);
      return;
    }
  }

  close(fd);
}

void load_metadata() {
  int fd = open(METADATA_FILE, O_RDONLY);
  if (fd == -1) {
    // No existing metadata file is not an error
    return;
  }

  char buffer[BUFFER_SIZE];
  ssize_t bytes_read;
  char line[BUFFER_SIZE];
  int line_pos = 0;

  while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
    for (ssize_t i = 0; i < bytes_read; i++) {
      if (buffer[i] == '\n') {
        line[line_pos] = '\0';

        // Parse line
        char *key_str = strtok(line, "|");
        char *title = strtok(NULL, "|");
        char *authors_str = strtok(NULL, "|");
        char *year_str = strtok(NULL, "|");
        char *path = strtok(NULL, "|");

        if (!key_str || !title || !authors_str || !year_str || !path) {
          perror("Erro: linha de metadados inválida");
          line_pos = 0;
          continue;
        }

        // Convert key and year
        //int key = atoi(key_str);
        int year = atoi(year_str);

        // Split authors
        char *authors[20] = {0};  // Assume max 20 authors
        int author_count = 0;
        char *author = strtok(authors_str, ";");
        while (author && author_count < 19) {
          authors[author_count++] = author;
          author = strtok(NULL, ";");
        }
        authors[author_count] = NULL;

        // Restore the entry - path already has document_folder
        indexation_add(title, (const char **)authors, year, path);

        line_pos = 0;
      } else {
        line[line_pos++] = buffer[i];
        if (line_pos >= BUFFER_SIZE - 1) {
          perror("Erro: linha de metadados muito longa");
          close(fd);
          return;
        }
      }
    }
  }

  if (bytes_read == -1) {
    perror("Erro ao ler o arquivo de metadados");
  }

  close(fd);
}
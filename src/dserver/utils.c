#include <fcntl.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>

#include "../../include/Indexation.h"

#define BUFFER_SIZE 512

char *deleteChar(char *s, char ch) {
  int i, j;
  int len = strlen(s);
  for (i = j = 0; i < len; i++) {
    if (s[i] != ch) {
      s[j++] = s[i];
    }
  }
  s[j] = '\0';
  return s;
}

char **split_authors(const char *authors) {
  int count = 1;
  for (const char *tmp = authors; *tmp; ++tmp) {
    if (*tmp == ';') {
      count++;
    }
  }

  char **auths = malloc((count + 1) * sizeof(char *));
  if (!auths) {
    return NULL;
  }

  char *authors_copy = strdup(authors);
  if (!authors_copy) {
    free(auths);
    return NULL;
  }

  char *token = strtok(authors_copy, ";");
  int i = 0;
  while (token != NULL) {
    auths[i] = strdup(token);
    i++;
    token = strtok(NULL, ";");
  }
  auths[i] = NULL;

  free(authors_copy);
  return auths;
}

void process_command(char *command, char *response) {
  char *token = strtok(command, " ");
  if (token == NULL) {
    strcpy(response, "Comando inválido");
    return;
  }

  if (strcmp(token, "-a") == 0) {
    char *title = NULL;
    char *authors = NULL;
    char *year_str = NULL;
    char *path = NULL;

    char *rest = command + strlen(token) + 1;

    if (*rest == '"') {
      rest++;
      title = rest;

      while (*rest && *rest != '"') rest++;
      if (*rest == '\0') {
        strcpy(response, "Erro: aspas de fechamento ausentes");
        return;
      }
      if (*rest == '"') {
        *rest = '\0';
        rest++;
      }
    }

    while (*rest && isspace((unsigned char)*rest)) rest++;

    if (*rest == '"') {
      rest++;
      authors = rest;

      while (*rest && *rest != '"') rest++;
      if (*rest == '\0') {
        strcpy(response, "Erro: aspas de fechamento ausentes");
        return;
      }
      if (*rest == '"') {
        *rest = '\0';
        rest++;
      }
    }

    while (*rest && isspace((unsigned char)*rest)) rest++;

    year_str = rest;
    while (*rest && !isspace((unsigned char)*rest)) rest++;
    if (*rest) {
      *rest = '\0';
      rest++;
    }

    while (*rest && isspace((unsigned char)*rest)) rest++;

    if (*rest == '"') {
      rest++;
      path = rest;

      while (*rest && *rest != '"') rest++;
      if (*rest == '\0') {
        strcpy(response, "Erro: aspas de fechamento ausentes");
        return;
      }
      if (*rest == '"') {
        *rest = '\0';
        rest++;
      }
    } else {
      path = rest;
    }
    char *year_final = deleteChar(year_str, '"');
    int year = atoi(year_final);

    char **auths = split_authors(authors);

    int key_added = indexation_add(title, (const char **)auths, year, path);
    if (key_added == -1) {
      strcpy(response, "Erro: Cache size excedido");
      free(auths);
      return;
    }
    indexation_printAll();
    snprintf(response, BUFFER_SIZE,
             "Documento adicionado com sucesso com a chave %d", key_added);
  } else if (strcmp(token, "-d") == 0) {
    int key = atoi(strtok(NULL, " "));
    if (indexation_remove(key) == 0) {
      strcpy(response, "Documento não encontrado");
    } else {
      strcpy(response, "Documento removido com sucesso");
    }
  } else if (strcmp(token, "-c") == 0) {
    int key = atoi(strtok(NULL, " "));
    if (indexation_exists(key) == 0) {
      strcpy(response, "Documento não encontrado");
    } else {
      char *title = getTitle(key);
      char **authors = getAuthors(key);
      int year = getYear(key);
      char *path = getPath(key);

      char *authors_str = malloc(BUFFER_SIZE);
      for (int i = 0; authors[i] != NULL; i++) {
        authors_str = strcat(authors_str, authors[i]);
        if (authors[i + 1] != NULL) {
          authors_str = strcat(authors_str, "; ");
        }
      }
      snprintf(response, BUFFER_SIZE,
               "\nTítulo: %s\nAutores: %s\nAno: %d\nPath: %s", title,
               authors_str, year, path);
    }
  } else if (strcmp(token, "-l") == 0) {
    int key = atoi(strtok(NULL, " "));
    char *keyword = strtok(NULL, " ");
    int count = indexation_count_lines_with_keyword(key, keyword);
    sprintf(response, "Número de linhas com a palavra '%s': %d", keyword,
            count);
  } else if (strcmp(token, "-s") == 0) {
    char *keyword = strtok(NULL, " ");
    int max_processes = atoi(strtok(NULL, " "));
    int n = 0;
    int *document_keys = indexation_list_by_keyword(keyword, &n);

    if (n == 0) {
      strcpy(response, "Nenhum documento encontrado com a palavra");
    } else {
      // Clear the response buffer first
      memset(response, 0, BUFFER_SIZE);
      strcpy(response, "Documentos encontrados com a palavra\n");

      int active_processes = 0;
      pid_t pids[max_processes];

      // Pipe to communicate between child and parent
      int pipefd[2];
      if (pipe(pipefd) == -1) {
        strcpy(response, "Erro ao criar pipe");
        free(document_keys);
        return;
      }

      for (int i = 0; i < n; i++) {
        pid_t pid = fork();
        if (pid == 0) {
          // Child process
          close(pipefd[0]);  // Close read end

          char key_str[20];
          sprintf(key_str, "Key: %d\n", document_keys[i]);

          // Write to pipe
          write(pipefd[1], key_str, strlen(key_str));

          close(pipefd[1]);
          exit(0);
        } else if (pid > 0) {
          pids[active_processes++] = pid;
        } else {
          strcpy(response, "Erro ao criar processo");
          free(document_keys);
          return;
        }
      }

      close(pipefd[1]);  // Close write end

      // Wait for all children and read from pipe
      for (int i = 0; i < active_processes; i++) {
        waitpid(pids[i], NULL, 0);

        char buffer[100];
        ssize_t bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1);
        if (bytes_read > 0) {
          buffer[bytes_read] = '\0';
          strcat(response, buffer);
        }
      }

      close(pipefd[0]);
      free(document_keys);
    }
  } else if (strcmp(token, "-f") == 0) {
    save_metadata();
    indexation_destroy();
    strcpy(response, "Servidor encerrado com sucesso");
  } else {
    strcpy(response, "Comando inválido");
  }
}
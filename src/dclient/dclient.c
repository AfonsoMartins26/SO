#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define FIFO_CLIENT_TO_SERVER "/tmp/client_to_server_fifo"
#define FIFO_SERVER_TO_CLIENT "/tmp/server_to_client_fifo"
#define BUFFER_SIZE 512

int sendToServer(char* command) {
  char message[BUFFER_SIZE];
  char response[BUFFER_SIZE];

  snprintf(message, BUFFER_SIZE, "%s", command);

  int fd_write = open(FIFO_CLIENT_TO_SERVER, O_WRONLY | O_NONBLOCK);
  if (fd_write < 0) {
    perror("Erro: O servidor não está disponível");
    return -1;
  }

  write(fd_write, message, strlen(message) + 1);
  close(fd_write);

  int fd_read = open(FIFO_SERVER_TO_CLIENT, O_RDONLY);
  if (fd_read < 0) {
    perror("Erro ao abrir o FIFO para leitura");
    return -1;
  }

  if (read(fd_read, response, BUFFER_SIZE) > 0) {
    printf("Resposta do servidor: %s\n", response);
  } else {
    perror("Erro ao ler a resposta do servidor");
  }
  close(fd_read);

  return 0;
}

int main(int argc, char* argv[]) {
  if (argc < 2 || argc > 6) {
    printf("Usage:\n");
    printf(
        "  Adicionar uma indexação: %s -a 'titulo' 'autor1;autor2;autor3' "
        "'ano' 'filepath'\n",
        argv[0]);
    printf("  Consultar uma indexação: %s -c 'chave'\n", argv[0]);
    printf("  Eliminar uma indexação: %s -d 'chave'\n", argv[0]);
    printf("  Contar linhas com uma palavra: %s -l 'chave' 'palavra-chave'\n",
           argv[0]);
    printf(
        "  Listar os ficheiros que contêm uma palavra %s -s 'palavra-chave' "
        "'nr_processes'\n",
        argv[0]);
    printf("  Parar o servidor: %s -f\n", argv[0]);
    return 1;
  }

  if (strcmp(argv[1], "-a") == 0) {
    if (argc != 6) {
      printf("Usage: %s -a 'titulo' 'autor1;autor2;autor3' 'ano' 'filepath'\n",
             argv[0]);
      return 1;
    }
    // Adicionar uma indexação (RECEBE o título, o autor(es), o ano e o
    // filepath, DEVOLVE a chave do ficheiro)
    char command[BUFFER_SIZE];
    snprintf(command, BUFFER_SIZE, "%s \"%s\" \"%s\" \"%s\" \"%s\"", argv[1],
             argv[2], argv[3], argv[4], argv[5]);
    int res = sendToServer(command);
    if (res < 0) {
      return 1;
    }
  }

  if (strcmp(argv[1], "-c") == 0) {
    if (argc != 3) {
      printf("Usage: %s -c 'chave'\n", argv[0]);
      return 1;
    }
    // Consultar uma indexação (RECEBE a chave do ficheiro, DEVOLVE o titulo, o
    // autor(es), o ano e o filepath)
    char command[BUFFER_SIZE];
    snprintf(command, BUFFER_SIZE, "%s %s", argv[1], argv[2]);
    int res = sendToServer(command);
    if (res < 0) {
      return 1;
    }
  }

  if (strcmp(argv[1], "-d") == 0) {
    if (argc != 3) {
      printf("Usage: %s -d 'chave'\n", argv[0]);
      return 1;
    }
    // Eliminar uma indexação (RECEBE a chave do ficheiro, DEVOLVE 1 se
    // eliminou, 0 se não eliminou)
    char command[BUFFER_SIZE];
    snprintf(command, BUFFER_SIZE, "%s %s", argv[1], argv[2]);
    int res = sendToServer(command);
    if (res < 0) {
      return 1;
    }
  }

  if (strcmp(argv[1], "-l") == 0) {
    if (argc != 4) {
      printf("Usage: %s -l 'chave' 'palavra-chave'\n", argv[0]);
      return 1;
    }
    // Devolver o número de linhas do ficheiro que contêm uma palavra-chave
    // (RECEBE a chave do ficheiro e a palavra-chave, DEVOLVE o número de
    // linhas)
    char command[BUFFER_SIZE];
    snprintf(command, BUFFER_SIZE, "%s %s %s", argv[1], argv[2], argv[3]);
    int res = sendToServer(command);
    if (res < 0) {
      return 1;
    }
  }

  if (strcmp(argv[1], "-s") == 0) {
    if (argc != 4) {
      printf("Usage: %s -s 'palavra-chave' 'nr_processes'\n", argv[0]);
      return 1;
    }
    // Devolver as chaves dos ficheiros que contêm uma palavra-chave (RECEBE a
    // palavra-chave e o número máximo de processos, DEVOLVE a lista das chaves
    // dos ficheiros)
    char command[BUFFER_SIZE];
    snprintf(command, BUFFER_SIZE, "%s %s %s", argv[1], argv[2], argv[3]);
    int res = sendToServer(command);
    if (res < 0) {
      return 1;
    }
  }
  if (strcmp(argv[1], "-f") == 0) {
    if (argc != 2) {
      printf("Usage: %s -f\n", argv[0]);
      return 1;
    }
    // Parar o servidor
    char command[BUFFER_SIZE];
    snprintf(command, BUFFER_SIZE, "%s", argv[1]);
    int res = sendToServer(command);
    if (res < 0) {
      return 1;
    }
  }

  return 0;
}
#include <fcntl.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../../include/Indexation.h"
#include "../../include/utils.h"

#define FIFO_CLIENT_TO_SERVER "/tmp/client_to_server_fifo"
#define FIFO_SERVER_TO_CLIENT "/tmp/server_to_client_fifo"
#define BUFFER_SIZE 512
#define METADATA_FILE "/tmp/document_metadata.txt"

int main(int argc, char *argv[]) {
  // Verifica se os argumentos necessários foram fornecidos
  if (argc != 3) {
    fprintf(stderr, "Uso: %s document_folder cache_size\n", argv[0]);
    return 1;
  }

  const char *document_folder = argv[1];
  int cache_size = atoi(argv[2]);

  if (cache_size <= 0) {
    perror("Erro: cache_size deve ser um número inteiro positivo.");
    return 1;
  }

  printf("Iniciando servidor com document_folder: %s e cache_size: %d\n", document_folder, cache_size);

  // Cria os FIFOs
  mkfifo(FIFO_CLIENT_TO_SERVER, 0666);
  mkfifo(FIFO_SERVER_TO_CLIENT, 0666);

  char command[BUFFER_SIZE];
  char response[BUFFER_SIZE];

  // Inicializa a indexação com os argumentos fornecidos
  indexation_init_with_params(document_folder, cache_size);

  load_metadata();

  printf("Servidor iniciado com sucesso 🚀\n");

  while (1) {
    // Abre o FIFO para ler comandos do cliente
    int fd_read = open(FIFO_CLIENT_TO_SERVER, O_RDONLY);
    if (fd_read < 0) {
      perror("Erro ao abrir o FIFO para leitura");
      _exit(1);
    }

    read(fd_read, command, BUFFER_SIZE);
    close(fd_read);

    printf("Comando recebido do cliente: %s\n", command);

    process_command(command, response);

    int fd_write = open(FIFO_SERVER_TO_CLIENT, O_WRONLY);
    if (fd_write < 0) {
      perror("Erro ao abrir o FIFO para escrita");
      _exit(1);
    }

    write(fd_write, response, strlen(response) + 1);
    close(fd_write);

    if (strcmp(command, "-f") == 0) {
      // Se o comando for -f, encerra o servidor
      printf("Servidor encerrado com sucesso 🚀\n");
      close(fd_read);
      unlink(FIFO_CLIENT_TO_SERVER);
      unlink(FIFO_SERVER_TO_CLIENT);
      break;
    }

    printf("Resposta enviada ao cliente: %s\n", response);
  }

  return 0;
}
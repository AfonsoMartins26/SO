# SO — Process Indexation Server (client/server)

Process indexation and simple search service implemented with a client-server
architecture. This project was developed for the Operating Systems course
(Sistemas Operativos). The server indexes documents from a given folder into an
in-memory cache (GHashTable), supports queries from a client and persists
metadata across runs.

Grade: 17 / 20 🚀⭐

## Project overview

- Server (`dserver`) — manages an index of documents. It receives commands from
  the client through a named pipe (FIFO), processes them (add/remove/list/search/count),
  and responds back through a second FIFO. The server keeps metadata in
  `/tmp/document_metadata.txt` and stores it on graceful shutdown.
- Client (`dclient`) — sends commands to the server (via FIFO) and prints the
  server response to stdout.

The implementation uses GLib's GHashTable to store index entries and forks
processes to run `grep` when searching or counting lines containing a keyword.

## Repository layout

- `include/` — public headers (`Indexation.h`, `utils.h`).
- `src/dserver/` — server sources (`dserver.c`, `Indexation.c`, `utils.c`).
- `src/dclient/` — client source (`dclient.c`).
- `Makefile`, build/install targets.

## Dependencies

- GNU Make (build-time)
- GCC (build-time)
- GLib 2.0 development headers (build-time): install `libglib2.0-dev` on Debian/Ubuntu

Example (Debian/Ubuntu):

```sh
sudo apt-get install build-essential libglib2.0-dev
```

## Building

Build the project with:

```sh
make
```

Clean build artifacts with:

```sh
make clean
```

Install (optional):

```sh
make install
```

## How it works — quick description

- The server creates two FIFOs under `/tmp`:
  - `/tmp/client_to_server_fifo` — client -> server commands
  - `/tmp/server_to_client_fifo` — server -> client responses
- The server keeps an in-memory index (a GHashTable) and assigns incremental
  integer keys to each added document. The cache_size provided at server start
  limits the maximum number of entries that can be stored.
- On shutdown (`-f` command) the server writes the metadata to
  `/tmp/document_metadata.txt`. On startup it attempts to read that file and
  restore prior entries.

## Running the server

Usage:

```sh
./dserver <document_folder> <cache_size>
```

Examples:

```sh
# Start server indexing files under /home/user/docs with cache size 100
./dserver /home/user/docs 100 &
# Run in foreground (no &), useful for debugging
./dserver /home/user/docs 100
```

Notes:

- `document_folder` is used as a prefix when storing file paths. When adding a
  document from the client you should provide the file path relative to
  `document_folder` (the server concatenates them).
- If the server crashes and FIFOs remain in `/tmp`, remove them manually:

```sh
rm -f /tmp/client_to_server_fifo /tmp/server_to_client_fifo
```

## Client usage (commands)

The client (`dclient`) forwards commands to the server and prints the server's
reply. Typical usage:

```sh
./dclient [command]
```

Supported commands (examples):

- Add an indexation (returns a numeric key on success):

  ```sh
  ./dclient -a "Title" "Author1;Author2" "2023" "/relative/path.txt"
  ```

  - Arguments: title (quoted), authors (semicolon-separated, quoted), year,
    filepath (relative to `document_folder`). Note the client and server use
    quotes for fields that contain spaces.

- Consult / retrieve an index by key:

  ```sh
  ./dclient -c 5
  ```

  - Returns title, authors, year and stored path.

- Delete an index by key:

  ```sh
  ./dclient -d 5
  ```

- Count lines in a document that contain a keyword (returns an integer):

  ```sh
  ./dclient -l 5 keyword
  ```

- List documents that contain a keyword (spawn up to N children to report keys):

  ```sh
  ./dclient -s keyword 4
  ```

  - Arguments: keyword, max number of child processes to use.

- Stop the server (graceful shutdown; saves metadata):

  ```sh
  ./dclient -f
  ```

## Persistence & metadata

- Metadata is saved to `/tmp/document_metadata.txt` on graceful shutdown
  (`-f`). On startup the server tries to load this file to restore state.
- If the server was not shut down cleanly the metadata file may be missing or
  incomplete. The server tolerates a missing metadata file.

## Developers / Authors

- Afonso Paulo Martins — A106931 — AfonsoMartins26
- Gonçalo José Vieira de Castro — A107337 — goncalo122016
- Luís Miguel Jerónimo Felício — A106913 — luisfelicio
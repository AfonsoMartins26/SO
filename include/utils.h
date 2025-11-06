#ifndef UTILS_H
#define UTILS_H

/**
 * @brief Splits a string of authors into an array of individual author names.
 *
 * @param authors A null-terminated string containing the list of authors
 *                separated by a delimiter.
 * @return A dynamically allocated array of strings, where each string is
 *         an individual author's name. The caller is responsible for
 *         freeing the memory allocated for the array and its elements.
 */
char **split_authors(const char *authors);

/**
 * @brief Processes a command received from the client.
 *
 * @param command The command received from the client.
 * @param response The response to be sent back to the client.
 */
void process_command(char *command, char *response);

/**
 * @brief Deletes all occurrences of a specified character from a string.
 *
 * @param s The string from which to delete the character.
 * @param ch The character to delete.
 * @return A pointer to the modified string.
 */
char *deleteChar(char *s, char ch);

#endif

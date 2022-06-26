#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "hashtable.h"

hashTable * ht;

// Paths to search for headers
char directories[10][100];
int n_directories;

// Return value of a key defined by a token. If it's found,
// return OK = 1;
char *return_value_of_key_token(char *token, int *OK)
{

	element *current2 = ht->array[hashCode(token)];
	char *value;

	while (current2 != NULL) {
		if (strcmp(current2->key, token) == 0) {
			value = malloc((strlen(current2->value) + 1)
				* sizeof(char));
			if (value == NULL)
				exit(12);
			strcpy(value, current2->value);
			*OK = 1;
			break;
		}
		current2 = current2->next;
	}
	return value;
}

int main(int argc, char **argv)
{
	hashTable *ht = createHashTable();

	FILE *outfile;
	FILE *fp1;

	char *input_file_name;
	char *output_file;
	int fd_out = 0;
	int fd1;
	int file_from_stdin = 1;

	char buffer[256];
	size_t size = 255;
	char *buffer2, *aux_buffer2;

	int printOK = 1;
	int skip = 0;
	int characters;
	int i;

	// Processing the arguments
	if (argc > 1) {
		char pair[2][20];
		char *p;

		for (i = 1; i < argc; i++) {
			// Processing 'define' arguments after -d option
			if (strstr(argv[i], "-D")) {
				char *arg1 = NULL;
				char *arg2 = NULL;

				arg1 = malloc((strlen(argv[i]) + 1) * sizeof(char));
				if (arg1 == NULL)
					exit(12);
				strcpy(arg1, argv[i]);

				// If arguments for -d options are not appended to -d
				if (argv[i][2] == '\0') {
					arg2 = malloc((strlen(argv[i + 1]) + 1) * sizeof(char));
					if (arg2 == NULL)
						exit(12);
					strcpy(arg2, argv[i+1]);
					// arg2 has the key=value pair
					p = strtok(arg2, "=\n");
					i++;
				} else
					// 'arg1 + 2' has the key=value pair
					p = strtok(arg1 + 2, "=\n");

				// In p, now, we have the key from 'key=value' token
				strcpy(pair[0], p);
				p = strtok(NULL, "=\n");

				// If p is null, it means it didn't have a value;
				if (p != NULL)
					strcpy(pair[1], p);
				else
					strcpy(pair[1], "");

				// Add the key-value to the hashtable;
				add(ht, pair[0], pair[1]);
				strcpy(pair[0], "");
				strcpy(pair[1], "");
				free(arg1);

				if (arg2 != NULL)
					free(arg2);

			} else if (strstr(argv[i], "-I")) {
				// if the argument for option -I is appended to it;
				if (*(argv[i] + 2) != '\0') {
					strcpy(directories[n_directories++], argv[i] + 2);
				} else {
					strcpy(directories[n_directories++], argv[i+1]);
					i++;
				}
			} else if (strstr(argv[i], "-o")) {
				// If the argument for option -o is appended to it;
				if (*(argv[i] + 2) != '\0') {
					output_file = malloc((strlen(argv[i]) - 1) * sizeof(char));
					if (output_file == NULL)
						exit(12);
					strcpy(output_file, argv[i] + 2);
					fd_out = open(output_file, O_RDWR | O_CREAT, 0644);
				} else {
					output_file = malloc((strlen
						(argv[i+1]) + 1) * sizeof(char));
					if (output_file == NULL)
						exit(12);
					strcpy(output_file, argv[i+1]);
					fd_out = open(output_file, O_RDWR | O_CREAT, 0644);
				}
			} else if (strstr(argv[i], "-"))
				// If there are any bad options (anything besides -D, -I, -o)
				return 1;
			else if ((!strstr(argv[i-1], "-D") || !strstr(argv[i-1], "-I") || !strstr(argv[i-1], "-o")) && file_from_stdin == 1) {
				// If this argument has no option before it, it means that it is an input file (for output file without -o,
				// see below)
				if (strstr(argv[i], ".")) {
					input_file_name = malloc((strlen(argv[i]) + 1) * sizeof(char));
					if (input_file_name == NULL)
						exit(12);
					strcpy(input_file_name, argv[i]);
					file_from_stdin = 0;
				}
			} else if (i == argc - 1 && file_from_stdin == 0) {
				// If the last argument is a file name and the input file was initialized, then it can be an output file
				output_file = malloc((strlen(argv[i]) + 1) * sizeof(char));
				if (output_file == NULL)
					exit(12);
				strcpy(output_file, argv[i]);
				fd_out = open(output_file, O_RDWR | O_CREAT, 0644);
				free(output_file);
			}
		}
	}

	// Open input_file if there is any
	if (file_from_stdin == 0) {
		fd1 = open(input_file_name, O_RDONLY);
		if (fd1 < 0)
			return 1;
		fp1 = fdopen(fd1, "r");
	}

	// Open output file
	if (fd_out != 0)
		outfile = fdopen(fd_out, "w");

	buffer2 = malloc(size * sizeof(char));
	if (buffer2 == NULL)
		exit(12);

	// Decide if the code to be read is from stdin, or from input_file
	if (file_from_stdin == 0)
		aux_buffer2 = fgets(buffer2, size, fp1);
	else
		aux_buffer2 = fgets(buffer2, size, stdin);

	// Run this while there is information to be read;
	while (aux_buffer2 != NULL) {
		// if it's an include clause;
		if (strstr(buffer2, "#include")) {
			FILE *header_file;
			// First token has "#include", so we ignore it
			char *token = strtok(buffer2, "\" ");
			char header_name[100];
			char buff[256];

			// This token has the header file name
			token = strtok(NULL, "\n\" ");

			// If the source code was read from stdin, it means the header file
			// is searched firstly in the current directory
			if (file_from_stdin == 1) {
				strcpy(header_name, token);
			} else {
				// Else, proccess the input_file_name to take the parent directory,
				// then search there for the header file
				char tokenize_input_filename[100];

				strcpy(tokenize_input_filename, input_file_name);
				while (strstr(tokenize_input_filename, "/")) {
					memcpy(tokenize_input_filename, strstr(tokenize_input_filename, "/") + 1,
						strlen(strstr(tokenize_input_filename, "/") + 1) + 1);
				}
				strcpy(header_name, input_file_name);
				strcpy(header_name + strlen(header_name) -
					strlen(strstr(header_name, tokenize_input_filename)), token);
			}

			header_file = fopen(header_name, "r");

			// If it couldn't be found neither in the current directory, nor the same
			// directory as the input_file, search in the paths given as arguments
			// to the -I option
			if (header_file == NULL) {
				char aux[100];

				for (i = 0; i < n_directories; i++) {
					strcpy(aux, directories[i]);
					strcat(aux, "/");
					strcat(aux, token);
					header_file = fopen(aux, "r");
					if (header_file != NULL)
						break;
				}
				// If it is not found anywhere, exit program;
				if (header_file == NULL)
					return 1;
			}

			while (fgets(buff, size, header_file) != NULL) {
				// Process the #define directives from the header file the same
				// way as we do with the input_file source code;
				if (strstr(buff, "#define") && *buff == '#') {

					char *key = NULL, *value = NULL;
					// Extract "#define" and throw it away
					char *token = strtok(buff, "\t []{}<>=+-*/%!&|^.,:;()");

					//Extract key_to_be token
					token = strtok(NULL, "\t []{}<>=+-*/%!&|^.,:;()");

					// Move token to key;
					key = malloc((strlen(token) + 1) * sizeof(char));
					if (key == NULL)
						exit(12);
					strcpy(key, token);

					// Extract value_to_be token;
					token = strtok(NULL, "\n");
					add(ht, key, token);
					free(key);
					free(value);
				} else {
					// Print content to the stdout, or output_file
					if (fd_out == 0)
						printf("%s", buff);
					else
						fprintf(outfile, "%s", buff);
				}
			}
			fclose(header_file);

		} else if (strstr(buffer2, "#define") && *buffer2 == '#') {

			char *key, *value;
			// Extract "#define" and throw it away
			char *token = strtok(buffer2,
				"\t []{}<>=+-*/%!&|^.,:;()");

			//Extract key_to_be token
			token = strtok(NULL,
				"\t []{}<>=+-*/%!&|^.,:;()");

			// Move token to key;
			key = malloc((strlen(token) + 1) * sizeof(char));
			if (key == NULL)
				exit(12);
			strcpy(key, token);

			// Extract value_to_be token;
			token = strtok(NULL, "\n");

			// If the value_to_be exists as a key in hashtable;
			if (ht->array[hashCode(token)] != NULL) {
				int ok = 0;
				element *current = ht->array[hashCode(token)];

				// Search for that specific key
				while (current != NULL) {
					if (strcmp(current->key, token) == 0) {
						value = malloc((strlen
							(current->value) + 1) * sizeof(char));
						if (value == NULL)
							exit(12);
						strcpy(value, current->value);
						ok = 1;
						break;
					}
					current = current->next;
				}

				/* If it is false alarm, add it in hashtable as it is;
				 */
				if (ok == 0) {
					// Process the value_to_be token as it may be an arithmetic expression
					// ex: #define ABC BCD + 4
					int OK = 0;
					element *current2;
					char aux_token1[25], aux_token2[25];

					strcpy(aux_token1, token);
					strcpy(aux_token2, token);

					token = strtok(aux_token1, " +");

					/* Search for first operand if it already exists in hashtable
					 */

					if (ht->array[hashCode(token)] != NULL) {

						value = return_value_of_key_token(token, &OK);
						// If the alarm was false, copy the value_to_be as value
						if (OK == 0)
							strcpy(value, token);
					} else {
						value = malloc((strlen(token) + 1) * sizeof(char));
						if (value == NULL)
							exit(12);
						strcpy(value, token);
					}

					/* If this isn't the end of line, it means
					 * there is a second operator, so we process the value
					 */
					if (*(aux_token2 + strlen(token)) != '\0') {
						token = aux_token2 + strlen(token) + 3;

						if (*token >= 34) {
							token = strtok(NULL, " +");
							strcat(value, " + ");
							strcat(value, token);
						}
					}
				}
			} else {
				// The value doesn't exist as a key,
				// so add it as it is
				int ok = 0;
				char aux_token1[25], aux_token2[25];
				element *current2;

				strcpy(aux_token1, token);
				strcpy(aux_token2, token);
				// Try to find if value is an expression.
				token = strtok(aux_token1, " +");

				// Search for first operand if it
				// already exists in hashtable

				if (ht->array[hashCode(token)] != NULL) {

					current2 = ht->array[hashCode(token)];

					while (current2 != NULL) {
						if (strcmp(current2->key, token) == 0) {
							value = malloc((strlen(aux_token2) + 1) * sizeof(char));
							if (value == NULL)
								exit(12);
							strcpy(value, current2->value);
							ok = 1;
							break;
						}
						current2 = current2->next;
					}
					/* If the alarm was false,
					 * copy the value_to_be as value
					 */
					if (ok == 0)
						strcpy(value, token);

				} else {
					value = malloc((strlen(aux_token2) + 1) * sizeof(char));
					if (value == NULL)
						exit(12);
					strcpy(value, token);
				}
				/* If this isn't the end of line, it means
				 * there is a second operator,
				 * so we process the value
				 */
				if (*(aux_token2 + strlen(token)) != '\0') {
					char *token2 = aux_token2 + strlen(token) + 3;

					if (*token2 >= 34) {
						token = strtok(NULL, " +");
						strcat(value, " + ");
						strcat(value, token);
					}
				}
			}
			add(ht, key, value);
			free(key);
			free(value);

		} else if (strstr(buffer2, "#undef")) {
			// delete the element from hashtable
			char *tk = strtok(buffer2, "\n ");

			tk = strtok(NULL, "\n ");
			delete(ht, tk);

		} else if (strstr(buffer2, "#ifndef")) {
			// Search for second token if it not defined. If not,
			// print the code until it meets #endif
			int define_exists = 0;
			int i = 0;
			char *tk = strtok(buffer2, "\n ");

			tk = strtok(NULL, "\n ");

			for (i = 0; i < HASHTABLE_SIZE; i++) {
				if (ht->array[i] != NULL) {
					element *current = ht->array[i];

					while (current != NULL) {
						if (strcmp(current->key, tk) == 0)
							define_exists = 1;
						current = current->next;
					}
					if (define_exists)
						break;
				}
			}
			// If it doesn't exist, allow printing the next lines.
			if (!define_exists)
				printOK = 1;
			else
				printOK = 0;
		}

		else if (strstr(buffer2, "#ifdef")) {
			// Same as #ifndef, but it prints if it is defined.
			int define_exists = 0;
			int i = 0;
			char *tk = strtok(buffer2, "\n ");

			tk = strtok(NULL, "\n ");

			for (i = 0; i < HASHTABLE_SIZE; i++) {
				if (ht->array[i] != NULL) {
					element *current = ht->array[i];

					while (current != NULL) {
						if (strcmp(current->key, tk) == 0)
							define_exists = 1;
						current = current->next;
					}
					if (define_exists)
						break;
				}
			}
			if (define_exists)
				printOK = 1;
			else
				printOK = 0;
		}

		else if ((strstr(buffer2, "#if") || strstr(buffer2, "#elif")) && *buffer2 == '#')  {
			// This token has "#if" or "#elif";
			char *tk = strtok(buffer2, "\n ");
			char if_or_elif[7];
			int isNumber = 1;
			int i = 0;

			strcpy(if_or_elif, tk);

			if (skip == 1) {
				printOK = 0;
				continue;
			}

			tk = strtok(NULL, "\n ");

			// Check if the condition-token is a number or not
			for (i = 0; i < strlen(tk); i++) {
				if (tk[i] < '0' || tk[i] > '9') {
					isNumber = 0;
					break;
				}
			}

			if (isNumber == 1) {
				int cond = atoi(tk);

				// If it's equal to 0, don't print next lines
				if (cond == 0)
					printOK = 0;
				else {
					// If the condition is not 0 and is on #elif branch,
					// print the bellow lines;
					if (strcmp(if_or_elif, "#elif") == 0)
						printOK = 1;
					// This skip variable helps us if "#if" and "#elif"
					// have non-zero conditions. The first #if or #elif that
					// has non-zero conditions will make skip = 1, meaning
					// that it will ignore everything until #endif
					skip = 1;
				}
			} else {
				// Search the condition in hashtable
				for (i = 0; i < HASHTABLE_SIZE; i++) {
					if (ht->array[i] != NULL) {
						element *current = ht->array[i];

						while (current != NULL) {
							if (strcmp(current->key, tk) == 0) {
								if (atoi(current->value) != 0) {
									if (strcmp(if_or_elif, "#elif") == 0)
										printOK = 1;
									skip = 1;
								} else
									printOK = 0;
							}
							current = current->next;
						}
					}
				}
			}
		} else if (strstr(buffer2, "#else"))  {
			// If skip = 0, it means that no #if or #elif had non-zero
			// conditions, so change the printOK variable the other way;
			if (skip == 0) {
				if (printOK == 1)
					printOK = 0;
				else
					printOK = 1;
			} else
				//If skip = 1, ignore this #else
				printOK = 0;
		} else if (strstr(buffer2, "#endif")) {
			// print every line below this and reset skip variable;
			printOK = 1;
			skip = 0;
		} else if (*buffer2 != '\n' && printOK == 1) {
			char final[256];

			// Copy all the line in the final variable
			strcpy(final, buffer2);
			// Check if there is any in hashtable that appears at least one time in the
			// final string;
			for (i = 0; i < HASHTABLE_SIZE; i++) {
				if (ht->array[i] != NULL) {
					element *current = ht->array[i];

					while (current != NULL) {
						while (strstr(buffer2, current->key)) {
							// If the key appears in the string, but between quotes, ignore it
							// and continue searching after the final quotation mark;
							if (strstr(buffer2, "\"")) {
								memcpy(buffer2, strstr(strstr(buffer2, "\"") + 1, "\"") + 1,
									strlen(strstr(strstr(buffer2, "\"") + 1, "\"") + 1) + 1);
								continue;
							}
							// Replace the key with the value;
							strcpy(final + strlen(final) -
								strlen(strstr(buffer2, current->key)), current->value);

							// Append the rest of the string after the first occurrence of the key;
							strcat(final, buffer2 + strlen(buffer2) -
								strlen(strstr(buffer2, current->key)) + strlen(current->key));

							// Keep the remaining string after the first occurrence and repeat the above steps
							// on this string;
							memcpy(buffer2, strstr(buffer2, current->key) +
								strlen(current->key), strlen(strstr(buffer2, current->key) +
									strlen(current->key)) + 1);
						}
						current = current->next;
					}
				}
			}
			if (fd_out == 0)
				printf("%s", final);
			else
				fprintf(outfile, "%s", final);
		}
		// Read the next line
		if (file_from_stdin == 0)
			aux_buffer2 = fgets(buffer2, size, fp1);
		else
			aux_buffer2 = fgets(buffer2, size, stdin);
	}

	if (file_from_stdin == 0) {
		free(input_file_name);
		fclose(fp1);
	}
	if (fd_out != 0)
		fclose(outfile);

	free(buffer2);

	// Free the hashtable
	for (i = 0; i < HASHTABLE_SIZE; i++) {
		element *tmp;

		while (ht->array[i] != NULL) {
			tmp = ht->array[i];
			ht->array[i] = ht->array[i]->next;
			free(tmp->key);
			free(tmp->value);
			free(tmp);
		}
	}
	free(ht->array);
	free(ht);
	return 0;
}

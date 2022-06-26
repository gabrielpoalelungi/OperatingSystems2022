#include "hashtable.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int hashCode(char *key)
{
	unsigned int index = 0;
	int i;

	for (i = 0; i < strlen(key) - 1; i++)
		index += key[i]*key[i];

	index = index % HASHTABLE_SIZE;

	return index;
}

hashTable *createHashTable(void)
{
	int i;
	hashTable *hashtable;

	hashtable = malloc(sizeof(hashTable));
	hashtable->array = malloc(HASHTABLE_SIZE * sizeof(element *));

	if (hashtable == NULL || hashtable->array == NULL)
		exit(12);

	/* I put this because Valgrind yells
	 * "Uninitialised value was created by a heap allocation"
	 */
	for (i = 0; i < HASHTABLE_SIZE; i++)
		hashtable->array[i] = NULL;

	return hashtable;
}

element *createElement(char *key, char *value)
{
	element *e;

	e = malloc(sizeof(element));
	e->key = malloc((strlen(key) + 1) * sizeof(char));
	e->value = malloc((strlen(value) + 1) * sizeof(char));

	if (e == NULL || e->key == NULL || e->value == NULL) {
		exit(12);
	}
	/* I put this because Valgrind yells
	 * "Uninitialised value was created by a heap allocation"
	 */
	e->next = NULL;

	strcpy(e->key, key);
	strcpy(e->value, value);

	return e;
}

void add(hashTable *hashtable, char *key, char *value)
{
	int index = hashCode(key);
	element *prev, *current;

	// If there is not element with the hashcode of this key,
	// add it as it is
	if (hashtable->array[index] == NULL) {
		hashtable->array[index] = createElement(key, value);
		return;
	}

	current = hashtable->array[index];

	// Iterate through the linked list until the end
	while (current != NULL) {
		prev = current;
		current = current->next;
	}

	// Add the pair at the end of the linked list
	prev->next = createElement(key, value);
}

void delete(hashTable *hashtable, char *key)
{
	int index = hashCode(key);

	if (hashtable->array[index] != NULL) {
		element *current = hashtable->array[index];
		element *prev = NULL;

		while (current != NULL) {
			if (strcmp(current->key, key) == 0) {
				// If the element to be deleted is the
				// first in the linked list
				if (current == hashtable->array[index])
					hashtable->array[index] = current->next;
				else if (current->next != NULL)
					// If the element to be deleted is in the middle of
					// the linked list;
					prev->next = current->next;
				else
					// If the element to be deleted is the last one;
					prev->next = NULL;

				free(current->key);
				free(current->value);
				free(current);
				break;
			}
			prev = current;
			current = current->next;
		}
	}
}

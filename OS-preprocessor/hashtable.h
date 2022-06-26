#ifndef hashtable_h
#define hashtable_h

#define HASHTABLE_SIZE 2000

typedef struct element {

	char *key;
	char *value;
	struct element *next;

} element;

typedef struct hashTable {

	element **array;

} hashTable;

int hashCode(char *key);
hashTable *createHashTable();
element *createElement(char *key, char *value);
void add(hashTable *hashtable, char *key, char *value);
void delete(hashTable *hashtable, char *key);

#endif

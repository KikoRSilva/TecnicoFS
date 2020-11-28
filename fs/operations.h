#ifndef FS_H
#define FS_H
#include "state.h"

#define CREATE 1
#define DELETE 2
#define LOOKUP 3

typedef struct ArrayLock {
    int contador;
    int locks[INODE_TABLE_SIZE];
} ArrayLocks;

struct timespec begin, end;

void rwlock_read(int i);
void rwlock_write(int i);
void init_fs();
void destroy_fs();
int is_dir_empty(DirEntry *dirEntries);
int create(char *name, type nodeType);
int delete(char *name);
int move(char *name, char *new_name);
int search(char *name, int function_type);
int lookup(char *name, int function_type, ArrayLocks *arr);
void unlocknodes(ArrayLocks *arr);
int print_tecnicofs_tree(char *outputFile);

#endif /* FS_H */

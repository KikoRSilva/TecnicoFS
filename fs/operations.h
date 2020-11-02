#ifndef FS_H
#define FS_H
#include "state.h"

#define CREATE 1
#define DELETE 2
#define LOOKUP 3
#define MOVE 4

void rwlock_read(pthread_rwlock_t lock);
void rwlock_write(pthread_rwlock_t lock);
void init_fs();
void destroy_fs();
int is_dir_empty(DirEntry *dirEntries);
int create(char *name, type nodeType);
int delete(char *name);
int lookup(char *name, int function_type);
void unlocknodes(int arrlocks[]);
void print_tecnicofs_tree(FILE *fp);
int arrlocks[INODE_TABLE_SIZE];

#endif /* FS_H */

/**@file
 * This library is designed to map open file descriptors to
 * data.
 * We exploit the sequential nature of open file descriptors
 * to efficiently map them in a hash table.
 * @author Craig Hesling <craig@hesling.com>
 * @date Febuary 4, 2016
 */

#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
/* fd_set data type */
#include <sys/select.h>
#include <err.h>

#include "fdmap_list.h"
#include "fdmap.h"

/**
 * Structure for holding a list of open file descriptors
 *
 * We want lookups to be fast. Insertions and deletions
 * being fast is less important.
 */
typedef struct fdmap {
	/** file descriptors in the table */
	int items;
	/** size of the table */
	int size;
	/** collisions */
//	int colls;
	/** the actual hash table of table_node pointers */
	fdmap_list_t *table;
	/** mark which file descriptors are in the hash table */
	fd_set fs;
} *fdmap_t;

int compareints (const void * a, const void * b)
{
	return ( *(int*)a - *(int*)b );
}

static int
fdmap_is_valid(fdmap_t map) {
	if(map == NULL) return 0;
	if(map->table == NULL) return 0;
	return 1;
}

static int
fdmap_hash(fdmap_t map, int fd) {
	return fd%map->size;
}


/**
 * Allocates a new fdmap with a hash table of initial_size.
 *
 * @param initial_size The requested initial size for the hash table
 * @return A new fdmap or NULL. If NULL is returned, errno is set.
 */
fdmap_t
fdmap_new(int initial_size) {
	fdmap_t map;
	int index;
	map = (fdmap_t) malloc(sizeof(struct fdmap));
	if(!map) {
		return NULL;
	}
	map->items = 0;
	map->size = initial_size>0?initial_size:FDMAP_DEFAULT_SIZE;
//	map->colls = 0;
	map->table = (fdmap_list_t *) calloc(map->size, sizeof(fdmap_list_t));
	if(!map->table) {
		free(map);
		return NULL;
	}
	for(index = 0; index < map->size; index++) {
		map->table[index] = fdmap_list_new(FDMAP_LIST_TYPE_ORDERED);
		if(!map->table[index]) err(EXIT_FAILURE, 0);
	}
	return map;
}

void
fdmap_free(fdmap_t map) {
	int index;
	assert(map);
	assert(fdmap_is_valid(map));
	for(index = 0; index < map->size; index++) {
		fdmap_list_free(map->table[index]);
	}
	free(map->table);
	free(map);
}

//void fdmap_resize(fdmap_t map, int newsize) {
//	assert(map);
//	map->table = realloc(map->table, newsize);
//	if(map->table == NULL) {
//		perror("could not resize the file descriptor list");
//	}
//	map->size = newsize;
//}

void
fdmap_add(fdmap_t map, int fd, data_t data) {
	fdmap_list_t list;
	assert(map);
	assert(fd >= 0);

	list = map->table[fdmap_hash(map, fd)];
	fdmap_list_add(list, fd, data);
	map->items++;
}

void
fdmap_rm(fdmap_t map, int fd) {
	fdmap_list_t list;
	assert(map);
	assert(fd >= 0);

	list = map->table[fdmap_hash(map, fd)];
	if(fdmap_list_rm(list, fd)) {
		map->items--;
	}
}

/**
 *
 * @param map
 * @param fd
 * @return 0 if not found, 1 if found
 */
int
fdmap_find(fdmap_t map, int fd, data_t *data) {
	fdmap_list_t list;
	assert(map);
	assert(fd >= 0);

	list = map->table[fdmap_hash(map, fd)];
	return fdmap_list_find(list, fd, data);
}

//int
//fdmap_isin(fdmap_t *list, int fd) {
//	int *item;
//	assert(list);
//	assert(fd >= 0);
//
//	///@todo verify bsearch can search on an array wit 0 items
//
//	item = (int *)bsearch(&fd, list->table, list->items, sizeof(int), compareints);
//	return item != NULL;
//}

/**@file
 * @author Craig Hesling <craig@hesling.com>
 * @date Feb 4, 2016
 */

#ifndef FDMAP_FDMAP_LIST_H_
#define FDMAP_FDMAP_LIST_H_

#include <stddef.h> // size_t

typedef enum {
	FDMAP_LIST_TYPE_FIFO,
	FDMAP_LIST_TYPE_ORDERED,
} fdmap_list_type_t;

/**
 * Sets the data value to map to the fd key.
 */
typedef void *data_t;
typedef struct fdmap_list *fdmap_list_t;

fdmap_list_t
fdmap_list_new(fdmap_list_type_t type);
void
fdmap_list_free(fdmap_list_t list);
size_t
fdmap_list_size(fdmap_list_t list);
int
fdmap_list_find(fdmap_list_t list, int fd, data_t *data);
void
fdmap_list_add(fdmap_list_t list, int fd, data_t data);
int
fdmap_list_rm(fdmap_list_t list, int fd);
int
fdmap_list_pop(fdmap_list_t list, int *fd, data_t *data);
int
fdmap_list_peek(fdmap_list_t list, int *fd, data_t *data);

#endif /* FDMAP_FDMAP_LIST_H_ */

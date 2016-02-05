/**@file
 * @author Craig Hesling <craig@hesling.com>
 * @date Febuary 4, 2016
 */

#ifndef FDMAP_FDMAP_H_
#define FDMAP_FDMAP_H_

#include "fdmap_list.h" // data_t

#define FDMAP_DEFAULT_SIZE 10
//#define FDMAP_SLOW_DOWN_SIZE 512

typedef struct fdmap *fdmap_t;

fdmap_t
fdmap_new(int initial_size);
void
fdmap_free(fdmap_t map);
void
fdmap_add(fdmap_t map, int fd, data_t data);
void
fdmap_rm(fdmap_t map, int fd);
int
fdmap_find(fdmap_t map, int fd, data_t *data);

#endif /* FDMAP_FDMAP_H_ */

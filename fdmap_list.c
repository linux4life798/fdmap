/**@file
 * @author Craig Hesling <craig@hesling.com>
 * @date Feb 4, 2016
 */

#include "fdmap_list.h"

#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <err.h>


typedef enum {
	BEFORE,
	AFTER
} preposition_t;


typedef struct fdmap_node *fdmap_node_t;
struct fdmap_node {
	int           fd;
	data_t        data;
	fdmap_node_t  prev;
	fdmap_node_t  next;
};

/**
 * When used as a FIFO, we pop from head and push to tail.
 */
struct fdmap_list {
	fdmap_list_type_t type;
	size_t       size;
	fdmap_node_t head;
	fdmap_node_t tail;
};
typedef struct fdmap_list *fdmap_list_t;


/*---------------------------------------------------------*
 *                  Private Node Helpers                   *
 *---------------------------------------------------------*/

static fdmap_node_t
fdmap_node_new(int fd, data_t data, fdmap_node_t prev, fdmap_node_t next) {
	fdmap_node_t node = (fdmap_node_t)malloc(sizeof(struct fdmap_node));
	if(!node) return NULL;
	node->fd   = fd;
	node->data = data;
	node->prev = prev;
	node->next = next;
	return node;
}

static void
fdmap_node_free(fdmap_node_t node) {
	assert(node);
	free(node);
}

/** @brief Free all nodes forward from given node */
static void
fdmap_node_free_fwd(fdmap_node_t node) {
	assert(node);
	if(node->next) fdmap_node_free_fwd(node->next);
	fdmap_node_free(node);
}


/*---------------------------------------------------------*
 *                  Private Methods                        *
 *---------------------------------------------------------*/

static void
fdmap_list_link(fdmap_list_t list, fdmap_node_t node, preposition_t placement, fdmap_node_t refnode) {
	assert(list);
	assert(node);
	assert((placement==BEFORE)||(placement==AFTER));

	if(refnode == NULL) {

		/* insert the node in the only (single) spot possible */
		if(list->head == NULL) {
			assert(list->head == NULL);
			assert(list->tail == NULL);
			assert(list->size == 0);

			node->next = NULL;
			node->prev = NULL;
			list->head = node;
			list->tail = node;
			list->size++;
			return;
		}

		/* they were just lazy */
		refnode = (placement==BEFORE) ? list->head : list->tail;
	}

	// remaining operations count on refnode
	assert(refnode);

	/* insert before the refnode */
	if(placement == BEFORE) {
		/* set my pointers */
		node->next = refnode;
		node->prev = refnode->prev;

		if(refnode->prev) {
			/* have prev node point to me */
			assert(list->head != refnode);
			refnode->prev->next = node;
		} else {
			/* fix head to point to me */
			assert(list->head == refnode);
			list->head = node;
		}

		/* make refnode point back to me */
		refnode->prev = node;

		list->size++;
		return;
	}

	/* insert after the refnode */
	if(placement == AFTER) {
		/* set my pointers */
		node->next = refnode->next;
		node->prev = refnode;

		if(refnode->next) {
			/* have next node point back to me */
			assert(list->tail != refnode);
			refnode->next->prev = node;
		} else {
			/* fix tail to point to me */
			assert(list->tail == refnode);
			list->tail = node;
		}

		/* make refnode point to me */
		refnode->next = node;

		list->size++;
		return;
	}

}

static void
fdmap_list_unlink(fdmap_list_t list, fdmap_node_t node) {
	assert(list);
	assert(node);

	if(node->prev) {
		assert(list->head != node);
		assert(list->size > 0);
		node->prev->next = node->next;
	} else {
		assert(list->head == node);
		list->head = node->next;
	}

	if(node->next) {
		assert(list->tail != node);
		assert(list->size > 0);
		node->next->prev = node->prev;
	} else {
		assert(list->tail == node);
		list->tail = node->prev;
	}

	list->size--;
}

static fdmap_node_t
fdmap_list_fd_find(fdmap_list_t list, int fd) {
	fdmap_node_t rover; // name stolen from fragglet@GitHub
	switch(list->type) {
	case FDMAP_LIST_TYPE_FIFO:
		for(rover = list->head; rover; rover = rover->next) {
			if(rover->fd == fd) {
				return rover;
			}
		}
		return NULL;
	case FDMAP_LIST_TYPE_ORDERED:
		for(rover = list->head; rover; rover = rover->next) {
			if(rover->fd == fd) {
				return rover;
			} else if(rover->fd > fd) {
				return NULL;
			}
		}
		return NULL;
	default:
		assert(0);
	}
}

/*---------------------------------------------------------*
 *                  Public Methods                         *
 *---------------------------------------------------------*/

/**
 * Create a new list of fdmaps key/values.
 * @param type The type of list to create. Either a fifo or an ordered.
 * @return
 */
fdmap_list_t
fdmap_list_new(fdmap_list_type_t type) {
	fdmap_list_t list = (fdmap_list_t) malloc(sizeof(struct fdmap_list));
	if(!list) return NULL;
	if((type!=FDMAP_LIST_TYPE_FIFO) & (type!=FDMAP_LIST_TYPE_ORDERED)) {
		free(list);
		return NULL;
	}
	list->head = NULL;
	list->tail = NULL;
	list->type = type;
	list->size = 0;
	return list;
}

void
fdmap_list_free(fdmap_list_t list) {
	assert(list);
	if(list->head) {
		assert(list->tail);
		assert(list->size > 0);
		fdmap_node_free_fwd(list->head);
	}
	free(list);
}

size_t
fdmap_list_size(fdmap_list_t list) {
	assert(list);
	return list->size;
}

/**
 * @brief Find the data associated with the given fd in the list
 */
int
fdmap_list_find(fdmap_list_t list, int fd, data_t *data) {
	fdmap_node_t node;
	assert(list);
	assert(data);

	switch(list->type) {
	case FDMAP_LIST_TYPE_FIFO:
		for(node = list->head; node; node = node->next) {
			if(node->fd == fd) {
				*data = node->data;
				return 1;
			}
		}
		break;
	case FDMAP_LIST_TYPE_ORDERED:
		for(node = list->head; node; node = node->next) {
			if(node->fd == fd) {
				*data = node->data;
				return 1;
			} else if(node->fd > fd) {
				return 0;
			}
		}
		break;
	}
	return 0;
}

/**
 * @brief Add the mapping for fd
 * @note This method allows inserting duplicate mappings
 * @param list The target list
 * @param fd The file descriptor to remove
 */
void
fdmap_list_add(fdmap_list_t list, int fd, data_t data) {
	fdmap_node_t node;
	assert(list);

	node = fdmap_node_new(fd, data, NULL, NULL);
	if(!node) err(EXIT_FAILURE, 0);

	switch(list->type) {
	case FDMAP_LIST_TYPE_FIFO:
		/* add to tail */
		fdmap_list_link(list, node, AFTER, list->tail);
		break;
	case FDMAP_LIST_TYPE_ORDERED: {
		/* find where to insert */
		fdmap_node_t runner;
		// run to the first element that is larger than(or equal to) fd
		for(runner = list->head; runner && (runner->fd < fd); runner = node->next) ;
		///@note This will technically insert before another node with the same fd
		if(runner) {
			// found an element to place before
			fdmap_list_link(list, node, BEFORE, runner);
		} else {
			// either no elements OR insert at the end
			fdmap_list_link(list, node, AFTER, runner);
		}
		break;
	}
	}
}

/**
 * @brief Remove the mapping for fd
 * @param list The target list
 * @param fd The file descriptor to remove
 * @return 1 upon successful removal, 0 if the fd couldn't be found
 */
int
fdmap_list_rm(fdmap_list_t list, int fd) {
	fdmap_node_t node;
	assert(list);

	node = fdmap_list_fd_find(list, fd);
	if(node) {
		fdmap_list_unlink(list, node);
		fdmap_node_free(node);
		return 1;
	}
	return 0;
}

/**
 * @brief Fetches the last inserted mapping
 *
 * When configured as a FIFO, this function fetches the last (fd,data) pair
 * and places the info in the locations specified.
 * @param list The reference fdmap list.
 * @param fd The locations to place the last mapping fd in.
 * @param data The location to place the last mapping data value in.
 * @return 1 when successful, 0 when the fd couldn't be found.
 */
int
fdmap_list_pop(fdmap_list_t list, int *fd, data_t *data) {
	assert(list);

	// if there is an element to pop
	if(list->head) {
		fdmap_node_t node = list->head;
		fdmap_list_unlink(list, node);
		if(fd)   *fd   = node->fd;
		if(data) *data = node->data;
		fdmap_node_free(node);
		return 1;
	}
	return 0;
}

/**
 * @brief Fetches the last inserted mapping without removing
 *
 * When configured as a FIFO, this function fetches the last (fd,data) pair
 * and places the info in the locations specified.
 * @param list The reference fdmap list.
 * @param fd The locations to place the last mapping fd in.
 * @param data The location to place the last mapping data value in.
 * @return 1 when successful, 0 when the fd couldn't be found.
 */
int
fdmap_list_peek(fdmap_list_t list, int *fd, data_t *data) {
	assert(list);

	// if there is an element to pop
	if(list->head) {
		fdmap_node_t node = list->head;
		if(fd)   *fd   = node->fd;
		if(data) *data = node->data;
		return 1;
	}
	return 0;
}

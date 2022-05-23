#include <stdatomic.h>
#include <stdlib.h>
typedef struct _hp_node hp_node_t;

typedef struct _hp_node_rt hp_node_rt_t;

typedef struct _hp_protect hp_protect_t;

typedef struct _hp hp_t;

hp_protect_t *hp_protect_init();

hp_t *hp_init(hp_protect_t *protect, void (*dealloc)(void *));

void *hp_protect_load(hp_protect_t *hp, void *ptr);

void hp_protect_release(hp_protect_t *pr, void *ptr);

void hp_retired(hp_t *hp, void *ptr);

int hp_scan(hp_t *hp);

int hp_pr_size(hp_protect_t *pr);

int hp_pr_list_size(hp_protect_t *pr);

void hp_pr_destroy(hp_protect_t *pr);

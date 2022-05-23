#include <stdatomic.h>
#include <stdlib.h>

typedef void *const *hp_addr_t;

typedef struct _hp_node hp_node_t;

typedef struct _hp_pr hp_pr_t;

typedef struct _hp hp_t;

hp_pr_t *hp_pr_init();

hp_t *hp_init(hp_pr_t *pr, void (*dealloc)(void *));

hp_addr_t hp_pr_load(hp_pr_t *hp, void *ptr);

void hp_pr_release(hp_pr_t *pr, hp_addr_t ptr_addr);

void hp_retired(hp_t *hp, void *ptr);

int hp_scan(hp_t *hp);

int hp_pr_size(hp_pr_t *pr);

int hp_pr_list_size(hp_pr_t *pr);

void hp_pr_destroy(hp_pr_t *pr);

#include "hp.h"
struct _hp_node {
    void *ptr;
    struct _hp_node *next;
    int used;
};

struct _hp_node_rt {
    void *ptr;
    struct _hp_node_rt *next;
};

struct _hp_protect {
    hp_node_t *head;
    int size;
    int list_size;
};

struct _hp {
    hp_protect_t *protect;  // The protect list is shared by all threads.

    hp_node_rt_t *retired;  // The retired thread is one each thread.
    int retired_size;       // Therefore, each thread has distinct hp_t,
                            // which has the same protect list but different
                            // retired list.

    void (*dealloc)(void *);
};

static void *protect_add(hp_protect_t *pr, void *ptr);

hp_protect_t *hp_protect_init()
{
    hp_protect_t *tmp = malloc(sizeof(*tmp));
    tmp->head = NULL;
    tmp->list_size = 0;
    tmp->size = 0;
    return tmp;
}

hp_t *hp_init(hp_protect_t *protect, void (*dealloc)(void *))
{
    hp_t *tmp = malloc(sizeof(*tmp));
    tmp->protect = protect;
    tmp->retired = NULL;
    tmp->retired_size = 0;
    tmp->dealloc = dealloc;
    return tmp;
}

void *hp_protect_load(hp_protect_t *pr, void *ptr)
{
    void **pptr = (void **) ptr;
    hp_node_t *cur = NULL;
    cur = protect_add(pr, ptr);
    return cur;
}

static void *protect_add(hp_protect_t *pr, void *ptr)
{
    /* Room of improvement:
        Return the entire node to caller, so the caller can
        used protect_release on the node, which obviates the
        use of cur->used.

        However, to abstract the implementation of hazard pointer,
        the struct needs to be redesigned.


    */
    void **pptr = (void **) ptr;

    if (atomic_load(pptr) == NULL)
        return NULL;
    hp_node_t *cur;
    for (cur = atomic_load(&pr->head); cur; cur = cur->next) {
        // Room of improvement:CAS cur->ptr by some value
        // to occupy
        if (!atomic_load(&cur->ptr)) {
            int exp_used = 0;
            // acquire the node
            if (!atomic_compare_exchange_strong(&cur->used, &exp_used, 1))
                continue;

            if (atomic_load(&cur->ptr)) {
                atomic_store(&cur->used, 0);
                continue;
            }
            do {
                void *val = atomic_load(pptr);
                // success
                if (!val)
                    return NULL;
                atomic_store(&cur->ptr, val);
                if (val == atomic_load(pptr)) {
                    atomic_store(&cur->used, 0);
                    atomic_fetch_add(&pr->size, 1);
                    // In case some thread change the cur->ptr
                    // between releasing node and returning value.
                    return val;
                }
                // *pptr is changed by others
            } while (1);
        }
        // CAS success, but *pptr is changed by others and back(ABA problem)
        // will this happens?
    }

    cur = malloc(sizeof(*cur));
    atomic_store(&cur->used, 1);
    atomic_store(&cur->ptr, NULL);
    while (1) {
        cur->next = atomic_load(&pr->head);
        if (atomic_compare_exchange_strong(&pr->head, &cur->next,
                                           cur))  // need strong
            break;
    }
    while (1) {
        void *val = atomic_load(pptr);
        atomic_store(&cur->ptr, val);

        if (val == atomic_load(pptr)) {
            atomic_store(&cur->used, 0);
            atomic_fetch_add(&pr->list_size, 1);
            atomic_fetch_add(&pr->size, 1);
            return val;
        }
    }
}

void hp_protect_release(hp_protect_t *pr, void *ptr)
{
    if (!ptr)
        return;
    while (1) {
        for (hp_node_t *cur = atomic_load(&pr->head); cur; cur = cur->next) {
            if (atomic_load(&cur->ptr) != ptr) {
                continue;
            }
            int exp_used = 0;
            if (!atomic_compare_exchange_strong(&cur->used, &exp_used, 1)) {
                continue;
            }
            void *exp = ptr;
            if (atomic_compare_exchange_strong(&cur->ptr, &exp, NULL)) {
                atomic_store(&cur->used, 0);
                atomic_fetch_sub(&pr->size, 1);
                return;
            }
            atomic_store(&cur->used, 0);
        }
        // When two reader check the same node and find its cur->ptr is NULL,
        // they use CAS(cur->used,0,1) on the node.
        // The first acquires the cur->used and change the ptr.
        // After the first releases the cur->used, the second uses
        // CAS(cur->used,0,1) just on time, therefore the first node can't
        // use protect_release to release the node, because the cur->used is
        // acquired by second reader. After the second reader finds that
        // this node's cur->ptr is being used, it will release the cur->used
        // and search for another one. At this time, the first reader can
        // safely release this hazard pointer.
    }
}

void hp_retired(hp_t *hp, void *ptr)
{
    hp_node_rt_t *tmp = malloc(sizeof(*tmp));
    tmp->next = hp->retired;
    hp->retired = tmp;
    tmp->ptr = ptr;

    int threshold = atomic_load(&hp->protect->size);
    threshold += (unsigned) threshold >> 2;
    if (hp->retired_size < threshold)
        return;
    hp_scan(hp);
}

int hp_scan(hp_t *hp)
{
    int count = 0;
    for (hp_node_rt_t **cur_rt = &hp->retired; cur_rt && *cur_rt;) {
        int safe_free = 1;
        for (hp_node_t *cur = atomic_load(&hp->protect->head); cur;
             cur = cur->next) {
            if (atomic_load(&cur->ptr) == (*cur_rt)->ptr) {
                safe_free = 0;
                count++;
                break;
            }
        }

        if (safe_free) {
            hp->dealloc((*cur_rt)->ptr);
            hp_node_rt_t *tmp = *cur_rt;
            *cur_rt = (*cur_rt)->next;
            free(tmp);
        } else
            cur_rt = &(*cur_rt)->next;
    }
    return count;
}

int hp_pr_size(hp_protect_t *pr)
{
    return pr->size;
}

int hp_pr_list_size(hp_protect_t *pr)
{
    return pr->list_size;
}

void hp_pr_destroy(hp_protect_t *pr)
{
    hp_node_t *tmp;
    int count = 0;
    while (pr->head) {
        tmp = pr->head;
        pr->head = tmp->next;
        free(tmp);
    }
    free(pr);
}
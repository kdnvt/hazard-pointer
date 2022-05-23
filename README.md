# hazard-pointer

## Tutorial

#### `hp_pr_t *hp_pr_init();`

Create the protect list, which stores the hazard pointers. Every thread using the same hazard pointers should share the same hp_pr_t.

#### `hp_t *hp_init(hp_pr_t *pr, void (*dealloc)(void *));`

Create the hp_t, which comprises an existing protect list(hazard pointers) and an empty retired list. The retired list is local to each writer thread, therefore all writer threads should call hp_init. The caller can define the deallocating function.

#### `hp_addr_t hp_pr_load(hp_pr_t *pr, void *ptr);`

Access the pointer and store it in protect list(hazard pointers). The ptr must be the address of the variable you want to load.

The return value hp_addr_t is an address of the hazard pointer, by dereferencing to access the value.

#### `void hp_pr_release(hp_pr_t *pr, hp_addr_t ptr_addr);`

Release the hazard pointer. Pass the hp_addr_t returned by hp_pr_load.

**Example:**
```c
    hp_addr_t ptr_addr = hp_pr_load(pr, &data);
    int *ptr = *ptr_addr;
    // do something
    printf("%d\n", *ptr);
    hp_pr_release(pr, ptr_addr);
```

#### `void hp_retired(hp_t *hp, void *ptr);`

Add the ptr to the retired list and try to deallocate it.

#### `int hp_scan(hp_t *hp);`

Try to release the pointers in the retired list. The return value is the number of pointers that can't be deallocated. Before the writer thread returns, it should use this function to ensure that there aren't remaining pointers in the retired list.

**Example:**
```c
void *writer(void *arg)
{
    void **argv = (void **) arg;
    hp_pr_t *pr = argv[0];
    hp_t *hp = hp_init(pr, free);
    
    /* do something */
    /* retire something */
    
    while (hp_scan(hp))
        ;
    free(hp);
}
```

#### `int hp_pr_size(hp_pr_t *pr);`

Return the number of hazard pointers.

#### `void hp_pr_destroy(hp_pr_t *pr);`

Deallocate the protect list in pr and hp_pr_t itself.
 

#ifndef RB_TREE_H_
#define RB_TREE_H_

#include <stddef.h>

typedef struct RBTree *RBTree;

RBTree rbtree_new__(size_t);
RBTree rbtree_new_ex__(size_t, void *alloc_ctx,
        void *(*alloc)(void *alloc_ctx, size_t),
        void (*dealloc)(void *alloc_ctx, void *, size_t));
#define rbtree_new(x) rbtree_new__(sizeof(x))
#define rbtree_new_ex(x, actx, a, d) rbtree_new_ex__(sizeof(x), actx, a, d)

void rbtree_set_compar(RBTree, int (*)(void*, void*));
void *rbtree_insert(RBTree, void *);
int rbtree_delete(RBTree, void *);
void *rbtree_get(RBTree, void *);
void rbtree_destroy(RBTree);
void rbtree_traverse(RBTree, void *data, void (*traverser)(void *, void *data));

#endif

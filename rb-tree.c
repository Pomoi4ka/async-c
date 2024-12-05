#include "rb-tree.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "da.h"

enum { RED, BLACK };

typedef struct RBNode {
    struct RBNode *parent;
    struct RBNode *child[2];
    int color;

    /* element */
} RBNode;

struct RBTree {
    int (*compar)(void *, void *);
    size_t elem_size;
    RBNode *root;
    void *alloc_ctx;
    void *(*alloc)(void *, size_t);
    void (*dealloc)(void *, void *, size_t);
};

/* doing n + 1 is the same as n + sizeof(*n), pointer arithmetic */
#define NODE_ELEM(n) ((char*)(n + 1))

static void *default_alloc(void *ctx, size_t size)
{
    (void) ctx;
    return malloc(size);
}

static void default_dealloc(void *ctx, void *reg, size_t size)
{
    (void) ctx;
    (void) size;
    free(reg);
}

RBTree rbtree_new__(size_t elem_size)
{
    RBTree tree = malloc(sizeof(*tree));
    tree->elem_size = elem_size;
    tree->root = NULL;
    tree->alloc_ctx = NULL;
    tree->alloc = default_alloc;
    tree->dealloc = default_dealloc;

    return tree;
}

RBTree rbtree_new_ex__(size_t elem_size, void *alloc_ctx,
        void *(*alloc)(void *alloc_ctx, size_t),
        void (*dealloc)(void *alloc_ctx, void *, size_t))
{
    RBTree tree = alloc(alloc_ctx, sizeof(*tree));
    tree->elem_size = elem_size;
    tree->root = NULL;
    tree->alloc_ctx = alloc_ctx;
    tree->alloc = alloc;
    tree->dealloc = dealloc;

    return tree;
}

void rbtree_set_compar(RBTree tree, int (*compar)(void*, void*))
{
    tree->compar = compar;
}

static void rotateDirRoot(RBTree T, RBNode *P, int dir)
{
    RBNode *G, *S, *C;
    G = P->parent;
    S = P->child[1-dir];
    assert(S != NULL);
    C = S->child[dir];
    P->child[1-dir] = C; if (C != NULL) C->parent = P;
    S->child[  dir] = P; P->parent = S;
    S->parent = G;
    if (G != NULL) {
        G->child[P == G->child[1] ? 1 : 0] = S;
    } else {
        T->root = S;
    }
}

#define rotateDir(P, dir) rotateDirRoot(T, P, dir)

static RBNode *newNode(const RBTree T, void *v)
{
    RBNode *node = T->alloc(T->alloc_ctx, sizeof(*node) + T->elem_size);
    memset(node, 0, sizeof(*node));
    memcpy(NODE_ELEM(node), v, T->elem_size);

    return node;
}

static RBNode *findParent(RBTree T, void *v, int *dir)
{
    RBNode *current = T->root;

    *dir = 0;
    if (current == NULL) return current;
    do {
        if (T->compar(v, NODE_ELEM(current)) < 0) *dir = 0;
        else if (T->compar(v, NODE_ELEM(current)) > 0) *dir = 1;
        else { *dir = -1; return NULL; }
        if (current->child[*dir]) current = current->child[*dir];
        else break;
    } while (1);

    return current;
}

static int childDir(const RBNode *N)
{
    return N != N->parent->child[0];
}

static void insert1(RBTree T, RBNode *N, RBNode *P, int dir)
{
    RBNode *G, *U;

    N->color = RED;
    N->parent = P;
    if (P == NULL) {
        T->root = N;
        return;
    }

    P->child[dir] = N;

    do {
        if (P->color == BLACK) return;

        if ((G = P->parent) == NULL) goto Case_I4;
        dir = childDir(P);
        U = G->child[1-dir];
        if (U == NULL || U->color == BLACK) goto Case_I56;
        P->color = BLACK;
        U->color = BLACK;
        G->color = RED;
        N = G;
    } while ((P = N->parent) != NULL);
    return;

 Case_I4:
    P->color = BLACK;
    return;

 Case_I56:
    if (N == P->child[1-dir]) {
        rotateDir(P, dir);
        N = P;
        P = G->child[dir];
    }

    rotateDirRoot(T, G, 1 - dir);
    P->color = BLACK;
    G->color = RED;
}

void *rbtree_insert(RBTree T, void *v)
{
    int dir;
    RBNode *node = newNode(T, v);
    RBNode *parent = findParent(T, v, &dir);
    if (parent == NULL && dir == -1) return NULL;

    insert1(T, node, parent, dir);
    return NODE_ELEM(node);
}

void *rbtree_get(RBTree T, void *v)
{
    int dir;
    RBNode *current = T->root;

    if (current == NULL) return NULL;
    do {
        if (T->compar(v, NODE_ELEM(current)) < 0) dir = 0;
        else if (T->compar(v, NODE_ELEM(current)) > 0) dir = 1;
        else return NODE_ELEM(current);
        if (current->child[dir]) current = current->child[dir];
        else return NULL;
    } while (1);
}

#define first child[0]
#define second child[1]

static RBNode *getSuccessor(RBNode *N)
{
    N = N->second;
    while (N->first) {
        N = N->first;
    }

    return N;
}

static void delete2(RBTree T, RBNode *N)
{
    int dir;
    RBNode *P, *S, *C, *D;

    P = N->parent;
    dir = childDir(N);
    P->child[dir] = NULL;
    T->dealloc(T->alloc_ctx, N, sizeof(*N) + T->elem_size);
    goto Start_D;

    do {
        dir = childDir(N);
    Start_D:
        S = P->child[1 - dir];
        D = S->child[1 - dir];
        C = S->child[    dir];

        if (S->color == RED) goto Case_D3;
        if (D != NULL && D->color == RED) goto Case_D6;
        if (C != NULL && C->color == RED) goto Case_D5;
        if (P->color == RED) goto Case_D4;
        S->color = RED;
        N = P;
    } while ((P = N->parent) != NULL);
    return;

 Case_D3:
    rotateDirRoot(T, P, dir);
    P->color = RED;
    S->color = BLACK;
    S = C;
    D = S->child[1 - dir];
    if (D != NULL && D->color == RED) goto Case_D6;
    C = S->child[dir];
    if (C != NULL && C->color == RED) goto Case_D5;

 Case_D4:
    S->color = RED;
    P->color = BLACK;
    return;

 Case_D5:
    rotateDir(S, 1-dir);
    S->color = RED;
    C->color = BLACK;
    D = S;
    S = C;

 Case_D6:
    rotateDirRoot(T, P, dir);
    S->color = P->color;
    P->color = BLACK;
    D->color = BLACK;
}

static RBNode **parentChildRef(RBTree T, RBNode *N)
{
    if (N->parent) return &N->parent->child[childDir(N)];
    else return &T->root;
}

int rbtree_delete(RBTree T, void *v)
{
    RBNode *N = rbtree_get(T, v);
    if (N-- == NULL) return -1;

    if (N->first && N->second) {
        RBNode *S = getSuccessor(N), t = *S;
        *parentChildRef(T, N) = S;
        *parentChildRef(T, S) = N;
        *S = *N;
        *N = t;
        if (N->parent == N) N->parent = S;
        if (S->first) S->first->parent = S;
        if (S->second) S->second->parent = S;
    }

    if (N->first || N->second) {
        RBNode *C = N->first; if (!C) C = N->second;

        assert(C->color == RED);
        assert(N->color == BLACK);
        *parentChildRef(T, N) = C;
        C->parent = N->parent;
        C->color = BLACK;
        T->dealloc(T->alloc_ctx, N, sizeof(*N) + T->elem_size);
        return 0;
    }

    if (N->color == RED || !N->parent) {
        *parentChildRef(T, N) = NULL;
        T->dealloc(T->alloc_ctx, N, sizeof(*N) + T->elem_size);
    } else delete2(T, N);

    return 0;
}

static void rbtree_free_node(RBTree T, RBNode *node)
{
    if (node->child[0]) rbtree_free_node(T, node->child[0]);
    if (node->child[1]) rbtree_free_node(T, node->child[1]);
    T->dealloc(T->alloc_ctx, node, sizeof(*node) + T->elem_size);
}

void rbtree_destroy(RBTree T)
{
    if (T->root) rbtree_free_node(T, T->root);
    T->dealloc(T->alloc_ctx, T, sizeof(*T));
}

static void nodeTraverse(RBNode *node, void *data, void (*traverser)(void *, void *data))
{
    if (node->child[0]) nodeTraverse(node->child[0], data, traverser);
    traverser(NODE_ELEM(node), data);
    if (node->child[1]) nodeTraverse(node->child[1], data, traverser);
}

void rbtree_traverse(RBTree T, void *data, void (*traverser)(void *, void *data))
{
    if (T->root == NULL) return;

    nodeTraverse(T->root, data, traverser);
}

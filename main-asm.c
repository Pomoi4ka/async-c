#include <stdio.h>
#include <poll.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <setjmp.h>

#include "rb-tree.h"
#include "da.h"

typedef enum {
    TASK_CREATED,
    TASK_DONE,
    TASK_WAITING,
    TASK_YIELD,
    TASK_YIELD_VALUE
} Task_Status;

typedef struct EventLoop EventLoop;

typedef struct Task {
    void (*func)(struct Task*);
    void *stack;
    void *yielded_value;
    struct Task *waiting;
    EventLoop *event_loop;
    Task_Status status;
    int id;
    jmp_buf task_env;
} Task;

typedef RBTree Tasks;

typedef struct {
    Task **items;
    size_t count;
    size_t capacity;
} TasksPtrs;

struct EventLoop {
    Tasks tasks;
    TasksPtrs finalized_task_ids;
    int last_id;
    int running_tasks;
    jmp_buf event_loop_env;
};

int compare_task(void *t1, void *t2)
{
    return ((Task*)t1)->id - ((Task*)t2)->id;
}

Task *async(void (*f)(Task *), Task *c)
{
    Task t = {0}, *result;
    t.func = f;
    t.event_loop = c->event_loop;

    do {
        t.id = c->event_loop->last_id++;
        result = rbtree_insert(c->event_loop->tasks, &t);
    } while (result == NULL);
    return result;
}

#define yield do { \
        if (setjmp(task->task_env) != 1) longjmp(task->event_loop->event_loop_env, 1); \
    } while (0)

#define await(iterating) do {                   \
        task->status = TASK_WAITING;            \
        task->waiting = iterating;              \
        yield;                                  \
    } while (iterating->status != TASK_DONE)

#define yield_value(val) do {                   \
        task->yielded_value = val;              \
        task->status = TASK_YIELD_VALUE;        \
        yield;                                  \
    } while (0)

void *yield_from__(Task *iterating, Task *task)
{
    task->status = TASK_WAITING;
    task->waiting = iterating;
    yield;
    return iterating->yielded_value;
}

#define yield_from(iterating) yield_from__(iterating, task)

void iterate(register Task *task)
{
    #define STACK_SIZE 8192
    int status = setjmp(task->event_loop->event_loop_env);
    if (status == 1) return;
    if (status == 2) {
        free((char*)task->stack - STACK_SIZE);
        task->status = TASK_DONE;
        task->event_loop->running_tasks--;
        return;
    }

    switch (task->status) {
    case TASK_CREATED: {
        task->event_loop->running_tasks++;
        task->status = TASK_YIELD;
        task->stack = (char*)malloc(STACK_SIZE) + STACK_SIZE;
        __asm__("mov %0, %%rsp"
                :
                : "r"(task->stack));
        task->func(task);
        longjmp(task->event_loop->event_loop_env, 2);
    } break;
    case TASK_WAITING:
        if (task->waiting->status == TASK_DONE) {
            da_append(&task->event_loop->finalized_task_ids, task->waiting);
            task->waiting = NULL;
        } else if (task->waiting->status != TASK_YIELD_VALUE) break;
        else task->waiting->status = TASK_YIELD;
        task->status = TASK_YIELD;
        /* fallthrough */
    case TASK_YIELD:
        longjmp(task->task_env, 1);
        break;
    case TASK_YIELD_VALUE:
    case TASK_DONE:
    default:
        assert(0 && "unreachable");
    }
}

void iterate_yields(void *_task, void *_data)
{
    Task *task = _task;
    (void) _data;

    if (task->status == TASK_YIELD ||
        task->status == TASK_CREATED ||
        task->status == TASK_WAITING) {
        iterate(task);
    }
}

void event_loop(void (*main)(Task *))
{
    EventLoop el = {0};
    Task main_task = {0};
    el.tasks = rbtree_new(Task);
    rbtree_set_compar(el.tasks, compare_task);

    main_task.event_loop = &el;
    main_task.func = main;
    main_task.id = el.last_id++;
    rbtree_insert(el.tasks, &main_task);

    do {
        size_t i = 0;
        rbtree_traverse(el.tasks, NULL, iterate_yields);
        for (; i < el.finalized_task_ids.count; ++i) {
            rbtree_delete(el.tasks, el.finalized_task_ids.items[i]);
        }
        el.finalized_task_ids.count = 0;
    } while (el.running_tasks);
    rbtree_destroy(el.tasks);
    free(el.finalized_task_ids.items);
}

void foo(Task *task)
{
    int i;

    for (i = 0; i < 10; ++i) {
        printf("%02d: foo %d\n", task->id, i);
        yield_value(&i);
    }
}

void async_main(Task *task)
{
    size_t i;
    Task *ts[10];
    for (i = 0; i < ARRAY_LEN(ts); ++i) {
        int j;
        ts[i] = async(foo, task);
        printf("async_main %lu\n", i);
        for (j = 0; j < 8; j++) {
            printf("foo(%d) yielded %d\n", ts[i]->id, *(int*)yield_from(ts[i]));
        }
    }
    for (i = 0; i < ARRAY_LEN(ts); ++i) await(ts[i]);
}

int main(void)
{
    event_loop(async_main);

    return 0;
}

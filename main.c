#include <stdio.h>
#include <poll.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#define da_append(da, x)                                                \
    do {                                                                \
        if ((da)->count >= (da)->capacity) {                            \
            (da)->capacity = (da)->capacity ? (da)->capacity*2 : 1;     \
            (da)->items = realloc((da)->items, sizeof(*(da)->items)*(da)->capacity); \
            assert((da)->items);                                        \
        }                                                               \
        (da)->items[(da)->count++] = x;                                 \
    } while (0)

typedef struct {
    struct pollfd *items;
    size_t count;
    size_t capacity;
} Polls;

typedef struct {
    size_t current_microtask;
    void *closure;
    int io_require;
    int done;
} Task;

#define ASYNC_FUNCTION_BEGIN                                            \
    size_t __microtask = 0;                                             \
    if (task->closure == NULL) task->closure = malloc(sizeof(*closure)); \
    closure = task->closure;
#define ASYNC_FUNCTION_END \
    task->done = 1; \
    free(task->closure);
#define MICROTASK_BEGIN if (task->current_microtask == __microtask++) {
#define MICROTASK_END task->current_microtask++; }
#define MICROTASK_INTERRUPT(ev) \
    do {                                        \
        if (task->io_require == ev) break;      \
        task->io_require = ev;                  \
        return;                                 \
    } while (0)


void async_func(Task *task)
{
    struct {
        char message[256];
    } *closure;
    ASYNC_FUNCTION_BEGIN
    MICROTASK_BEGIN
        MICROTASK_INTERRUPT(POLLOUT);
        printf("Enter your name: ");
        fflush(stdout);
    MICROTASK_END
    MICROTASK_BEGIN
        MICROTASK_INTERRUPT(POLLIN);
        scanf("%s", closure->message);
    MICROTASK_END
    MICROTASK_BEGIN
        MICROTASK_INTERRUPT(POLLOUT);
        printf("Hello, %s!\n", closure->message);
        fflush(stdout);
    MICROTASK_END
    ASYNC_FUNCTION_END
}

void event_loop(void)
{
    Polls polls = {0};
    struct pollfd pfd = {0};
    Task task = {0};
    pfd.fd = STDIN_FILENO;

    pfd.events = 0;
    da_append(&polls, pfd);

    while (!task.done) {
        async_func(&task);
        polls.items[0].events = task.io_require;
        poll(polls.items, polls.count, -1);
        task.io_require = polls.items[0].revents;
    }
}

int main(void)
{
    event_loop();

    return 0;
}

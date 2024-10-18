#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static pid_t *pid_arr;
static int    n;

void
handler(int s)
{
    printf("In handler\n");
    pid_t prev = -1;
    pid_t current;
    for (int i = 0; i < n; i++)
    {
        current = pid_arr[i];
        if (current == 0) // la lista se copia con los
                          // valores que tenia antes del fork,
                          // por lo que todo son ceros a partir (incluido)
                          // del actual.
        {
            if (prev != -1)
            {
                printf("El hijo %d mato a %d\n", current, prev);
                kill(prev, SIGTERM);
            }
            else
                printf("El primero hijo no puede matar a nadie!\n");

            break;
        }
        prev = current;
    }
}

int
on_child()
{
    printf("Hijo: pid %d \n", getpid());
    while (1)
        sleep(1);
    return 0;
}

int
main(int argc, char *argv[])
{
    signal(SIGUSR2, handler);

    printf("Cuantos hijos quieres crear? ");
    scanf(" %d", &n);

    assert(n >= 0);

    pid_arr = calloc(n, sizeof(pid_t));

    for (int i = 0; i < n; i++)
    {
        pid_arr[i] = fork();
        if (pid_arr[i] == 0)
            exit(on_child());
    }

    waitpid(pid_arr[n - 1], NULL, 0);
    free(pid_arr);

    return 0;
}

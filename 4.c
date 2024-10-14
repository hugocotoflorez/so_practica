#include <signal.h>
#include <stdio.h>
#include <unistd.h>

void
handler(int s)
{
    static int count = 0;
    char       input;

    if (count++ == 3)
    {
        printf("Deseas salir al modo normal? (y/n): ");
        scanf(" %c", &input);

        if (input == 'y')
            signal(SIGINT, SIG_DFL);
    }

    printf("Another Cc\n");
}

int
main(int argc, char *argv[])
{
    signal(SIGINT, handler);
    while (1)
    {
        usleep(300000);
    }
    return 0;
}

#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static FILE *shared_file;

static void
jugador_handler(int sig)
{
    static int pos = -1;
    int        npos;

    switch (sig)
    {
        // Obtener nueva posicion
        case SIGUSR1:
            do
            {
                printf("Posicion [0,9]: ");
                scanf(" %d", &npos);
            } while (pos >= 0 && abs(pos - npos) > 2);

            pos = npos;
            fprintf(shared_file, "%d\n", pos);

            // Avisar al padre de que ya esta el dato en
            // el archivo
            kill(getppid(), SIGUSR1);

            break;

        // obtener direccion pelota
        case SIGUSR2:
            do
            {
                printf("Direccion pelota [0,9]: ");
                scanf(" %d", &npos);
            } while (pos >= 0 && abs(pos - npos) > 2);

            pos = npos;
            fprintf(shared_file, "%d\n", pos);

            // Avisar al padre de que ya esta el dato en
            // el archivo
            kill(getppid(), SIGUSR1);

            break;
    }
}

static void
maquina_handler(int sig)
{
    switch (sig)
    {
        case SIGUSR1:
            break;

        case SIGUSR2:
            break;
    }
}

int
jugador()
{
    signal(SIGUSR1, jugador_handler);
    signal(SIGUSR2, jugador_handler);
    for (;;)
        pause();
    return 0;
}

int
maquina()
{
    signal(SIGUSR1, maquina_handler);
    signal(SIGUSR2, maquina_handler);
    for (;;)
        pause();
    return 0;
}

int
main_loop()
{
    int turno;
    int marcador[2] = { 0 };
    int jpos;
    int mpos;

    srand(time(NULL));
    turno = rand() % 2;


    while (marcador[0] < 10 && marcador[1] < 10)
    {
        jpos = get_jposicion();
        mpos = get_mposition();

        switch (turno)
        {
            case 0:
                break;
            case 1:
                break;
        }
    turno = (turno + 1) % 2;
    }
}

int
main(int argc, char *argv[])
{
    pid_t jugador_pid;
    pid_t maquina_pid;

    // crear archivo temporal
    // para compartir informacion
    shared_file = tmpfile();
    assert(shared_file);

    jugador_pid = fork();
    assert(jugador_pid >= 0);

    if (!jugador_pid)
        exit(jugador());

    maquina_pid = fork();
    assert(maquina_pid >= 0);

    if (!maquina_pid)
        exit(maquina());

    return main_loop();
}

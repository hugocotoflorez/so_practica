#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static int shared_fd;

int
get_posicion(int pid)
{
    int pos = -1;

    puts("Asking for position");
    kill(pid, SIGUSR1);
    puts("Waiting for response");
    pause(); // wait for signal
             // when position is written a
             // signal is sent
    read(shared_fd, &pos, sizeof(int));
    printf("Response: %d\n", pos);

    return pos;
}

int
get_direccion(int pid)
{
    int dir;

    puts("Asking for direction");
    kill(pid, SIGUSR2);
    puts("Waiting for response");
    pause(); // wait for signal
             // when dirition is written a
             // signal is sent
    read(shared_fd, &dir, sizeof(int));
    printf("Response: %d\n", dir);

    return dir;
}

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
                if (pos < 0)
                    printf("Posicion [0,9]: ");
                else
                    printf("Posicion [%d,%d]: ", pos < 2 ? 0 : pos - 2,
                           pos > 7 ? 9 : pos + 2);

                scanf("%d", &npos);
            } while (pos >= 0 && abs(pos - npos) > 2 && pos <= 9);

            pos = npos;
            write(shared_fd, &pos, sizeof(int));
            fsync(shared_fd);
            puts("Written");

            // Avisar al padre de que ya esta el dato en
            // el archivo
            kill(getppid(), SIGUSR1);

            break;

        // obtener direccion pelota
        case SIGUSR2:
            printf("Direccion pelota [0,9]: ");
            scanf("%d", &pos);
            write(shared_fd, &pos, sizeof(int));
            fsync(shared_fd);
            puts("Written");

            // Avisar al padre de que ya esta el dato en
            // el archivo
            kill(getppid(), SIGUSR1);

            break;
    }
}

static void
maquina_handler(int sig)
{
    static int pos = -1;
    int        npos;

    switch (sig)
    {
        // Obtener nueva posicion
        case SIGUSR1:
            printf("Getting M pos\n");
            do
            {
                npos = rand() % 10;
            } while (pos >= 0 && abs(pos - npos) > 2);

            pos = npos;

            write(shared_fd, &pos, sizeof(int));
            fsync(shared_fd);
            puts("Written");

            // Avisar al padre de que ya esta el dato en
            // el archivo
            kill(getppid(), SIGUSR1);

            break;

        // obtener direccion pelota
        case SIGUSR2:
            printf("Getting M dir\n");
            pos = rand() % 10;
            write(shared_fd, &pos, sizeof(int));
            fsync(shared_fd);
            puts("Written");

            // Avisar al padre de que ya esta el dato en
            // el archivo
            kill(getppid(), SIGUSR1);

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

enum Turno
{
    JUGADOR = 0,
    MAQUINA = 1,
};

int
es_punto(int *marcador, int jpos, int mpos, int dir, int turno)
{
    switch (turno)
    {
        case JUGADOR:
            if (abs(mpos - dir) > 2)
            {
                ++marcador[turno];
                printf("Punto de J\n");
                return 1;
            }
            break;

        case MAQUINA:
            if (abs(jpos - dir) > 2)
            {
                ++marcador[turno];
                printf("Punto de M\n");
                return 1;
            }
            break;
    }
    return 0;
}

void
main_loop(int jugador, int maquina)
{
    int turno;
    int marcador[2] = { 0 };
    int dir;
    int jpos;
    int mpos;

    srand(time(NULL));

    while (marcador[0] < 10 && marcador[1] < 10)
    {
        turno = rand() % 2;
        printf("Saca %s\n", ((char *[2]) { "Jugador", "Maquina" })[turno]);
        do
        {
            switch (turno)
            {
                case JUGADOR:
                    jpos = get_posicion(jugador);
                    mpos = get_posicion(maquina);
                    dir  = get_direccion(jugador);
                    break;

                case MAQUINA:
                    jpos = get_posicion(jugador);
                    mpos = get_posicion(maquina);
                    dir  = get_direccion(maquina);
                    break;
            }

            turno = (turno + 1) % 2;
        } while (!es_punto(marcador, jpos, mpos, dir, turno));
    }
}

void
parent_handler(int s)
{
    // other signals are recived but ignored
    if (s == SIGTERM)
    {
        close(shared_fd);
        kill(-getpid(), SIGTERM);
        raise(SIGKILL);
    }
    return;
}

int
main(int argc, char *argv[])
{
    pid_t jugador_pid;
    pid_t maquina_pid;

    // crear archivo temporal
    // para compartir informacion
    // shared_file = tmpfile();
    shared_fd = open("shared", O_TRUNC | O_RDWR | O_CREAT, 0666);
    assert(shared_fd >= 0);

    // crear proceso jugador
    jugador_pid = fork();
    assert(jugador_pid >= 0);

    // codigo que ejecuta el jugador
    if (!jugador_pid)
        exit(jugador());

    // crear proceso maquina
    maquina_pid = fork();
    assert(maquina_pid >= 0);

    // codigo que ejecuta la maquina
    if (!maquina_pid)
        exit(maquina());

    signal(SIGUSR1, parent_handler);
    signal(SIGUSR2, parent_handler);
    signal(SIGTERM, parent_handler);

    main_loop(jugador_pid, maquina_pid);

    return 0;
}

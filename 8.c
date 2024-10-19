#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define BBG "\e[40;37m"
#define GBG "\e[42;30m"
#define RST "\e[0m"

static int shared_fd;

int
get_posicion(int pid)
{
    int pos = -1;

    kill(pid, SIGUSR1);
    pause(); // wait for signal
             // when position is written a
             // signal is sent
    lseek(shared_fd, -sizeof(int), SEEK_CUR);
    read(shared_fd, &pos, sizeof(int));

    return pos;
}

int
get_direccion(int pid)
{
    int dir = -1;

    kill(pid, SIGUSR2);
    pause(); // wait for signal
             // when dirition is written a
             // signal is sent
    lseek(shared_fd, -sizeof(int), SEEK_CUR);
    read(shared_fd, &dir, sizeof(int));

    return dir;
}

static void
jugador_handler(int sig)
{
    static int pos = -1;
    static int dir;
    int        npos;

    switch (sig)
    {
        // Obtener nueva posicion
        case SIGUSR1:
            do
            {
                printf("[Mover jugador]\n");
                if (pos < 0)
                    printf(BBG "[0,9]:" RST " ");
                else
                    printf(BBG "[%d,%d]:" RST " ", pos < 2 ? 0 : pos - 2,
                           pos > 7 ? 9 : pos + 2);

                scanf("%d", &npos);
            } while (pos >= 0 && npos >= 0 && abs(pos - npos) > 2 && npos <= 9);

            pos = npos;
            write(shared_fd, &pos, sizeof(int));
            fsync(shared_fd);

            // Avisar al padre de que ya esta el dato en
            // el archivo
            kill(getppid(), SIGUSR1);

            break;

        // obtener direccion pelota
        case SIGUSR2:
            do
            {
                printf(BBG "Direccion pelota [0,9]:" RST " ");
                scanf("%d", &dir);
            } while (!(0 <= dir && dir <= 9));

            write(shared_fd, &dir, sizeof(int));
            fsync(shared_fd);

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
    static int dir;
    int        npos;

    switch (sig)
    {
        // Obtener nueva posicion
        case SIGUSR1:
            do
            {
                npos = rand() % 10;
            } while (pos >= 0 && abs(pos - npos) > 2);

            pos = npos;

            write(shared_fd, &pos, sizeof(int));
            fsync(shared_fd);

            // Avisar al padre de que ya esta el dato en
            // el archivo
            kill(getppid(), SIGUSR1);
            break;

        // obtener direccion pelota
        case SIGUSR2:
            dir = rand() % 10;
            write(shared_fd, &dir, sizeof(int));
            fsync(shared_fd);

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
    switch ((turno + 1) % 2)
    {
        case JUGADOR:
            if (abs(mpos - dir) > 3)
            {
                ++marcador[turno];
                printf("Punto de J\n");
                printf("Marcador: " GBG "[J %d - %d M]" RST "\n",
                       marcador[JUGADOR], marcador[MAQUINA]);
                return 1;
            }
            else
                printf("La maquina devuelve la bola\n");
            break;

        case MAQUINA:
            if (abs(jpos - dir) > 3)
            {
                ++marcador[turno];
                printf("Punto de M\n");
                printf("Marcador: " GBG "[J %d - %d M]" RST "\n",
                       marcador[JUGADOR], marcador[MAQUINA]);
                return 1;
            }
            else
                printf("El jugador devuelve la bola\n");
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

    while (marcador[JUGADOR] < 10 && marcador[MAQUINA] < 10)
    {
        // Posiciones iniciales
        turno = rand() % 2;
        jpos  = get_posicion(jugador);
        mpos  = get_posicion(maquina);

        printf("-> Saca %s\n", ((char *[2]) { "Jugador", "Maquina" })[turno]);
        do
        {
            printf("-> J: %d\n"
                   "   M: %d\n",
                   jpos, mpos);
            // Obtener direccion de la bola
            switch (turno)
            {
                case JUGADOR:
                    dir = get_direccion(jugador);
                    printf("Direccion de la bola: %d\n", dir);
                    mpos = get_posicion(maquina);
                    break;

                case MAQUINA:
                    dir = get_direccion(maquina);
                    printf("Direccion de la bola: %d\n", dir);
                    jpos = get_posicion(jugador);
                    break;
            }

            // mover jugadores
            turno = (turno + 1) % 2;

        } while (!es_punto(marcador, jpos, mpos, dir, turno));
    }
    printf("Gana %s\n", ((char *[2]) { "Jugador", "Maquina" })[turno]);
}

void
parent_handler(int s)
{
    // other signals are recived but ignored
    switch (s)
    {
        case SIGTERM:
        {
            close(shared_fd);
            kill(-getpid(), SIGTERM);
            raise(SIGKILL);
        }
        case SIGUSR1:
        case SIGUSR2:
            break;
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
    shared_fd = open("shared.txt", O_RDWR | O_TRUNC | O_CREAT, 0666);
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

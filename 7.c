#include <csignal>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> //pid_t
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

// Función para imprimir la hora actual y el PID
void
print_timestamp(const char *msg)
{
    time_t     now = time(NULL);
    struct tm *t   = localtime(&now);
    printf("[%02d:%02d:%02d] [PID: %d] %s\n", t->tm_hour, t->tm_min,
           t->tm_sec, getpid(), msg);
}

void
handler_padre(int signal)
{
    switch (signal)
    {
        case SIGUSR1:
            print_timestamp("El proceso Padre ha recibido SIGUSR1");
            break;
        case SIGUSR2:
            print_timestamp("El proceso Padre ha recibido SIGUSR2");
            break;
        default:
            break;
    }
}

int
hijo1()
{
    // Enviar SIGUSR1 al padre
    print_timestamp("El proceso Hijo 1 envía SIGUSR1 al Padre");
    kill(getppid(), SIGUSR1);
    sleep(20);

    // Enviar SIGUSR2 al padre
    print_timestamp("El proceso Hijo 1 envía SIGUSR2 al Padre");
    kill(getppid(), SIGUSR2);
    sleep(20);

    // Finalizar con código 33
    print_timestamp("El proceso Hijo 1 finaliza");
    return 33;
}

void
hijo2()
{
    // Hijo 2 finaliza inmediatamente
    kill(getppid(), SIGINT);
    print_timestamp("El proceso Hijo 2 envia SIGINT al padre");
    exit(1);
}

int
main(int argc, char const *argv[])
{
    pid_t h1, h2;
    int   codigo_h1 = 0;

    // Configurar manejadores de señales para el padre
    signal(SIGUSR1, handler_padre);
    signal(SIGUSR2, handler_padre);
    signal(SIGINT, SIG_IGN); // Ignorar SIGINT

    // Crear el primer hijo
    if ((h1 = fork()) == 0)
    {
        // Código del Hijo 1
        exit(hijo1());
    }

    // Crear el segundo hijo
    if ((h2 = fork()) == 0)
    {
        // Código del Hijo 2
        hijo2();
    }

    // Bloquear la señal SIGUSR1 temporalmente
    struct sigaction bloqueo;
    sigset_t         block_mask, blocked_masks;

    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &block_mask, NULL);

    print_timestamp("El proceso Padre bloquea SIGUSR1 temporalmente");

    // Esperar 5 segundos para comprobar el bloqueo
    sleep(22);
    sigpending(&blocked_masks);

    if (sigismember(&blocked_masks, SIGUSR1))
    {
        print_timestamp("SIGUSR1 está bloqueado");
    }

    // Desbloquear la señal SIGUSR1
    sigprocmask(SIG_UNBLOCK, &block_mask, NULL);
    print_timestamp("El proceso Padre desbloquea SIGUSR1");

    // Esperar la finalización del primer hijo y obtener su código de salida
    waitpid(h1, &codigo_h1, 0);

    char mensaje[100];
    sprintf(mensaje, "El proceso Padre recibe el código de salida del Hijo 1: %d",
            WEXITSTATUS(codigo_h1));
    print_timestamp(mensaje);

    return 0;
}

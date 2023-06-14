#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <fcntl.h>

void start_daemon(const char *log_name, int facility)
{
    pid_t pid;

    // Forken, um einen Waisenprozess zu erzeugen
    pid = fork();
    if (pid < 0)
    {
        perror("Fehler beim Forken");
        exit(EXIT_FAILURE);
    }

    // Elternprozess beenden
    if (pid > 0)
    {
        exit(EXIT_SUCCESS);
    }

    // Zugriffsberechtigung für die Datei festlegen
    umask(0);

    // Neue Sitzung und Gruppenführerschaft übernehmen
    if (setsid() < 0)
    {
        perror("Fehler beim Erzeugen der neuen Sitzung");
        exit(EXIT_FAILURE);
    }

    // Arbeitsverzeichnis auf "/" ändern
    if (chdir("/") < 0)
    {
        perror("Fehler beim Ändern des Arbeitsverzeichnis");
        exit(EXIT_FAILURE);
        // umask
    }
    // Filediskreptoren schließen
    for (int fd = sysconf(_SC_OPEN_MAX); fd > 0; fd--)
    {
        close(fd);
    }

    // Umleitung der Standard-Ein-/Ausgabe
    open("/dev/null", O_RDONLY);                         // STDIN_FILENO
    open(log_name, O_WRONLY | O_CREAT | O_APPEND, 0644); // STDOUT_FILENO
    dup(STDOUT_FILENO);                                  // STDERR_FILENO

    // Initialisierung des Log-Systems
    setlogmask(LOG_UPTO(LOG_INFO));
    openlog(log_name, LOG_CONS | LOG_PID | LOG_NDELAY, facility);

    syslog(LOG_INFO, "Daemon gestartet");
}
void stop_daemon()
{
    pid_t pid;
    FILE *pid_file;

    pid_file = fopen("/var/run/daemon.pid", "r");
    if (pid_file != NULL)
    {
        fscanf(pid_file, "%d", &pid);
        fclose(pid_file);

        kill(pid, SIGTERM);
        fprintf(stdout, "Daemon erfolgreich beendet.\n");
        closelog();
    }
    else
    {
        fprintf(stderr, "Fehler beim Beenden des Daemons\n");
        exit(1);
    }
}
void manage_daemon()
{
    int choice;

    while (1)
    {
        printf("\nDaemons verwalten\n");
        printf("1. CPU-Zeit abrufen\n");
        printf("2. RAM-Verbrauch abrufen\n");
        printf("3. Daemon stoppen\n");
        printf("Bitte wählen Sie eine Option: ");
        scanf("%d", &choice);

        switch (choice)
        {
        case 1:
        {
            struct rusage usage;
            if (getrusage(RUSAGE_SELF, &usage) == 0)
            {
                printf("CPU-Zeit des Daemons: %ld.%06ld Sekunden\n", usage.ru_utime.tv_sec, usage.ru_utime.tv_usec);
            }
            else
            {
                printf("Fehler beim Abrufen der CPU-Zeit.\n");
            }
            break;
        }
        case 2:
        {
            struct rusage usage;
            if (getrusage(RUSAGE_SELF, &usage) == 0)
            {
                printf("RAM-Verbrauch des Daemons: %ld Kilobytes\n", usage.ru_maxrss);
            }
            else
            {
                printf("Fehler beim Abrufen des RAM-Verbrauchs.\n");
            }
            break;
        }
        case 3:
            stop_daemon();
            printf("Daemon wird gestoppt...\n");
            // ...
            printf("Daemon erfolgreich gestoppt.\n");
            break;
        case 4:
            // Programm beenden
            printf("Programm wird beendet.\n");
            exit(EXIT_SUCCESS);
        default:
            printf("Ungültige Option. Bitte versuchen Sie es erneut.\n");
            break;
        }
    }
}
int main()
{
    char *daemonStarten;
    printf("Wollen Sie einen Daemon starten?");
    scanf("%s", &daemonStarten);
    if (daemonStarten == "ja")
    {
        start_daemon("meinDaemon", LOG_LOCAL0);
        manage_daemon();
    }
    else if (daemonStarten == "nein")
    {
        printf("Dann nicht");
    }
    else
    {
        printf("Ungueltige Eingabe");
    }
}
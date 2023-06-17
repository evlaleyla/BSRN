#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <syslog.h>

void start_daemon()
{
    int i;
    pid_t pid = fork(); // Erstelle einen Kindprozess
    if (pid < 0)
    {
        fprintf(stderr, "Fehler beim Starten des Daemons\n");
        exit(1);
    }
    else if (pid > 0)
    {
        exit(0); // Beende den Elternprozess
    }

    umask(0); // Setze die Zugriffsrechte für Dateien

    if (setsid() < 0)
    {
        fprintf(stderr, "Fehler beim Erstellen einer neuen Sitzung\n");
        exit(1);
    }
    for (i = sysconf (_SC_OPEN_MAX); i > 0; i--)
    close(i);


    close(STDIN_FILENO);  // Schließe die Standard-Eingabe
    close(STDOUT_FILENO); // Schließe die Standard-Ausgabe
    close(STDERR_FILENO); // Schließe die Standard-Fehlerausgabe
}

void create_pid_file()
{
    FILE *pid_file = fopen("/var/run/daemon.pid", "w");
    if (!pid_file)
    {
        fprintf(stderr, "Fehler beim Erstellen der PID-Datei\n");
        exit(1);
    }

    fprintf(pid_file, "%d", getpid()); // Schreibe die PID des aktuellen Prozesses in die Datei

    fclose(pid_file);
}

void stop_daemon()
{
    FILE *pid_file = fopen("/var/run/daemon.pid", "r");
    if (!pid_file)
    {
        fprintf(stderr, "Fehler beim Lesen der PID-Datei\n");
        exit(1);
    }

    pid_t pid;
    fscanf(pid_file, "%d", &pid);

    fclose(pid_file);

    if (kill(pid, SIGTERM) < 0)
    {
        fprintf(stderr, "Fehler beim Beenden des Daemons\n");
        exit(1);
    }
}

void check_daemon_status()
{
    FILE *pid_file = fopen("/var/run/daemon.pid", "r");
    if (!pid_file)
    {
        fprintf(stderr, "Fehler beim Lesen der PID-Datei\n");
        exit(1);
    }

    pid_t pid;
    fscanf(pid_file, "%d", &pid);

    fclose(pid_file);

    if (kill(pid, 0) == 0)
    {
        printf("Daemon läuft (PID: %d)\n", pid);
    }
    else
    {
        printf("Daemon ist nicht aktiv\n");
    }
}

struct ProcessInfo
{
    pid_t process_id;
    uid_t process_uid;
    gid_t process_gid;
    unsigned long long memory_usage;
};

struct ProcessInfo get_process_info(pid_t pid)
{
    struct ProcessInfo info;
    char statm_path[256];
    snprintf(statm_path, sizeof(statm_path), "/proc/%d/statm", pid);
    FILE *statm_file = fopen(statm_path, "r");
    if (!statm_file)
    {
        fprintf(stderr, "Fehler beim Lesen der Prozessinformationen\n");
        exit(1);
    }

    fscanf(statm_file, "%llu", &info.memory_usage);
    info.memory_usage *= sysconf(_SC_PAGESIZE);

    fclose(statm_file);

    char stat_path[256];
    snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", pid);
    FILE *stat_file = fopen(stat_path, "r");
    if (!stat_file)
    {
        fprintf(stderr, "Fehler beim Lesen der Prozessinformationen\n");
        exit(1);
    }

    fscanf(stat_file, "%d", &info.process_id);
    fseek(stat_file, 256, SEEK_CUR);
    fscanf(stat_file, "%d %d", &info.process_uid, &info.process_gid);

    fclose(stat_file);

    return info;
}

void run_daemon()
{
    FILE *pid_file = fopen("/var/run/daemon.pid", "w");
    if (!pid_file)
    {
        fprintf(stderr, "Fehler beim Schreiben der PID-Datei\n");
        exit(1);
    }

    fprintf(pid_file, "%d", getpid()); // Schreibe die PID des Daemon-Prozesses in die Datei

    fclose(pid_file);

    while (1)
    {
        // Führe hier deine gewünschten Aufgaben aus
        // ...

        syslog(LOG_INFO, "Protokolleintrag des Daemons"); // Protokolliere eine Meldung

        sleep(5); // Warte für 5 Sekunden
    }
}

int main()
{
    int daemonStart;
    int daemonInformationen;
    printf("Wollen Sie einen Daemon starten?\nGeben Sie 1 ein, damit ein Daemon gestartet wird.\nGeben Sie 0 ein damit kein Daemon gestartet wird.");
    scanf("%d", &daemonStart);
    switch (daemonStart)
    {
    case 1:
        create_pid_file();
        start_daemon();
        printf("Daemon gestartet.\n");

        pid_t pid = fork(); // Erstelle einen weiteren Kindprozess für die eigentlichen Aufgaben des Daemons
        if (pid < 0)
        {
            fprintf(stderr, "Fehler beim Starten des Daemons\n");
            exit(1);
        }
        else if (pid == 0)
        {
            // Kindprozess führt den Daemon-Code aus
            run_daemon();
            exit(0);
        }
       

        printf("Möchten Sie Informationen erhalten? -> 1\nMöchten Sie den Daemon beenden? -> 0\n");
        scanf("%d", &daemonInformationen);
        if (daemonInformationen == 1)
        {
            check_daemon_status();
            struct ProcessInfo info = get_process_info(getpid());
            printf("Prozess ID: %d\n", info.process_id);
            printf("Benutzer ID: %d\n", info.process_uid);
            printf("Gruppen ID: %d\n", info.process_gid);
            printf("Speichernutzung: %llu Bytes\n", info.memory_usage);
        }
        else if (daemonInformationen == 0)
        {
            stop_daemon();
            printf("Daemon wird beendet.\n");
        }
        break;
    case 0:
        printf("dann nicht");
        break;

    default:
        printf("Flasche Eingabe");
        break;
    }
    return 0;
}

/*
openlog("daemon", LOG_PID | LOG_NDELAY, LOG_DAEMON); // Öffne das Syslog für den Daemon-Prozess

syslog(LOG_INFO, "Daemon gestartet (PID: %d)", getpid()); // Protokolliere eine Nachricht

// Erstelle einen neuen Prozess für den Daemon und führe den Code weiter aus
if (fork() > 0)
{
   exit(0); // Der ursprüngliche Prozess beendet sich
}

setsid(); // Trenne die Verbindung zum Terminal und erstelle eine neue Sitzung

close(STDIN_FILENO);  // Schließe den Standard-Eingabe-Dateideskriptor
close(STDOUT_FILENO); // Schließe den Standard-Ausgabe-Dateideskriptor
close(STDERR_FILENO); // Schließe den Standard-Fehler-Dateideskriptor

// Führe ein neues Programm im Daemon-Prozess aus
if (execvp("/pfad/zum/deinem/programm", NULL) < 0)
{
   fprintf(stderr, "Fehler beim Ausführen des Programms\n");
   exit(1);
}

closelog(); // Schließe das Syslog
*/

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

void start_daemon() {
    // Erzeugt einen neuen Prozess (Daemon-Prozess)
    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Fehler beim Starten des Daemons\n");
        exit(1);
    }
    else if (pid > 0) {
        exit(0); // Elternprozess beendet sich
    }

    umask(0); // Setzt die Zugriffsrechte für Dateiberechtigungen

    if (setsid() < 0) {
        fprintf(stderr, "Fehler beim Erstellen einer neuen Sitzung\n");
        exit(1);
    }

    close(STDIN_FILENO); // Schließt den Standard-Eingabestrom
    close(STDOUT_FILENO); // Schließt den Standard-Ausgabestrom
    close(STDERR_FILENO); // Schließt den Standard-Fehlerstrom
}

void stop_daemon() {
    // Liest die PID-Datei des Daemons
    FILE* pid_file = fopen("/var/run/daemon.pid", "r");
    if (!pid_file) {
        fprintf(stderr, "Fehler beim Lesen der PID-Datei\n");
        exit(1);
    }

    pid_t pid;
    fscanf(pid_file, "%d", &pid);

    fclose(pid_file);

    // Beendet den Daemon-Prozess durch das Senden eines SIGTERM-Signals
    if (kill(pid, SIGTERM) < 0) {
        fprintf(stderr, "Fehler beim Beenden des Daemons\n");
        exit(1);
    }
}

void check_daemon_status() {
    // Liest die PID-Datei des Daemons
    FILE* pid_file = fopen("/var/run/daemon.pid", "r");
    if (!pid_file) {
        fprintf(stderr, "Fehler beim Lesen der PID-Datei\n");
        exit(1);
    }

    pid_t pid;
    fscanf(pid_file, "%d", &pid);

    fclose(pid_file);

    // Überprüft, ob der Daemon-Prozess aktiv ist
    if (kill(pid, 0) == 0) {
        printf("Daemon läuft (PID: %d)\n", pid);
    }
    else {
        printf("Daemon ist nicht aktiv\n");
    }
}

struct ProcessInfo {
    pid_t process_id;
    uid_t process_uid;
    gid_t process_gid;
    unsigned long long memory_usage;
};

struct ProcessInfo get_process_info(pid_t pid) {
    struct ProcessInfo info;

    // Öffnet die statm-Datei des Prozesses, um Speicherinformationen zu lesen
    char statm_path[256];
    sprintf(statm_path, "/proc/%d/statm", pid);
    FILE* statm_file = fopen(statm_path, "r");
    if (!statm_file) {
        fprintf(stderr, "Fehler beim Lesen der Prozessinformationen\n");
        exit(1);
    }

    // Liest die Speicherinformationen aus der Datei
    fscanf(statm_file, "%llu", &info.memory_usage);
    info.memory_usage *= sysconf(_SC_PAGESIZE);

    fclose(statm_file);

    // Öffnet die stat-Datei des Prozesses, um weitere Informationen zu lesen
    char stat_path[256];
    sprintf(stat_path, "/proc/%d/stat", pid);
    FILE* stat_file = fopen(stat_path, "r");
    if (!stat_file) {
        fprintf(stderr, "Fehler beim Lesen der Prozessinformationen\n");
        exit(1);
    }

    // Liest die Prozessinformationen aus der Datei
    fscanf(stat_file, "%d", &info.process_id);
    fscanf(stat_file, "%*s %*c"); // Ignoriert den Prozessnamen
    fscanf(stat_file, "%*c"); // Ignoriert den Status
    fscanf(stat_file, "%*d"); // Ignoriert die Elternprozess-ID
    fscanf(stat_file, "%d %d", &info.process_uid, &info.process_gid);

    fclose(stat_file);

    return info;
}

void run_daemon() {
    // Schreibt die PID des Daemons in die PID-Datei
    FILE* pid_file = fopen("/var/run/daemon.pid", "w");
    if (!pid_file) {
        fprintf(stderr, "Fehler beim Schreiben der PID-Datei\n");
        exit(1);
    }

    fprintf(pid_file, "%d", getpid());
    fclose(pid_file);

    while (1) {
        // Führe hier deine gewünschten Aufgaben aus
        // ...

        syslog(LOG_INFO, "Protokolleintrag des Daemons");

        sleep(5);
    }
}

int main() {
    int daemonStart;
    printf("Moechten Sie ein Daemon starten/n?");
    scanf("%d", &daemonStart);   
    while(1){
        switch (daemonStart)
        {
        case 1:
        start_daemon();
            break;
        case 0:
        printf("Daemon wird nicht gestartet");
        break;
        default:
        printf("falsche Eingabe");
        break;
        }
    }

    

    // Öffnet die syslog-Verbindung zum Protokollieren des Daemons
    openlog("daemon", LOG_PID | LOG_NDELAY, LOG_DAEMON);

    syslog(LOG_INFO, "Daemon gestartet (PID: %d)", getpid());

    run_daemon();

    // Schließt die syslog-Verbindung
    closelog();

    return 0;
}
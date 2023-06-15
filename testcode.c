#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <syslog.h>

void start_daemon()
{
    pid_t pid = fork();
    if (pid < 0)
    {
        fprintf(stderr, "Fehler beim Starten des Daemons\n");
        exit(1);
    }
    else if (pid > 0)
    {
        exit(0);
    }
    umask(0);
    if (setsid() < 0)
    {
        fprintf(stderr, "Fehler beim Erstellen einer neuen Sitzung\n");
        exit(1);
    }
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

void stop_daemon()
{
    int pid_file = open("/var/run/daemon.pid", O_RDONLY);
    if (pid_file < 0)
    {
        fprintf(stderr, "Fehler beim Lesen der PID-Datei\n");
        exit(1);
    }
    pid_t pid;
    read(pid_file, &pid, sizeof(pid_t));
    close(pid_file);

    if (kill(pid, SIGTERM) < 0)
    {
        fprintf(stderr, "Fehler beim Beenden des Daemons\n");
        exit(1);
    }
}

void check_daemon_status()
{
    int pid_file = open("/var/run/daemon.pid", O_RDONLY);
    if (pid_file == -1)
    {
        fprintf(stderr, "Fehler beim Lesen der PID-Datei\n");
        exit(1);
    }
    pid_t pid;
    read(pid_file, &pid, sizeof(pid_t));
    close(pid_file);

    if (kill(pid, 0) == 0)
    {
        printf("Daemon luft (PID: %d)\n", pid);
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

    char statm_path[100];
    sprintf(statm_path, "/proc/%d/statm", pid);
    int statm_file = open(statm_path, O_RDONLY);
    if (statm_file == -1)
    {
        fprintf(stderr, "Fehler beim Lesen der Prozessinformationen\n");
        exit(1);
    }

    char buffer[100];
    read(statm_file, buffer, 100);
    sscanf(buffer, "%llu", &info.memory_usage);
    info.memory_usage *= sysconf(_SC_PAGESIZE);
    close(statm_file);

    char stat_path[100];
    sprintf(stat_path, "/proc/%d/stat", pid);
    int stat_file = open(stat_path, O_RDONLY);
    if (stat_file == -1)
    {
        fprintf(stderr, "Fehler beim Lesen der Prozessinformationen\n");
        exit(1);
    }

    close(stat_file);

    return info;
}
struct process_info
{
    int process_id;
    int process_uid;
    int process_gid;
};

process_info get_process_info()
{
    process_info info;
    std::ifstream stat_file("/proc/self/stat");
    if (!stat_file)
    {
        std::cerr << "Fehler beim Öffnen der Statistik-Datei" << std::endl;
        exit(1);
    }
    stat_file >> info.process_id;
    stat_file.ignore(256, ' ');
    stat_file.ignore(256, ' ');
    stat_file.ignore(256, ' ');
    stat_file >> info.process_uid >> info.process_gid;
    stat_file.close();
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
    fprintf(pid_file, "%d", getpid());
    fclose(pid_file);
    while (1)
    {
        syslog(LOG_INFO, "Protokolleintrag des Daemons");
        sleep(5);
    }
}

int main()
{
    start_daemon();

    openlog("daemon", LOG_PID | LOG_NDELAY, LOG_DAEMON);
    syslog(LOG_INFO, "Daemon gestartet (PID: %d)", getpid());
    run_daemon();

    closelog();
    return 0;
}


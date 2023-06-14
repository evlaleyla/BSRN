#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <csignal>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <ctime>
#include <syslog.h>

void start_daemon() {
    // Erzeugt einen neuen Prozess (Daemon-Prozess)
    pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "Fehler beim Starten des Daemons" << std::endl;
        exit(1);
    }
    else if (pid > 0) {
        exit(0); // Elternprozess beendet sich
    }
    
    umask(0); // Setzt die Zugriffsrechte für Dateiberechtigungen
    
    if (setsid() < 0) {
        std::cerr << "Fehler beim Erstellen einer neuen Sitzung" << std::endl;
        exit(1);
    }
    
    close(STDIN_FILENO); // Schließt den Standard-Eingabestrom
    close(STDOUT_FILENO); // Schließt den Standard-Ausgabestrom
    close(STDERR_FILENO); // Schließt den Standard-Fehlerstrom
}

void stop_daemon() {
    // Liest die PID-Datei des Daemons
    std::ifstream pid_file("/var/run/daemon.pid");
    if (!pid_file) {
        std::cerr << "Fehler beim Lesen der PID-Datei" << std::endl;
        exit(1);
    }
    
    pid_t pid;
    pid_file >> pid;
    
    pid_file.close();
    
    // Beendet den Daemon-Prozess durch das Senden eines SIGTERM-Signals
    if (kill(pid, SIGTERM) < 0) {
        std::cerr << "Fehler beim Beenden des Daemons" << std::endl;
        exit(1);
    }
}

void check_daemon_status() {
    // Liest die PID-Datei des Daemons
    std::ifstream pid_file("/var/run/daemon.pid");
    if (!pid_file) {
        std::cerr << "Fehler beim Lesen der PID-Datei" << std::endl;
        exit(1);
    }
    
    pid_t pid;
    pid_file >> pid;
    
    pid_file.close();
    
    // Überprüft, ob der Daemon-Prozess aktiv ist
    if (kill(pid, 0) == 0) {
        std::cout << "Daemon läuft (PID: " << pid << ")" << std::endl;
    }
    else {
        std::cout << "Daemon ist nicht aktiv" << std::endl;
    }
}

struct ProcessInfo {
    pid_t process_id;
    uid_t process_uid;
    gid_t process_gid;
    unsigned long long memory_usage;
};

ProcessInfo get_process_info(pid_t pid) {
    ProcessInfo info;
    
    // Öffnet die statm-Datei des Prozesses, um Speicherinformationen zu lesen
    std::stringstream statm_path;
    statm_path << "/proc/" << pid << "/statm";
    std::ifstream statm_file(statm_path.str().c_str());
    if (!statm_file) {
        std::cerr << "Fehler beim Lesen der Prozessinformationen" << std::endl;
        exit(1);
    }
    
    // Liest die Speicherinformationen aus der Datei
    statm_file >> info.memory_usage;
    info.memory_usage *= sysconf(_SC_PAGESIZE);
    
    statm_file.close();
    
    // Öffnet die stat-Datei des Prozesses, um weitere Informationen zu lesen
    std::stringstream stat_path;
    stat_path << "/proc/" << pid << "/stat";
    std::ifstream stat_file(stat_path.str().c_str());
    if (!stat_file) {
        std::cerr << "Fehler beim Lesen der Prozessinformationen" << std::endl;
        exit(1);
    }
    
    // Liest die Prozessinformationen aus der Datei
    stat_file >> info.process_id;
    stat_file.ignore(256, ' '); // Ignoriert den Prozessnamen
    stat_file.ignore(256, ' '); // Ignoriert den Status
    stat_file.ignore(256, ' '); // Ignoriert die Elternprozess-ID
    stat_file >> info.process_uid >> info.process_gid;
    
    stat_file.close();
    
    return info;
}

void run_daemon() {
    // Schreibt die PID des Daemons in die PID-Datei
    std::ofstream pid_file("/var/run/daemon.pid");
    if (!pid_file) {
        std::cerr << "Fehler beim Schreiben der PID-Datei" << std::endl;
        exit(1);
    }
    
    pid_file << getpid();
    pid_file.close();
    
    while (true) {
        // Führe hier deine gewünschten Aufgaben aus
        // ...
        
        syslog(LOG_INFO, "Protokolleintrag des Daemons");
        
        sleep(5);
    }
}

int main() {
    start_daemon();
    
    // Öffnet die syslog-Verbindung zum Protokollieren des Daemons
    openlog("daemon", LOG_PID | LOG_NDELAY, LOG_DAEMON);
    
    syslog(LOG_INFO, "Daemon gestartet (PID: %d)", getpid());
    
    run_daemon();
    
    // Schließt die syslog-Verbindung
    closelog();
    
    return 0;
}


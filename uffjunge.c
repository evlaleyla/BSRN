
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
#include <sys/sysinfo.h>

void start_daemon()
{
    int session;
    pid_t pid = fork(); // Erstelle einen Kindprozess
    
    if (pid < 0)
    {
        perror("Fehler beim Starten des Daemons\n");
        exit(1);
    }
    

    if (pid == 0)
    {
        session = setsid();
        

        if (session < 0)
        {
            perror("Fehler beim Erstellen einer neuen Sitzung\n");
            exit(1);
        }
        if (session >= 0)
        {
            printf("Neue Session wurde erstellt. SessionnID %d\n", session);
        }
               char *arguments[] = {"/home/safashg/Schreibtisch/Projekt/funktionierdochendlich.c", NULL};
       
        if (execvp("/home/safashg/Schreibtisch/Projekt/helloworld", arguments) < 0){
          perror("Fehler beim Ausführen des Programms\n");
          exit(1);
       }


    }

    signal(SIGHUP, SIG_IGN); // Ignoriere Sighup
    
    // Verzeichniswechsel
    if (chdir("/") < 0)
    {
        perror("Fehler beim Verzeichniswechsel\n");
        exit(1);
    }

    // Überprüfen Sie, ob das Programm bereits mit Superuser-Rechten ausgeführt wird
    if (getuid() == 0)
    {
        printf("Das Programm wird bereits mit Superuser-Rechten ausgeführt.\n");
    }
    if (setuid(0) == 0)
    {
        printf("Superuser-Rechte erfolgreich erhalten.\n");
    }

    umask(0); // Setze die Zugriffsrechte für Dateien


    time_t rawtime;
    struct tm *timeinfo;
    char buffer[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    printf("Daemon wurde gestartet um: %s\n", buffer);

     syslog(LOG_INFO, "Daemon wurde gestartet um: %s\n", buffer );
}


void create_pid_file()
{
    FILE *pid_file = fopen("/home/safashg/Schreibtisch/Projekt/log.txt", "w");
    if (!pid_file)
    {
        perror("Fehler beim Erstellen der PID-Datei\n");
        exit(1);
    } else {
    fprintf(pid_file, "%d", getpid()); // Schreibe die PID des aktuellen Prozesses in die Datei
    
    }
    fclose(pid_file);
syslog(LOG_INFO, "Datei wurde erstellt.");


}

void check_daemon_status()
{
    FILE *pid_file = fopen("/home/safashg/Schreibtisch/Projekt/log.txt", "r");
    if (!pid_file)
    {
        perror("Fehler beim Lesen der PID-Datei\n");
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
        exit(1);
    }
}

struct ProzessInfo
{
    int prozess_id;
    int prozess_uid;
    int prozess_gid;
    unsigned long long speichernutzung;
    mode_t prozess_rechte; // Neue Ergänzung für die Prozessrechte
};
struct ProzessInfo get_process_info(pid_t pid)
{

    struct ProzessInfo info;
    char statm_path[256];
    snprintf(statm_path, sizeof(statm_path), "/proc/%d/statm", pid);
    FILE *statm_file = fopen(statm_path, "r");
    
    if (!statm_file)
    {
        perror("Fehler beim Lesen der Prozessinformationen\n");
        exit(1);
    }

    fscanf(statm_file, "%llu", &info.speichernutzung);
    info.speichernutzung *= sysconf(_SC_PAGESIZE);
    fclose(statm_file);
   
    char stat_path[256];
    snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", pid);
    FILE *stat_file = fopen(stat_path, "r");
    
    if (!stat_file)
    {
        perror("Fehler beim Lesen der Prozessinformationen\n");
        exit(1);
    }

    fscanf(stat_file, "%d", &info.prozess_id);
    fseek(stat_file, 256, SEEK_CUR);
    fscanf(stat_file, "%d %d", &info.prozess_uid, &info.prozess_gid);
    fseek(stat_file, 10, SEEK_CUR);                // Überspringe die nächsten 10 Felder im stat-Datei-Format
    fscanf(stat_file, "%o", &info.prozess_rechte); // Lese die Prozessrechte im oktalen Format ein
    fclose(stat_file);
    return info;
}

void ausgabe(){
   
   
    check_daemon_status();
    struct ProzessInfo info = get_process_info(getpid());

    struct sysinfo sys_info;
    
    if (sysinfo(&sys_info) != 0) {
        printf("Fehler beim Abrufen der Systeminformationen.\n");
        exit(1);
    }
    
    unsigned long total_ram = sys_info.totalram * sys_info.mem_unit;
    unsigned long free_ram = sys_info.freeram * sys_info.mem_unit;
    unsigned long used_ram = total_ram - free_ram;
    
    
    pid_t pid = getpid();
    uid_t uid = getuid();
    gid_t gid = getgid();
    
    FILE *pid_file = fopen("/home/safashg/Schreibtisch/Projekt/log.txt", "w");

    fprintf(pid_file, "Prozess ID2: %d\n", pid);
    fprintf(pid_file, "Benutzer ID2: %d\n", uid);
    fprintf(pid_file, "GruppenID2: %d\n", gid);
    fprintf(pid_file,"Speichernutzung: %llu Bytes\n", info.speichernutzung); // Schreibe die SPEICHERNUTZUNG des aktuellen Prozesses in die Datei
    fprintf(pid_file,"Prozessrechte: %o\n", info.prozess_rechte); // Schreibe die RECHTE des aktuellen Prozesses in die Datei
    fprintf(pid_file, "gesamter RAM: %lu bytes\n", total_ram);
    fprintf(pid_file, "verfügbarer RAM: %lu bytes\n", free_ram);
    fprintf(pid_file, "Verwendeter RAM: %lu bytes\n", used_ram);

    syslog(LOG_INFO, "\nDaemon gestartet (PID: %d)", getpid()); // Protokolliere eine Nachricht

     fclose(pid_file); 

      int daemonInformationen;

    printf("Möchten Sie Informationen erhalten? Wenn ja, dann tippe 1. wenn nein dann 0\n");
        scanf("%d", &daemonInformationen);

        
        
        if (daemonInformationen == 1)
        {
            FILE *pid_file = fopen("/home/safashg/Schreibtisch/Projekt/log.txt", "r");
            if (!pid_file){
                printf("Fehler beim Öffnen der Datei.\n");
                exit(1);
            }
            char puffer[256]; //angenommene maxiale zeilenlänge

            while (fgets(puffer,sizeof(puffer), pid_file) != NULL){
                printf("%s", puffer);
            }
            fclose(pid_file);

    
        } else if(daemonInformationen == 0){
            printf("informationen wurden gespeichert, werden aber nicht ausgegeben.\n");
        }
}

void run_daemon()
{  
    printf("hallo"); 
    int running = 1;
    int daemonBeenden;
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
   
   
    while (running)
    {
        printf("1");
        syslog(LOG_INFO, "Daemon läuft...");
        syslog(LOG_ERR, "Fehler beim Verarbeiten der Anfrage.");
        sleep(5); // Warte für 5 Sekunden
        syslog(LOG_INFO, "Daemon schläft...");
        

        // Überprüfe, ob der Daemon beendet werden soll
        printf("Moechten Sie den Daemon beenden? wenn ja dann 1");
        scanf("%d", &daemonBeenden);
        if (daemonBeenden == 1)
        {
            printf("2");
            printf("Daemon wird gestoppt..");
           printf("3");
    
    FILE *pid_file = fopen("/home/safashg/Schreibtisch/Projekt/log.txt", "r");
   
   if(!pid_file){
    perror("Fehler beim lesen der PID-Datei\n");
    exit(1);
   }

    printf("4");
    pid_t pid;
    fscanf(pid_file, "%d", &pid);
    fclose(pid_file);
    
    if (kill(pid, SIGTERM) == 0)
    {
        printf("Daemon wurde beendet um: %s\n", buffer);
         syslog(LOG_INFO, "Daemon wurde beendet um: %s\n", buffer );
        running = 0;
        return;
   
    }else {
    perror("Fehler beim Beenden des Daemons\n");
        exit(1);
    }       
        } else if(daemonBeenden == 0){
            printf("daemon läuft weiter");
        }
        }
    }

int main()
{

 
    

    int daemonStart;
    int daemonInformationen;

    FILE*logFile;
    logFile = fopen("logdatei.log", "a");


    setlogmask(LOG_UPTO(LOG_NOTICE));
    openlog("daemons", LOG_PID, LOG_DAEMON); // Öffne das Syslog für den Daemon-Prozess
   
   /* if(syslog(LOG_ERR, "Fehler beim öffnen der Datei") < 0){
        perror("Fehler beim protokollieren der Meldung\n");
        exit(1);
    }*/
    printf("Wollen Sie einen Daemon starten?\nGeben Sie 1 ein, damit ein Daemon gestartet wird.\nGeben Sie 0 ein damit kein Daemon gestartet wird.");
    scanf("%d", &daemonStart);
    switch (daemonStart)
    {
    case 1:
        create_pid_file();
        start_daemon();
        printf("Daemon gestartet.\n");
        ausgabe();
        run_daemon();
      
        break;
    case 0:
        printf("Eingabe gespeichert. Daemon wird nicht gestartet.");
        break;
    default:
        printf("Falsche Eingabe");
        break;
    }
    closelog(); // Schließe den syslog
    return 0;
}

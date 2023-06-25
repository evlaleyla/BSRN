#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <sys/sysinfo.h>

void starteDaemon()    //Daemon wird gestartet
{
    int session;
    pid_t pid = fork(); //Erstelle einen Kindprozess

    if (pid < 0)    //Fehler beim Erstellen des Kindprozesses
    {
        perror("Fehler beim Starten des Daemons\n");
        FILE *log_file = fopen("/home/evlaleyla/Schreibtisch/BSRN Projekt/logDatei.log", "a");
        fprintf(log_file, "Fehler beim Starten des Daemons\n");
        fclose(log_file);
        exit(1);
    }

    if (pid == 0)   //Kindprozess
    {
        session = setsid(); // Erstelle eine neue Sitzung.

        if (session < 0)    //Fehler beim Erstellen einer Sitzung
        {
            perror("Fehler beim Erstellen einer neuen Sitzung\n");
            FILE *log_file = fopen("/home/evlaleyla/Schreibtisch/BSRN Projekt/logDatei.log", "a");
            fprintf(log_file, "Fehler beim Erstellen einer neuen Sitzung\n");
            fclose(log_file);
            exit(1);
        }
        if (session >= 0)   //Neue Sitzung wurde erstellt. Kindprozess ist Sitzungsführer
        {
            printf("Neue Session wurde erstellt. Die Session-ID %d\n", session);
            FILE *log_file = fopen("/home/evlaleyla/Schreibtisch/BSRN Projekt/logDatei.log", "a");
            fprintf(log_file, "Neue Session wurde erstellt\n");
            fclose(log_file);
        }
        char *arguments[] = {"/home/evlaleyla/Schreibtisch/BSRN Projekt/sonntag.c", NULL};

        if (execvp("/home/evlaleyla/Schreibtisch/BSRN Projekt/helloworld", arguments) < 0) // Verkettung von Prozessen mit exec()
        {
            perror("Fehler beim Ausführen des Programms\n");
            FILE *log_file = fopen("/home/evlaleyla/Schreibtisch/BSRN Projekt/logDatei.log", "a");
            fprintf(log_file, "Fehler beim Ausführen des Programms\n");
            fclose(log_file);
            exit(1);
        }
    }

    signal(SIGHUP, SIG_IGN); // Ignoriere Sighup

    // Verzeichniswechsel
    if (chdir("/") < 0)
    {
        perror("Fehler beim Verzeichniswechsel\n");
        FILE *log_file = fopen("/home/evlaleyla/Schreibtisch/BSRN Projekt/logDatei.log", "a");
        fprintf(log_file, "Fehler beim Verzeichniswechsel\n");
        fclose(log_file);
        exit(1);
    }

    umask(0); // Setze die Zugriffsrechte für Dateien

    // Startzeit ermitteln
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
    printf("Daemon wurde gestartet um: %s\n", buffer);
    FILE *log_file = fopen("/home/evlaleyla/Schreibtisch/BSRN Projekt/logDatei.log", "a");
    fprintf(log_file, "Daemon wurde gestartet um: %s\n", buffer);
    fclose(log_file);
}

void ueberpruefeDateiVorhanden()    //PID-Datei wird ueberprueft
{
    FILE *pid_file = fopen("/home/evlaleyla/Schreibtisch/BSRN Projekt/pidDatei.txt", "w");
    if (!pid_file)
    {
        perror("Fehler beim Finden der PID-Datei\n");
        FILE *log_file = fopen("/home/evlaleyla/Schreibtisch/BSRN Projekt/logDatei.log", "a");
        fprintf(log_file, "Fehler beim Finden der PID-Datei\n");
        fclose(log_file);
        exit(1);
    }
    else
    {
        fprintf(pid_file, "%d", getpid()); // Schreibe die PID des aktuellen Prozesses in die Datei
    }
    fclose(pid_file);
    FILE *log_file = fopen("/home/evlaleyla/Schreibtisch/BSRN Projekt/logDatei.log", "a");
    fprintf(log_file, "\nDatei ist vorhanden.\n");
    fclose(log_file);
}
void speichereProzessinformationen()
{
    pid_t pid = getpid(); // Prozess ID
    uid_t uid = getuid(); // Benutzer ID
    gid_t gid = getgid(); // Gruppen ID

    mode_t mode = umask(0);
    umask(mode);

    // CPU-Zeit
    clock_t cpu_zeit = clock();
    double cpu_seconds = (double)cpu_zeit / CLOCKS_PER_SEC;
    char cpu_buffer[64];
    snprintf(cpu_buffer, sizeof(cpu_buffer), "CPU-Zeit: %.2f Sekunden\n", cpu_seconds); //CPU-Zeit wird in cpu_buffer geschrieben

    // RAM-Ausnutzung
    char statm_path[64];
    sprintf(statm_path, "/proc/%d/statm", pid); //Pfad wird in Puffer geschrieben

    int statm_fd = open(statm_path, O_RDONLY); //Pufferdatei wird im Lesemodus geöffnet
    if (statm_fd != -1)
    {
        char buffer[256];
        ssize_t bytesRead = read(statm_fd, buffer, sizeof(buffer) - 1);  //liest den Inhalt der göffneten Datei in Puffer "buffer"
        close(statm_fd);

        if (bytesRead > 0)
        {
            buffer[bytesRead] = '\0';   //Überprüfung, ob der Puffer als Zeichenkette verwendet werden kann
            unsigned long size, resident, share, text, lib, data, dt;
            sscanf(buffer, "%lu %lu %lu %lu %lu %lu %lu", &size, &resident, &share, &text, &lib, &data, &dt); //Extraktion der Daten in verschiedene Variablen

            // Schreiben der Informationen in die pidDatei.txt Datei
            FILE *pid_file = fopen("/home/evlaleyla/Schreibtisch/BSRN Projekt/pidDatei.txt", "w");
            if (pid_file != NULL)
            {
                fprintf(pid_file, "Prozess ID2: %d\n", pid);
                fprintf(pid_file, "Benutzer ID2: %d\n", uid);
                fprintf(pid_file, "GruppenID2: %d\n", gid);
                fprintf(pid_file, "Prozessrechte: %o\n", mode);
                fprintf(pid_file, "RAM Ausnutzung:\n");
                fprintf(pid_file, "Gesamtgröße der RAM Ausnutzung: %.2f MB\n", (float)size / 1024);
                fprintf(pid_file, "Resident: %.2f MB\n", (float)resident / 1024);
                fprintf(pid_file, "Shared: %.2f MB\n", (float)share / 1024);
                fprintf(pid_file, "Text: %.2f MB\n", (float)text / 1024);
                fprintf(pid_file, "Bibliotheken: %.2f MB\n", (float)lib / 1024);
                fprintf(pid_file, "Daten: %.2f MB\n", (float)data / 1024);
                fprintf(pid_file, "Datenseiten (Dirty): %.2f MB\n", (float)dt / 1024);
                fprintf(pid_file, "CPU-Zeit: %s", cpu_buffer);
                fclose(pid_file);
            }
            else
            {
                printf("Fehler beim Öffnen der Ausgabedatei.\n");
            }
        }
    }
    else
    {
        printf("Fehler beim Lesen der RAM-Ausnutzung.\n");
    }
    FILE *log_file = fopen("/home/evlaleyla/Schreibtisch/BSRN Projekt/logDatei.log", "a");
    fprintf(log_file, "Daemon gestartet (PID: %d)\n", getpid());
    fclose(log_file);
}

void ausgebenDerInfos()
{
    //ueberpruefeDaemonStatus();
    int daemonInformationen;

    printf("Möchten Sie Informationen erhalten? Wenn Ja, dann tippen Sie bitte '1', wenn Nein, dann tippen Sie '0'.\n");
    scanf("%d", &daemonInformationen);

    if (daemonInformationen == 1)
    {
        FILE *pid_file = fopen("/home/evlaleyla/Schreibtisch/BSRN Projekt/pidDatei.txt", "r");
        if (!pid_file)
        {
            printf("Fehler beim Öffnen der Datei.\n");
            FILE *log_file = fopen("/home/evlaleyla/Schreibtisch/BSRN Projekt/logDatei.log", "a");
            fprintf(log_file, "Fehler beim Öffnen der Datei.\n");
            fclose(log_file);
            exit(1);
        }
        else
        {
            char puffer[256]; // angenommene maximale Zeilenlänge

            while (fgets(puffer, sizeof(puffer), pid_file) != NULL)  //Zeilen aus der Datei pid_file werden in puffer gelesen
            {
                printf("%s", puffer);  //Ausgabe des Puffers in der Konsole
            }
        }
        fclose(pid_file);
    }
    else if (daemonInformationen == 0)
    {
        printf("Die Informationen wurden gespeichert, Sie werden aber nicht ausgegeben.\n");
    }
}

void daemonAusfuehren()
{
    int running = 1;
    int daemonBeenden;
    while (running)
    {
        // Daemon protokolliert in logDatei.log
        FILE *log_file = fopen("/home/evlaleyla/Schreibtisch/BSRN Projekt/logDatei.log", "a");
        fprintf(log_file, "Der Daemon laeuft orndungsgemaeß weiter.\n");
        fprintf(log_file, "Daemon schlaeft...\n");
        fclose(log_file);
        printf("Warten Sie einen kurzen Moment..\n");
        sleep(5); // Warte für 5 Sekunden

        // Überprüfung, ob der Daemon beendet werden soll
        printf("Wenn Sie moechte dass der Daemon weiterlaeuft, dann druecken Sie bitte die \"1\". \nWenn Sie moechten dass der Daemon beendet wird, dann geben sie im Terminal bitte \"kill -15 <pid>\" ein.\n");
        scanf("%d", &daemonBeenden);
        if (daemonBeenden == 1)
        {
            running = 1;  //Schleife wird erneut durchlaufen, Daemon läuft weiter
        }
        else
        {
            printf("Falsche Einfabe.\n");
        }
    }
}

int main()
{
    int daemonStart;
    int daemonInformationen;

    // Menue
    printf("Wollen Sie einen Daemon starten?\nGeben Sie bitte \"1\" ein, damit ein Daemon gestartet wird.\nGeben Sie bitte \"0\" ein, damit kein Daemon gestartet wird.");
    scanf("%d", &daemonStart);
    switch (daemonStart)
    {
    case 1:
        starteDaemon();
        ueberpruefeDateiVorhanden();
        speichereProzessinformationen();
        ausgebenDerInfos();
        daemonAusfuehren();
        break;
    case 0:
        printf("Eingabe gespeichert. Der Daemon wird nicht gestartet.\n");
        break;
    default:
        printf("Falsche Eingabe");
        break;
    }
    return 0;
}
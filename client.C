//============================================================================
// Name        : INF3_Prak.cpp
// Beschreibung: Client für Schiffe versenken (Einfache Version)
//============================================================================

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>     // für rand()
#include <ctime>       // für time()
#include <cstring>     // für memset()
#include <cstdio>      // für sscanf, printf
#include <unistd.h>    // für close()
#include <arpa/inet.h> // für Netzwerkverbindung
#include <sys/socket.h>
#include <algorithm>   // für random_shuffle
#include <fstream>     // für Datei speichern (CSV)

using namespace std;

// =============================================================
// 1. KLASSE FÜR DIE VERBINDUNG (Ganz simpel gehalten)
// =============================================================
class SimpleClient {
private:
    int sock;
    struct sockaddr_in serverAdresse;

public:
    SimpleClient() {
        sock = -1;
    }

    // Verbindung zum Server herstellen
    bool verbinden(string ip, int port) {
        // Socket erstellen
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1) {
            return false;
        }

        serverAdresse.sin_addr.s_addr = inet_addr(ip.c_str());
        serverAdresse.sin_family = AF_INET;
        serverAdresse.sin_port = htons(port);

        // Verbinden
        if (connect(sock, (struct sockaddr *)&serverAdresse, sizeof(serverAdresse)) < 0) {
            return false;
        }
        return true;
    }

    // Nachricht senden
    void senden(string nachricht) {
        send(sock, nachricht.c_str(), nachricht.size(), 0);
    }

    // Nachricht empfangen
    string empfangen() {
        char buffer[1024];
        memset(buffer, 0, 1024); // Buffer leeren
        int anzahlBytes = recv(sock, buffer, 1024, 0);
        if (anzahlBytes < 0) {
            return "";
        }
        return string(buffer);
    }

    // Verbindung schließen (wichtig für sauberes Beenden)
    void trennen() {
        close(sock);
    }
};

// =============================================================
// 2. HILFSFUNKTIONEN
// =============================================================

// Eine kleine Struktur für Koordinaten (x und y)
struct Punkt {
    int x;
    int y;
};

// Diese Funktion führt EINEN Schuss aus und gibt die Antwort des Servers zurück
// Sie baut den String "COORD[x;y]" zusammen.
string schiesseAuf(SimpleClient &client, int x, int y) {
    string befehl = "COORD[" + to_string(x) + ";" + to_string(y) + "]";
    client.senden(befehl);
    string antwort = client.empfangen();
    return antwort;
}

// =============================================================
// 3. DIE STRATEGIEN
// =============================================================

// Strategie 1: Einfach Zeile für Zeile durchgehen
int strategie1_Zeilenweise(SimpleClient &client) {
    int versuche = 0;

    // Y von 1 bis 10 (Zeilen)
    for (int y = 1; y <= 10; y++) {
        // X von 1 bis 10 (Spalten)
        for (int x = 1; x <= 10; x++) {
            versuche++;
            string antwort = schiesseAuf(client, x, y);

            // Wenn das Spiel vorbei ist, sofort aufhören
            if (antwort.find("GAME_OVER") != string::npos) {
                return versuche;
            }
        }
    }
    return versuche;
}

// Strategie 2: Schachbrettmuster (Erst alle schwarzen Felder, dann alle weißen)
int strategie2_Schachbrett(SimpleClient &client) {
    int versuche = 0;

    // RUNDE 1: Nur Felder, wo (x+y) gerade ist
    for (int y = 1; y <= 10; y++) {
        for (int x = 1; x <= 10; x++) {
            if ((x + y) % 2 == 0) {
                versuche++;
                string antwort = schiesseAuf(client, x, y);
                if (antwort.find("GAME_OVER") != string::npos) return versuche;
            }
        }
    }

    // RUNDE 2: Der Rest (ungerade Summe)
    for (int y = 1; y <= 10; y++) {
        for (int x = 1; x <= 10; x++) {
            if ((x + y) % 2 != 0) {
                versuche++;
                string antwort = schiesseAuf(client, x, y);
                if (antwort.find("GAME_OVER") != string::npos) return versuche;
            }
        }
    }
    return versuche;
}

// Strategie 3: Zufall (Wir erstellen eine Liste aller Felder und mischen sie)
int strategie3_Zufall(SimpleClient &client) {
    int versuche = 0;
    vector<Punkt> alleFelder;

    // Liste füllen mit allen 100 Koordinaten
    for (int x = 1; x <= 10; x++) {
        for (int y = 1; y <= 10; y++) {
            Punkt p;
            p.x = x;
            p.y = y;
            alleFelder.push_back(p);
        }
    }

    // Die Liste mischen (Shuffle)
    random_shuffle(alleFelder.begin(), alleFelder.end());

    // Die gemischte Liste abarbeiten
    for (size_t i = 0; i < alleFelder.size(); i++) {
        versuche++;
        string antwort = schiesseAuf(client, alleFelder[i].x, alleFelder[i].y);
        if (antwort.find("GAME_OVER") != string::npos) return versuche;
    }
    return versuche;
}

// Strategie 4: Zufall + "Search and Destroy" (Die Intelligente)
int strategie4_Smart(SimpleClient &client) {
    int versuche = 0;

    // Wir merken uns, wo wir schon geschossen haben, damit wir nichts doppelt machen
    bool besucht[12][12];
    for(int i=0; i<12; i++) for(int j=0; j<12; j++) besucht[i][j] = false;

    // Liste für Zufallsschüsse vorbereiten
    vector<Punkt> zufallsListe;
    for (int x = 1; x <= 10; x++) {
        for (int y = 1; y <= 10; y++) {
            Punkt p = {x, y};
            zufallsListe.push_back(p);
        }
    }
    random_shuffle(zufallsListe.begin(), zufallsListe.end());
    int zufallsIndex = 0;

    // Liste für Ziele, wenn wir ein Schiff getroffen haben (Stapel)
    vector<Punkt> zielListe;

    while (true) {
        Punkt aktuellesZiel;
        bool habeZiel = false;

        // SCHRITT 1: Haben wir ein konkretes Ziel (weil wir ein Schiff getroffen haben)?
        if (zielListe.size() > 0) {
            aktuellesZiel = zielListe.back(); // Letztes Element nehmen
            zielListe.pop_back();             // Und aus der Liste löschen
            habeZiel = true;
        }
        // SCHRITT 2: Kein Ziel? Dann nimm einen Zufallspunkt
        else {
            if (zufallsIndex < zufallsListe.size()) {
                aktuellesZiel = zufallsListe[zufallsIndex];
                zufallsIndex++;
                habeZiel = true;
            }
        }

        // Sicherheitscheck: Koordinaten gültig? Schon besucht?
        if (aktuellesZiel.x < 1 || aktuellesZiel.x > 10 ||
            aktuellesZiel.y < 1 || aktuellesZiel.y > 10 ||
            besucht[aktuellesZiel.x][aktuellesZiel.y] == true) {
            continue; // Überspringen und Schleife neu starten
        }

        // SCHRITT 3: Schießen!
        besucht[aktuellesZiel.x][aktuellesZiel.y] = true; // Als besucht markieren
        versuche++;

        string antwort = schiesseAuf(client, aktuellesZiel.x, aktuellesZiel.y);

        // Prüfen was passiert ist
        if (antwort.find("GAME_OVER") != string::npos) {
            return versuche; // Fertig!
        }

        // Wenn Treffer (HIT) oder Versenkt (DESTROYED), Nachbarn zur ZielListe hinzufügen
        if (antwort.find("HIT") != string::npos || antwort.find("SHIP_DESTROYED") != string::npos) {
            // Nachbar Rechts
            zielListe.push_back({aktuellesZiel.x + 1, aktuellesZiel.y});
            // Nachbar Links
            zielListe.push_back({aktuellesZiel.x - 1, aktuellesZiel.y});
            // Nachbar Unten
            zielListe.push_back({aktuellesZiel.x, aktuellesZiel.y + 1});
            // Nachbar Oben
            zielListe.push_back({aktuellesZiel.x, aktuellesZiel.y - 1});
        }
    }
    return versuche;
}

// =============================================================
// 4. MAIN PROGRAMM
// =============================================================
int main() {
    srand(time(NULL)); // Zufallsgenerator starten

    SimpleClient meinClient;

    cout << "--- Client gestartet ---" << endl;
    cout << "Verbinde zu 127.0.0.1 auf Port 2000..." << endl;

    // Versuche zu verbinden
    if (meinClient.verbinden("127.0.0.1", 2000)) {
        cout << "Connected" << endl; // Genau wie im Protokoll gefordert
    } else {
        cout << "Fehler: Konnte Server nicht finden. (Läuft ./server ?)" << endl;
        return 1;
    }

    // Strategie Auswahl
    int wahl = 0;
    cout << "\nWelche Strategie soll getestet werden?" << endl;
    cout << "1: Zeilenweise" << endl;
    cout << "2: Schachbrett" << endl;
    cout << "3: Zufall" << endl;
    cout << "4: Zufall + Smart (Search & Destroy)" << endl;
    cout << "Eingabe: ";
    cin >> wahl;

    // Vektor um Ergebnisse zu speichern
    vector<int> ergebnisse;
    int anzahlSpiele = 1000; // Vorgabe aus Protokoll [cite: 52]

    cout << "Starte " << anzahlSpiele << " Spiele..." << endl;

    for (int i = 0; i < anzahlSpiele; i++) {

        // Neues Spiel anfordern beim Server
        meinClient.senden("NEWGAME");
        string antwort = meinClient.empfangen(); // Server sagt "NEWGAME" zurück

        int versuche = 0;

        // Gewählte Strategie ausführen
        if (wahl == 1) versuche = strategie1_Zeilenweise(meinClient);
        else if (wahl == 2) versuche = strategie2_Schachbrett(meinClient);
        else if (wahl == 3) versuche = strategie3_Zufall(meinClient);
        else if (wahl == 4) versuche = strategie4_Smart(meinClient);
        else versuche = strategie1_Zeilenweise(meinClient); // Default

        // Ergebnis speichern und anzeigen
        ergebnisse.push_back(versuche);
        cout << "Game " << (i + 1) << ": needed " << versuche << " attempts" << endl; // Ausgabe laut Protokoll [cite: 139]
    }

    // Statistik in Datei speichern (CSV)
    ofstream datei("StatisticalData.csv"); // Dateiname laut Protokoll [cite: 53]
    if (datei.is_open()) {
        datei << "Spiel;Versuche\n";
        for (size_t i = 0; i < ergebnisse.size(); i++) {
            datei << (i + 1) << ";" << ergebnisse[i] << "\n";
        }
        datei.close();
        cout << "Statistical Data exported" << endl; // Ausgabe laut Protokoll [cite: 140]
    } else {
        cout << "Fehler beim Speichern der Datei!" << endl;
    }

    meinClient.trennen();
    return 0;
}

/*
 * client.C
 * Einfacher Client für Schiffe versenken
 * Passt genau zu deiner server.C (Port 2025)
 */

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
#include <fstream>     // für Datei speichern

using namespace std;

// ------------------------------------------------------------------
// 1. NETZWERK-HELFER (Damit main() sauber bleibt)
// ------------------------------------------------------------------
class SimpleClient {
public:
    int sock;
    struct sockaddr_in serverAdresse;

    // Verbinden
    bool verbinden(string ip, int port) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1) return false;

        serverAdresse.sin_addr.s_addr = inet_addr(ip.c_str());
        serverAdresse.sin_family = AF_INET;
        serverAdresse.sin_port = htons(port);

        if (connect(sock, (struct sockaddr *)&serverAdresse, sizeof(serverAdresse)) < 0) {
            return false;
        }
        return true;
    }

    // Senden
    void senden(string text) {
        send(sock, text.c_str(), text.size(), 0);
    }

    // Empfangen
    string empfangen() {
        char buffer[1024];
        memset(buffer, 0, 1024);
        recv(sock, buffer, 1024, 0);
        return string(buffer);
    }

    // Trennen
    void trennen() {
        close(sock);
    }
};

// Eine Koordinate
struct Punkt { int x; int y; };

// ------------------------------------------------------------------
// 2. SCHUSS-FUNKTION
// ------------------------------------------------------------------
// Baut den Befehl genau so, wie dein Server ihn will: SHOT[x,y]
string schuss(SimpleClient &client, int x, int y) {
    // WICHTIG: Dein Server nutzt ein KOMMA! SHOT[1,1]
    string befehl = "SHOT[" + to_string(x) + "," + to_string(y) + "]";
    client.senden(befehl);
    return client.empfangen();
}

// ------------------------------------------------------------------
// 3. STRATEGIEN
// ------------------------------------------------------------------

// Strategie 1: Zeile für Zeile (1,1 -> 1,2 -> ...)
int strat_Zeilen(SimpleClient &client) {
    int versuche = 0;
    for (int y = 1; y <= 10; y++) {
        for (int x = 1; x <= 10; x++) {
            versuche++;
            string antwort = schuss(client, x, y);
            if (antwort.find("GAME_OVER") != string::npos) return versuche;
        }
    }
    return versuche;
}

// Strategie 2: Schachbrett (Erst schwarze Felder, dann weiße)
int strat_Schachbrett(SimpleClient &client) {
    int versuche = 0;
    // Durchgang 1: Gerade Summe
    for (int y = 1; y <= 10; y++) {
        for (int x = 1; x <= 10; x++) {
            if ((x + y) % 2 == 0) {
                versuche++;
                if (schuss(client, x, y).find("GAME_OVER") != string::npos) return versuche;
            }
        }
    }
    // Durchgang 2: Ungerade Summe
    for (int y = 1; y <= 10; y++) {
        for (int x = 1; x <= 10; x++) {
            if ((x + y) % 2 != 0) {
                versuche++;
                if (schuss(client, x, y).find("GAME_OVER") != string::npos) return versuche;
            }
        }
    }
    return versuche;
}

// Strategie 3: Zufall (Alles mischen und abarbeiten)
int strat_Zufall(SimpleClient &client) {
    int versuche = 0;
    vector<Punkt> liste;
    // Liste füllen
    for (int x = 1; x <= 10; x++) for (int y = 1; y <= 10; y++) liste.push_back({x, y});
    // Liste mischen
    random_shuffle(liste.begin(), liste.end());

    for (size_t i = 0; i < liste.size(); i++) {
        versuche++;
        if (schuss(client, liste[i].x, liste[i].y).find("GAME_OVER") != string::npos) return versuche;
    }
    return versuche;
}

// Strategie 4: Smart (Zufall + Nachbarn prüfen bei Treffer)
int strat_Smart(SimpleClient &client) {
    int versuche = 0;
    bool besucht[12][12] = {false}; // Merken, wo wir waren
    vector<Punkt> ziele; // Liste für Nachbartreffer

    // Zufallsliste vorbereiten
    vector<Punkt> zufall;
    for (int x = 1; x <= 10; x++) for (int y = 1; y <= 10; y++) zufall.push_back({x, y});
    random_shuffle(zufall.begin(), zufall.end());
    int zIndex = 0;

    while (true) {
        Punkt p;

        // Haben wir ein Ziel (Nachbar von Treffer)?
        if (!ziele.empty()) {
            p = ziele.back();
            ziele.pop_back();
        } else {
            // Sonst nimm Zufall
            if (zIndex >= zufall.size()) break; // Sollte nicht passieren
            p = zufall[zIndex];
            zIndex++;
        }

        // Gültig checken (1-10) und ob schon besucht
        if (p.x < 1 || p.x > 10 || p.y < 1 || p.y > 10 || besucht[p.x][p.y]) continue;

        // Schießen
        besucht[p.x][p.y] = true;
        versuche++;
        string antwort = schuss(client, p.x, p.y);

        if (antwort.find("GAME_OVER") != string::npos) return versuche;

        // Wenn Treffer -> Nachbarn merken
        if (antwort.find("HIT") != string::npos || antwort.find("DESTROYED") != string::npos) {
            ziele.push_back({p.x + 1, p.y}); // Rechts
            ziele.push_back({p.x - 1, p.y}); // Links
            ziele.push_back({p.x, p.y + 1}); // Unten
            ziele.push_back({p.x, p.y - 1}); // Oben
        }
    }
    return versuche;
}

// ------------------------------------------------------------------
// 4. HAUPTPROGRAMM
// ------------------------------------------------------------------
int main() {
    srand(time(NULL));
    SimpleClient client;

    // PORT 2025 WIE IM SERVER!
    if (!client.verbinden("127.0.0.1", 2025)) {
        cout << "Fehler: Kein Server auf Port 2025 gefunden!" << endl;
        return 1;
    }
    cout << "Verbunden mit Server." << endl;

    int wahl;
    cout << "Strategie waehlen (1-4): ";
    cin >> wahl;

    ofstream datei("StatisticalData.csv");
    datei << "Spiel;Versuche\n"; // Header für CSV

    // 1000 Spiele Loop
    for (int i = 0; i < 1000; i++) {

        // Neues Spiel starten
        client.senden("NEWGAME");
        string check = client.empfangen(); // Warten auf "OK" vom Server

        int versuche = 0;
        if (wahl == 1) versuche = strat_Zeilen(client);
        else if (wahl == 2) versuche = strat_Schachbrett(client);
        else if (wahl == 3) versuche = strat_Zufall(client);
        else versuche = strat_Smart(client);

        // Speichern und Anzeigen
        datei << (i+1) << ";" << versuche << "\n";
        cout << "Game " << (i+1) << ": needed " << versuche << " attempts" << endl;
    }

    datei.close();
    cout << "Daten gespeichert in StatisticalData.csv" << endl;
    client.trennen();
    return 0;
}

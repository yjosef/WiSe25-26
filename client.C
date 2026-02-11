#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <algorithm>
#include <fstream>

using namespace std;


class SimpleClient {
public:
    int sock;
    struct sockaddr_in serverAdresse;


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


    void senden(string text) {
        send(sock, text.c_str(), text.size(), 0);
    }


    string empfangen() {
        char buffer[1024];
        memset(buffer, 0, 1024);
        recv(sock, buffer, 1024, 0);
        return string(buffer);
    }


    void trennen() {
        close(sock);
    }
};


struct Punkt { int x; int y; };


string schuss(SimpleClient &client, int x, int y) {

    string befehl = "SHOT[" + to_string(x) + "," + to_string(y) + "]";
    client.senden(befehl);
    return client.empfangen();
}



// Strategie 1: Zeile für Zeile
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

// Strategie 2: Schachbrett
int strat_Schachbrett(SimpleClient &client) {
    int versuche = 0;

    for (int y = 1; y <= 10; y++) {
        for (int x = 1; x <= 10; x++) {
            if ((x + y) % 2 == 0) {
                versuche++;
                if (schuss(client, x, y).find("GAME_OVER") != string::npos) return versuche;
            }
        }
    }

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

// Strategie 3: Zufall
int strat_Zufall(SimpleClient &client) {
    int versuche = 0;
    vector<Punkt> liste;

    for (int x = 1; x <= 10; x++) for (int y = 1; y <= 10; y++) liste.push_back({x, y});

    random_shuffle(liste.begin(), liste.end());

    for (size_t i = 0; i < liste.size(); i++) {
        versuche++;
        if (schuss(client, liste[i].x, liste[i].y).find("GAME_OVER") != string::npos) return versuche;
    }
    return versuche;
}

// Strategie 4: Smart (Zufall + Nachbarn)
int strat_Smart(SimpleClient &client) {
    int versuche = 0;
    bool besucht[12][12] = {false};
    vector<Punkt> ziele;


    vector<Punkt> zufall;
    for (int x = 1; x <= 10; x++) for (int y = 1; y <= 10; y++) zufall.push_back({x, y});
    random_shuffle(zufall.begin(), zufall.end());
    int zIndex = 0;

    while (true) {
        Punkt p;


        if (!ziele.empty()) {
            p = ziele.back();
            ziele.pop_back();
        } else {

            if (zIndex >= zufall.size()) break;
            p = zufall[zIndex];
            zIndex++;
        }


        if (p.x < 1 || p.x > 10 || p.y < 1 || p.y > 10 || besucht[p.x][p.y]) continue;


        besucht[p.x][p.y] = true;
        versuche++;
        string antwort = schuss(client, p.x, p.y);

        if (antwort.find("GAME_OVER") != string::npos) return versuche;


        if (antwort.find("HIT") != string::npos || antwort.find("DESTROYED") != string::npos) {
            ziele.push_back({p.x + 1, p.y});
            ziele.push_back({p.x - 1, p.y});
            ziele.push_back({p.x, p.y + 1});
            ziele.push_back({p.x, p.y - 1});
        }
    }
    return versuche;
}


int main() {
    srand(time(NULL));
    SimpleClient client;


    if (!client.verbinden("127.0.0.1", 2025)) {
        cout << "Error: No Server on Port 2025!" << endl;
        return 1;
    }
    cout << "Connected to the Server." << "\n" << "Welcome to Schiffeversenken" << "\n" << "The strategies: " << endl;

    int wahl;
    cout << "Line for Line (1)" << "\n" << "Chessboard (2)" << "\n" << "Random (3)" << "\n" << "Smart (Random + Neighbour) (4)" << "\n" << "Select strategy (1-4): ";
    cin >> wahl;

    ofstream datei("StatisticalData.csv");
    datei << "Games;Attempts\n";


    for (int i = 0; i < 1000; i++) {


        client.senden("NEWGAME");
        string check = client.empfangen();

        int versuche = 0;
        if (wahl == 1) versuche = strat_Zeilen(client);
        else if (wahl == 2) versuche = strat_Schachbrett(client);
        else if (wahl == 3) versuche = strat_Zufall(client);
        else versuche = strat_Smart(client);


        datei << (i+1) << ";" << versuche << "\n";
        cout << "Games " << (i+1) << ": needed " << versuche << " attempts" << endl;
    }

    datei.close();
    cout << "Datas saved in StatisticalData.csv" << endl;
    client.trennen();
    return 0;
}

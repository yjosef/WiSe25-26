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
#include <limits>

using namespace std;

class SimpleClient {
public:
    int sock;
    struct sockaddr_in address;

    bool connectToServer(string ip, int port) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1) {
            return false;
        }

        address.sin_addr.s_addr = inet_addr(ip.c_str());
        address.sin_family = AF_INET;
        address.sin_port = htons(port);

        if (connect(sock, (struct sockaddr*)&address, sizeof(address)) < 0) {
            return false;
        }
        return true;
    }

    void sendMessage(string message) {
        send(sock, message.c_str(), message.size(), 0);
    }

    string receiveMessage() {
        char buffer[1024];
        memset(buffer, 0, 1024);
        int len = recv(sock, buffer, 1024, 0);
        if (len <= 0) {
            return "";
        }
        return string(buffer);
    }

    void closeConnection() {
        close(sock);
    }
};

struct Point {
    int x;
    int y;
};

string shoot(SimpleClient &client, int x, int y) {
    string command = "SHOT[" + to_string(x) + "," + to_string(y) + "]";
    client.sendMessage(command);
    return client.receiveMessage();
}

int strategy1(SimpleClient &client) {
    int attempts = 0;
    for (int y = 1; y <= 10; y++) {
        for (int x = 1; x <= 10; x++) {
            attempts++;
            string response = shoot(client, x, y);
            if (response.find("GAME_OVER") != string::npos) {
                return attempts;
            }
        }
    }
    return attempts;
}

int strategy2(SimpleClient &client) {
    int attempts = 0;
    
    for (int y = 1; y <= 10; y++) {
        for (int x = 1; x <= 10; x++) {
            if ((x + y) % 2 == 0) {
                attempts++;
                string response = shoot(client, x, y);
                if (response.find("GAME_OVER") != string::npos) {
                    return attempts;
                }
            }
        }
    }

    for (int y = 1; y <= 10; y++) {
        for (int x = 1; x <= 10; x++) {
            if ((x + y) % 2 != 0) {
                attempts++;
                string response = shoot(client, x, y);
                if (response.find("GAME_OVER") != string::npos) {
                    return attempts;
                }
            }
        }
    }
    return attempts;
}

int strategy3(SimpleClient &client) {
    int attempts = 0;
    vector<Point> points;

    for (int x = 1; x <= 10; x++) {
        for (int y = 1; y <= 10; y++) {
            Point p;
            p.x = x;
            p.y = y;
            points.push_back(p);
        }
    }

    random_shuffle(points.begin(), points.end());

    for (size_t i = 0; i < points.size(); i++) {
        attempts++;
        string response = shoot(client, points[i].x, points[i].y);
        if (response.find("GAME_OVER") != string::npos) {
            return attempts;
        }
    }
    return attempts;
}

int strategy4(SimpleClient &client) {
    int attempts = 0;
    bool visited[12][12];
    
    for (int i = 0; i < 12; i++) {
        for (int j = 0; j < 12; j++) {
            visited[i][j] = false;
        }
    }

    vector<Point> randomPoints;
    for (int x = 1; x <= 10; x++) {
        for (int y = 1; y <= 10; y++) {
            Point p;
            p.x = x;
            p.y = y;
            randomPoints.push_back(p);
        }
    }
    random_shuffle(randomPoints.begin(), randomPoints.end());
    int randomIndex = 0;

    vector<Point> targets;

    while (true) {
        Point currentPoint;
        bool hasTarget = false;

        if (targets.size() > 0) {
            currentPoint = targets.back();
            targets.pop_back();
            hasTarget = true;
        } else {
            if (randomIndex < randomPoints.size()) {
                currentPoint = randomPoints[randomIndex];
                randomIndex++;
                hasTarget = true;
            }
        }

        if (hasTarget == false) {
            break; 
        }

        if (currentPoint.x < 1 || currentPoint.x > 10) continue;
        if (currentPoint.y < 1 || currentPoint.y > 10) continue;
        if (visited[currentPoint.x][currentPoint.y] == true) continue;

        visited[currentPoint.x][currentPoint.y] = true;
        attempts++;

        string response = shoot(client, currentPoint.x, currentPoint.y);

        if (response.find("GAME_OVER") != string::npos) {
            return attempts;
        }

        if (response.find("HIT") != string::npos || response.find("DESTROYED") != string::npos) {
            Point p1; p1.x = currentPoint.x + 1; p1.y = currentPoint.y; targets.push_back(p1);
            Point p2; p2.x = currentPoint.x - 1; p2.y = currentPoint.y; targets.push_back(p2);
            Point p3; p3.x = currentPoint.x; p3.y = currentPoint.y + 1; targets.push_back(p3);
            Point p4; p4.x = currentPoint.x; p4.y = currentPoint.y - 1; targets.push_back(p4);
        }
    }
    return attempts;
}

int main() {
    srand(time(NULL));
    SimpleClient client;

    if (client.connectToServer("127.0.0.1", 2025) == false) {
        cout << "Connection failed" << endl;
        return 1;
    }
    cout << "Connected" << endl;

    string command;

    while (true) {
        cout << "Enter command (NEWGAME or QUIT): ";
        cin >> command;

        if (command == "QUIT") {
            break;
        }

        if (command == "NEWGAME") {
            int strategy = 0;
            cout << "Select Strategy (1-4): ";
            cin >> strategy;

            if (strategy < 1 || strategy > 4) {
                cout << "Invalid strategy" << endl;
                continue;
            }

            ofstream file("StatisticalData.csv");
            file << "Game;Attempts\n";

            for (int i = 0; i < 1000; i++) {
                client.sendMessage("NEWGAME");
                string check = client.receiveMessage();

                int attempts = 0;
                if (strategy == 1) {
                    attempts = strategy1(client);
                } else if (strategy == 2) {
                    attempts = strategy2(client);
                } else if (strategy == 3) {
                    attempts = strategy3(client);
                } else {
                    attempts = strategy4(client);
                }

                file << (i + 1) << ";" << attempts << "\n";
            }
            file.close();
            cout << "Finished 1000 games" << endl;
        } else {
            cout << "Unknown command" << endl;
        }
    }

    client.closeConnection();
    return 0;
}

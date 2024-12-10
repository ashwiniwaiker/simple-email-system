#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <winsock2.h>
#include "json.hpp" 
#include <thread>
#include <mutex>
#include <fstream>

using namespace std;
using json = nlohmann::json;


// Email structure
struct Email {
    string from;
    string subject;
    string body;
};

// Global map to store user inboxes
map<string, vector<Email>> userInboxes;
pthread_mutex_t inboxMutex;

// File to store emails persistently
const string CACHE_FILE = "emails.json";

void saveEmailsToFile() {
    pthread_mutex_lock(&inboxMutex);
    json emailData;

    for (const auto &pair : userInboxes) {
        for (const auto &email : pair.second) {
            emailData[pair.first].push_back({{"from", email.from}, {"subject", email.subject}, {"body", email.body}});
        }
    }

    ofstream file(CACHE_FILE);
    if (file.is_open()) {
        file << emailData.dump(4); // Write JSON data with pretty formatting
        file.close();
    }
    pthread_mutex_unlock(&inboxMutex);

}

// Function to load emails from the cache file
void loadEmailsFromFile() {
    ifstream file(CACHE_FILE);
    if (file.is_open()) {
        json emailData;
        file >> emailData;

        for (const auto &pair : emailData.items()) {
            string username = pair.key();
            for (const auto &email : pair.value()) {
                userInboxes[username].push_back({email["from"], email["subject"], email["body"]});
            }
        }

        file.close();
    }
}


// Function to handle client requests
void handleClient(SOCKET clientSocket) {
    char buffer[1024];
    int bytesReceived;

    while (true) {
        // Receive data from client
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) break;

        buffer[bytesReceived] = '\0'; // Null-terminate the received string
        string receivedData(buffer);
        json request = json::parse(receivedData);

        // Process request
        string response;
        if (request["type"] == "inbox") {
            string username = request["username"];
            json inboxResponse;
            if (userInboxes.find(username) != userInboxes.end()) {
                for (const auto &email : userInboxes[username]) {
                    inboxResponse["emails"].push_back({{"from", email.from}, {"subject", email.subject}, {"body", email.body}});
                }
            } else {
                inboxResponse["status"] = "empty"; // Notify client that inbox is empty
            }
            response = inboxResponse.dump();
        } else if (request["type"] == "compose") {
            string to = request["to"];
            Email newEmail = {request["from"], request["subject"], request["body"]};
            userInboxes[to].push_back(newEmail);
            response = R"({"status": "Email sent successfully"})";
            saveEmailsToFile();
        }

        // Send response back to client
        send(clientSocket, response.c_str(), response.size(), 0);
    }

    // Close the client socket
    closesocket(clientSocket);
}

int main() {
    WSADATA wsaData;
    SOCKET serverSocket, clientSocket;
    sockaddr_in serverAddr, clientAddr;
    int clientAddrSize = sizeof(clientAddr);

    // Initialize Winsock
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // Create server socket
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        cerr << "Socket creation failed!" << endl;
        return 1;
    }

    // Configure server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(54000); // Port number
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket
    if (bind(serverSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Bind failed!" << endl;
        return 1;
    }

    // Listen for incoming connections
    listen(serverSocket, SOMAXCONN);
    cout << "Server is running on port 54000..." << endl;

    loadEmailsFromFile();

    // Accept and handle clients
    while (true) {
        clientSocket = accept(serverSocket, (sockaddr *)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET) {
            cerr << "Client connection failed!" << endl;
            continue;
        }
        cout << "Client connected!" << endl;
        thread clientThread(handleClient, clientSocket);
        clientThread.detach();
    }

    // Clean up
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}

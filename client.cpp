#include <iostream>
#include <string>
#include <winsock2.h>
#include "json.hpp" 

using namespace std;
using json = nlohmann::json;

string sendRequest(SOCKET socket, const json &request) {
    string requestData = request.dump();
    send(socket, requestData.c_str(), requestData.size(), 0);

    char buffer[2048];
    int bytesReceived = recv(socket, buffer, sizeof(buffer), 0);
    buffer[bytesReceived] = '\0'; 
    return string(buffer);
}

int main() {
    WSADATA wsaData;
    SOCKET clientSocket;
    sockaddr_in serverAddr;

    WSAStartup(MAKEWORD(2, 2), &wsaData);

    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        cerr << "Socket creation failed!" << endl;
        return 1;
    }

    // Configure server address
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(54000); // Server port number
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Server IP address

    // Connect to the server
    if (connect(clientSocket, (sockaddr *)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Connection to server failed!" << endl;
        return 1;
    }
    cout << "Connected to the server!" << endl;

    // Get username
    cout << "Enter your username: ";
    string username;
    cin >> username;

    while (true) {
        cout << "\nOptions:\n1. Inbox\n2. Compose\nEnter your choice: ";
        int choice;
        cin >> choice;

        if (choice == 1) {
            // Inbox request
            json inboxRequest = {{"type", "inbox"}, {"username", username}};
            string response = sendRequest(clientSocket, inboxRequest);
            json inboxResponse = json::parse(response);

            if (inboxResponse.contains("status") && inboxResponse["status"] == "empty") {
                cout << "\nYour inbox is empty.\n";
            } else {
                cout << "\nInbox:\n";
                int i = 1;
                for (const auto &email : inboxResponse["emails"]) {
                    cout << i++ << ". " << email["from"] << " - " << email["subject"] << endl;
                }

                // Read email
                cout << "\nEnter email number to read (-1 to go back): ";
                int emailNumber;
                cin >> emailNumber;
                if (emailNumber != -1) {
                    cout << "\nFrom: " << inboxResponse["emails"][emailNumber - 1]["from"] << endl;
                    cout << "Subject: " << inboxResponse["emails"][emailNumber - 1]["subject"] << endl;
                    cout << "Body: " << inboxResponse["emails"][emailNumber - 1]["body"] << endl;
                }
            }
        } else if (choice == 2) {
            // Compose email
            cout << "Enter recipient username: ";
            string recipient;
            cin >> recipient;

            cin.ignore(); // Clear input buffer
            cout << "Enter subject: ";
            string subject;
            getline(cin, subject);

            cout << "Enter body: ";
            string body;
            getline(cin, body);

            json composeRequest = {
                {"type", "compose"},
                {"from", username},
                {"to", recipient},
                {"subject", subject},
                {"body", body}};
            string response = sendRequest(clientSocket, composeRequest);
            cout << response << endl; // Confirm email sent
        } else {
            cout << "Invalid choice! Try again." << endl;
        }
    }

    // Close the socket
    closesocket(clientSocket);
    WSACleanup();
    return 0;
}

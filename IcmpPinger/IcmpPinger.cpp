// Lab 5: ICMP pinger due C++ using library winsock.h

#include <windowsx.h>
#include <iostream>
#include "icmp_structures.h"

#pragma comment(lib, "ws2_32.lib")
#pragma warning (disable: 4996)

using namespace std;

int ping(const char* host) {

    // ICMP.DLL LOADING
    HINSTANCE hIcmp;
    if ((hIcmp = LoadLibrary(L"ICMP.DLL")) == 0) {
        cerr << "Failed to load ICMP.DLL!" << endl;
        return 1;
    }

    // GETTING OF HOSTNAME
    struct hostent* phe;
    if ((phe = gethostbyname(host)) == 0) {
        cerr << "Could not find IP address for " << host << endl;
        return 2;
    }

    // FUNCTIONS FROM ICMP.DLL
    typedef HANDLE(WINAPI* pfnHV)(VOID);
    typedef BOOL(WINAPI* pfnBH)(HANDLE);
    typedef DWORD(WINAPI* pfnDHDPWPipPDD)(
        HANDLE,
        DWORD,
        LPVOID,
        WORD,
        PIP_OPTION_INFORMATION,
        LPVOID,
        DWORD,
        DWORD
        );

    pfnHV pIcmpCreateFile;
    pfnBH pIcmpCloseHandle;
    pfnDHDPWPipPDD pIcmpSendEcho;
    pIcmpCreateFile = (pfnHV)GetProcAddress(hIcmp, "IcmpCreateFile");
    pIcmpCloseHandle = (pfnBH)GetProcAddress(hIcmp, "IcmpCloseHandle");
    pIcmpSendEcho = (pfnDHDPWPipPDD)GetProcAddress(hIcmp, "IcmpSendEcho");

    if ((pIcmpCreateFile == 0) || (pIcmpCloseHandle == 0) ||
        (pIcmpSendEcho == 0)) {
        cerr << "Failed to get proc addr for function." << endl;
        return 3;
    }

    // PING SERVICE
    HANDLE hIP = pIcmpCreateFile();
    if (hIP == INVALID_HANDLE_VALUE) {
        cerr << "Failed to open ping service." << endl;
        return 4;
    }

    // BUILD: PING PACKET
    char acPingBuffer[64];
    memset(acPingBuffer, '\xAA', sizeof(acPingBuffer));

    PIP_ECHO_REPLY pIpe = (PIP_ECHO_REPLY)GlobalAlloc(
        GMEM_FIXED | GMEM_ZEROINIT,
        sizeof(IP_ECHO_REPLY) + sizeof(acPingBuffer)
    );

    if (pIpe == 0) {
        cerr << "Failed to allocate global ping packet buffer." << endl;
        return 5;
    }

    pIpe->Data = acPingBuffer;
    pIpe->DataSize = sizeof(acPingBuffer);

    // SEND: PING PACKET 
    DWORD dwStatus = pIcmpSendEcho(
        hIP,
        *((DWORD*)phe->h_addr_list[0]),
        acPingBuffer,
        sizeof(acPingBuffer),
        NULL,
        pIpe,
        sizeof(IP_ECHO_REPLY) + sizeof(acPingBuffer),
        5000
    );

    if (dwStatus != 0) {
        cout << "time = " << int(pIpe->RoundTripTime) << "ms, ";
        cout << "ttl = " << int(pIpe->Options.Ttl) << endl;
    }
    else {
        cerr << "Error obtaining info from ping packet." << endl;
    }

    GlobalFree(pIpe);
    FreeLibrary(hIcmp);
    return 0;
}

std::string getIPFromHostname(const std::string& hostName) {
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        std::cout << "WSAStartup failed: " << iResult << std::endl;
        WSACleanup();
        return "";
    }

    struct hostent* he;
    struct in_addr** addrList;

    if ((he = gethostbyname(hostName.c_str())) == NULL) {
        std::cout << "gethostbyname failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return "";
    }

    addrList = (struct in_addr**)he->h_addr_list;

    std::string ipAddress = "";
    for (int i = 0; addrList[i] != NULL; i++) {
        ipAddress = inet_ntoa(*addrList[i]);
        break;
    }

    WSACleanup();
    return ipAddress;
}

int chooseHost() {
    while (true) {
        char hostName[256];
        cout << "Type host name to ping: ";
        cin.getline(hostName, sizeof(hostName));
        cout << endl;

        cout << "Host name = " << hostName << endl;
        cout << "IP address = " << getIPFromHostname(hostName) << endl;

        WSAData wsaData;
        if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0) {
            return 255;
        }
        int result;
        for (int i = 0; i < 4; i++) {
            cout << "PING #" << i + 1 << ": ";
            result = ping(hostName);
        }
        cout << endl;
    }
    WSACleanup();
    return 0;
}

int main() {
    chooseHost();

    return 0;
}
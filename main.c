/* Copyright (c) Zeng Xiangfei 2019
 * QQ: 1023818103
 */

#include "event_messages.h"
#include <Windows.h>
#include <iphlpapi.h>
#include <winsock.h>
#include <tchar.h>

#define SERVICE_NAME TEXT("mydhcpclientsvc")
#define DISPLAY_NAME TEXT("My DHCP Client Service")
#define SERVICE_USER TEXT("NT AUTHORITY\\NetworkService")
#define LISTEN_FLAG_VALUE_NAME TEXT("EnableMyDHCP")
#define DHCP_IP_ADDRESS_VALUE_NAME TEXT("DhcpIPAddress")
#define VENDOR_CLASS_IDENTIFIER "android-dhcp-9"

SERVICE_STATUS serviceStatus;
SERVICE_STATUS_HANDLE serviceStatusHandle;
SIZE_T numAddress;
struct addrinfo **addrinfos;
SOCKET *sockets;
HANDLE *threads;
HANDLE addressChangeEvent;

int ReportServiceErrorEvent(DWORD messageId);
void ReportServiceSuccessEvent(DWORD messageId);
BOOL InstallService();
BOOL UninstallService();
void ReportServiceStatus(DWORD currentState, DWORD win32ExitCode, DWORD waitHint);
BOOL LoadListeningAddresses();
DWORD WINAPI ListenRoutine(LPVOID param);
void OnServiceStart();
void OnServiceDestroy();
void WINAPI ServiceCotrolHandler(DWORD control);
void WINAPI ServiceMain(DWORD argc, LPTSTR *argv);

void OnServiceStart()
{
    // initialize
    if (!LoadListeningAddresses())
    {
        ReportServiceStatus(SERVICE_STOPPED, GetLastError(), 0);
        return;
    }

    sockets = HeapAlloc(GetProcessHeap(), 0, sizeof(SOCKET) * numAddress);
    threads = HeapAlloc(GetProcessHeap(), 0, sizeof(HANDLE) * numAddress);

    NotifyAddrChange(&addressChangeEvent, NULL);

    for (DWORD i = 0; i < numAddress; i++)
    {
        sockets[i] = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        bind(sockets[i], addrinfos[i]->ai_addr, addrinfos[i]->ai_addrlen);
        // create threads and start listening to address changes
        threads[i] = CreateThread(NULL, 0, ListenRoutine, i, 0, NULL);
    }

    LocalFree(addrinfos);

    WaitForMultipleObjects(numAddress, threads, TRUE, INFINITE);

    /// clean up
    for (SIZE_T i = 0; i < numAddress; i++)
    {
        CloseHandle(sockets[i]);
    }
    CloseHandle(addressChangeEvent);

    HeapFree(GetProcessHeap(), 0, sockets);
    HeapFree(GetProcessHeap(), 0, threads);
}

void OnServiceDestroy()
{
    for (SIZE_T i = 0; i < numAddress; i++)
    {
        CloseHandle(threads[i]);
    }
}

DWORD WINAPI ListenRoutine(LPVOID param)
{
    DWORD i = param;
    for (;;)
    {
        WaitForSingleObject(addressChangeEvent, INFINITE);
    }
}

BOOL LoadListeningAddresses()
{
    HKEY interfacesKey;
    if (RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces"), &interfacesKey) != ERROR_SUCCESS)
    {
        ReportServiceErrorEvent(CANNOT_OPEN_REGISTER_KEY);
        return FALSE;
    }
    for (DWORD i = 0;; i++)
    {
        LPTSTR interfaceKeyName[MAX_PATH];
        if (RegEnumKey(interfacesKey, i, interfaceKeyName, sizeof(interfaceKeyName) / sizeof(TCHAR)) != ERROR_SUCCESS)
        {
            ReportServiceErrorEvent(CANNOT_ENUMERATE_REGISTER_KEYS);
            goto cleanup;
        }

        HKEY interfaceKey;
        if (RegOpenKey(interfacesKey, interfaceKeyName, &interfaceKey) != ERROR_SUCCESS)
        {
            ReportServiceErrorEvent(CANNOT_OPEN_REGISTER_KEY);
            goto cleanup;
        }

        LPTSTR address = NULL;
        LPBYTE value = NULL;
        DWORD size = 0;
        DWORD keyType = 0;
        LSTATUS status;
        if ((status = RegQueryValueEx(interfacesKey, LISTEN_FLAG_VALUE_NAME, NULL, &keyType, NULL, &size)) != ERROR_MORE_DATA)
        {
            if (status != ERROR_FILE_NOT_FOUND)
            {
                ReportServiceErrorEvent(CANNOT_QUERY_REGISTER_KEY_VALUE_SIZE);
            }
            goto cleanup;
        }
        value = LocalAlloc(0, size);
        if (RegQueryValueEx(interfacesKey, LISTEN_FLAG_VALUE_NAME, NULL, &keyType, value, &size) != ERROR_SUCCESS)
        {
            ReportServiceErrorEvent(CANNOT_QUERY_REGISTER_KEY_VALUE);
            goto cleanup;
        }

        if (keyType == REG_DWORD)
        {
            DWORD enableMyDhcp;
            memcpy_s(&enableMyDhcp, sizeof(DWORD), value, sizeof(DWORD));
            if (enableMyDhcp == 0)
            {
                goto cleanup;
            }
        }

        if ((status = RegQueryValueEx(interfacesKey, DHCP_IP_ADDRESS_VALUE_NAME, NULL, &keyType, NULL, &size)) != ERROR_MORE_DATA)
        {
            if (status != ERROR_FILE_NOT_FOUND)
            {
                ReportServiceErrorEvent(CANNOT_QUERY_REGISTER_KEY_VALUE_SIZE);
            }
            goto cleanup;
        }
        address = LocalAlloc(0, size);
        if (RegQueryValueEx(interfacesKey, DHCP_IP_ADDRESS_VALUE_NAME, NULL, &keyType, value, &size) != ERROR_SUCCESS)
        {
            ReportServiceErrorEvent(CANNOT_QUERY_REGISTER_KEY_VALUE);
            goto cleanup;
        }

        struct addrinfo hints = {
            .ai_family = AF_INET,
            .ai_socktype = SOCK_DGRAM,
            .ai_protocol = IPPROTO_UDP};
        struct addrinfo *addrinfo;
        if (getaddrinfo(address, TEXT("bootpc"), &hints, &addrinfo) != 0)
        {
            ReportServiceErrorEvent(CANNOT_GET_ADDRESS_INFO);
            goto cleanup;
        }

        addrinfos = numAddress = 0 ? LocalAlloc(0, sizeof(struct addrinfo) * ++numAddress) : LocalReAlloc(addrinfos, sizeof(struct addrinfo) * ++numAddress, 0);
        memcpy_s(addrinfos + numAddress - 1, sizeof(struct addrinfo), addrinfo, sizeof(struct addrinfo));

    cleanup:
        RegCloseKey(interfaceKey);
        LocalFree(value);
        LocalFree(address);
    }

    RegCloseKey(interfacesKey);
    return TRUE;
}

void ReportServiceSuccessEvent(DWORD messageId)
{
    HANDLE eventSource = RegisterEventSource(NULL, SERVICE_NAME);
    if (eventSource != NULL)
    {
        ReportEvent(
            eventSource,
            EVENTLOG_SUCCESS,
            0,
            messageId,
            NULL,
            1,
            0,
            NULL,
            NULL);
    }
    DeregisterEventSource(eventSource);
}

int ReportServiceErrorEvent(DWORD messageId)
{
    DWORD lastError = GetLastError();
    HANDLE eventSource = RegisterEventSource(NULL, SERVICE_NAME);
    LPTSTR strings[1];
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, lastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)strings, 0, NULL);
    if (eventSource != NULL)
    {
        ReportEvent(
            eventSource,
            EVENTLOG_ERROR_TYPE,
            0,
            messageId,
            NULL,
            1,
            0,
            strings,
            NULL);
    }
    LocalFree(strings[0]);
    DeregisterEventSource(eventSource);
    return lastError;
}

BOOL InstallService()
{
    SC_HANDLE service, scManager;
    TCHAR path[MAX_PATH];

    if (!GetModuleFileName(NULL, path, sizeof path / sizeof(TCHAR)))
    {
        ReportServiceErrorEvent(CANNOT_GET_MODULE_FILENAME);
        return FALSE;
    }

    scManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (scManager == NULL)
    {
        ReportServiceErrorEvent(CANNOT_OPEN_SCMANAGER);
        return FALSE;
    }

    service = OpenService(scManager, SERVICE_NAME, SERVICE_ALL_ACCESS);
    if (service != NULL)
    {
        if (!DeleteService(service))
        {
            ReportServiceErrorEvent(CANNOT_DELETE_SERVICE);
            return FALSE;
        }
    }
    service = CreateService(
        scManager,
        SERVICE_NAME,
        DISPLAY_NAME,
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_NORMAL,
        path,
        NULL,
        NULL,
        NULL,
        SERVICE_USER,
        NULL);
    if (service == NULL)
    {
        ReportServiceErrorEvent(CANNOT_CREATE_SERVICE);
        return FALSE;
    }
    CloseServiceHandle(service);
    CloseServiceHandle(scManager);
    return TRUE;
}

BOOL UninstallService()
{
    SC_HANDLE service, scManager;
    scManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (scManager == NULL)
    {
        ReportServiceErrorEvent(CANNOT_OPEN_SCMANAGER);
        return FALSE;
    }

    service = OpenService(scManager, SERVICE_NAME, SERVICE_ALL_ACCESS);
    if (service != NULL)
    {
        if (!DeleteService(service))
        {
            ReportServiceErrorEvent(CANNOT_DELETE_SERVICE);
            return FALSE;
        }
    }
    CloseServiceHandle(service);
    CloseServiceHandle(scManager);
    return TRUE;
}

void ReportServiceStatus(DWORD currentState, DWORD win32ExitCode, DWORD waitHint)
{
    static DWORD checkPoint = 1;

    serviceStatus.dwCurrentState = currentState;
    serviceStatus.dwWin32ExitCode = win32ExitCode;
    serviceStatus.dwWaitHint = waitHint;

    if (currentState == SERVICE_START_PENDING)
        serviceStatus.dwControlsAccepted = 0;
    else
        serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    if ((currentState == SERVICE_RUNNING) || (currentState == SERVICE_STOPPED))
        serviceStatus.dwCheckPoint = 0;
    else
        serviceStatus.dwCheckPoint = checkPoint++;

    SetServiceStatus(serviceStatusHandle, &serviceStatus);
}

void WINAPI ServiceCotrolHandler(DWORD control)
{
    switch (control)
    {
    case SERVICE_CONTROL_STOP:
        ReportServiceStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
        OnServiceDestroy();
        ReportServiceStatus(serviceStatus.dwCurrentState, NO_ERROR, 0);
        break;

    default:
        break;
    }
}

void WINAPI ServiceMain(DWORD argc, LPTSTR *argv)
{
    serviceStatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCotrolHandler);
    if (serviceStatusHandle == NULL)
    {
        ReportServiceErrorEvent(CANNOT_REGISTER_SERVICE_CONTROL_HANDLER);
        return;
    }

    serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    serviceStatus.dwServiceSpecificExitCode = 0;

    ReportServiceStatus(SERVICE_START_PENDING, NO_ERROR, 3000);
    OnServiceStart();
}

int _tmain(int argc, TCHAR *argv[])
{
    if (_tccmp(argv[1], TEXT("install")))
    {
        InstallService();
        return;
    }

    if (_tccmp(argv[1], TEXT("uninstall")))
    {
        UninstallService();
        return;
    }

    SERVICE_TABLE_ENTRY dispatchTable[] =
        {{SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
         {NULL, NULL}};

    if (!StartServiceCtrlDispatcher(dispatchTable))
    {
        ReportServiceErrorEvent(CANNOT_START_SERVICE_CONTROL_DISPATCHER);
        return -1;
    }

    return 0;
}
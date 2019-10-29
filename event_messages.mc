;/* Copyright (c) Zeng Xiangfei 2019
; * QQ: 1023818103
; */
;

MessageIdTypedef = DWORD

SeverityNames = (
    Success       = 0x0:STATUS_SEVERITY_SUCCESS
    Informational = 0x1:STATUS_SEVERITY_INFORMATIONAL
    Warning       = 0x2:STATUS_SEVERITY_WARNING
    Error         = 0x3:STATUS_SEVERITY_ERROR
)

FacilityNames = (
    System        = 0x0:FACILITY_SYSTEM
    Runtime       = 0x2:FACILITY_RUNTIME
    Stubs         = 0x3:FACILITY_STUBS
    Io            = 0x4:FACILITY_IO_ERROR_CODE
)

LanguageNames = (
    English       = 0x409:MSG00409
)

MessageId    = 0x1
Severity     = Error
Facility     = Runtime
SymbolicName = CANNOT_GET_MODULE_FILENAME
Language     = English
Cannot get module filename: %1.
.

MessageId    = 0x2
Severity     = Error
Facility     = Runtime
SymbolicName = CANNOT_OPEN_SCMANAGER
Language     = English
Cannot open service control manager: %1.
.

MessageId    = 0x3
Severity     = Error
Facility     = Runtime
SymbolicName = CANNOT_OPEN_SERVICE
Language     = English
Cannot open service: %1.
.

MessageId    = 0x4
Severity     = Error
Facility     = Runtime
SymbolicName = CANNOT_DELETE_SERVICE
Language     = English
Cannot delete service: %1.
.

MessageId    = 0x5
Severity     = Error
Facility     = Runtime
SymbolicName = CANNOT_CREATE_SERVICE
Language     = English
Cannot create service: %1.
.

MessageId    = 0x6
Severity     = Error
Facility     = Runtime
SymbolicName = CANNOT_REGISTER_SERVICE_CONTROL_HANDLER
Language     = English
Cannot register service control handler: %1.
.

MessageId    = 0x7
Severity     = Error
Facility     = Runtime
SymbolicName = CANNOT_GET_NETWORK_ADAPTER_NUMBERS
Language     = English
Cannot get network adapter numbers: %1.
.

MessageId    = 0x8
Severity     = Error
Facility     = Runtime
SymbolicName = CANNOT_GET_NETWORK_ADAPTERS
Language     = English
Cannot get network adapters: %1.
.

MessageId    = 0x9
Severity     = Error
Facility     = Runtime
SymbolicName = CANNOT_START_SERVICE_CONTROL_DISPATCHER
Language     = English
Cannot start service control dispatcher: %1.
.

MessageId    = 0xA
Severity     = Error
Facility     = Runtime
SymbolicName = CANNOT_OPEN_REGISTER_KEY
Language     = English
Cannot open register key: %1.
.

MessageId    = 0xB
Severity     = Error
Facility     = Runtime
SymbolicName = CANNOT_ENUMERATE_REGISTER_KEYS
Language     = English
Cannot enumerate register keys: %1.
.

MessageId    = 0xC
Severity     = Error
Facility     = Runtime
SymbolicName = CANNOT_GET_ADDRESS_INFO
Language     = English
Cannot get address info: %1.
.

MessageId    = 0xD
Severity     = Error
Facility     = Runtime
SymbolicName = CANNOT_INIT_WINSOCK
Language     = English
Cannot initialize Winsock: %1.
.

MessageId    = 0xF
Severity     = Error
Facility     = Runtime
SymbolicName = CANNOT_INIT_IOCP
Language     = English
Cannot initialize IOCP: %1.
.

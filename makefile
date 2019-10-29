# Copyright (c) Zeng Xiangfei 2019
# QQ: 1023818103

all: mydhcpclientsvc.exe

mydhcpclientsvc.exe: main.obj event_messages.res
	lib -out:mydhcpclientsvc.exe main.obj event_messages.res advapi32.lib iphlpapi.lib ws2_32.lib

main.obj: event_messages.h
	cl main.c

event_messages.rc event_messages.h:
	mc -U event_messages.mc

event_messages.res: event_messages.rc
	rc -r event_messages.rc

clean:
	del *.exe *.obj *.rc *.res *.bin event_messages.h
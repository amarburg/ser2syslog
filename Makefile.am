sbin_PROGRAMS = ser2syslog
AM_CFLAGS=-Wall
ser2syslog_SOURCES = controller.c dataxfer.c devcfg.c readconfig.c selector.c \
	ser2syslog.c utils.c telnet.c
noinst_HEADERS = controller.h dataxfer.h devcfg.h readconfig.h selector.h \
	utils.h telnet.h
man_MANS = ser2syslog.8
EXTRA_DIST = $(man_MANS) 
ACLOCAL_AMFLAGS=-I m4


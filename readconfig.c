/*
 *  ser2syslog - A program for forwarding serial data to syslog
 *  Copyright (C) 2011 Aaron Marburg <aaron.marburg@pg.canterbury.ac.nz>
 *
 *  Relative to ser2net, I've gutted the banner functionality and
 *  rearranged the configuration file a bit.
 *
 * Based heavily on:
 *
 *  ser2net - A program for allowing telnet connection to serial ports
 *  Copyright (C) 2001  Corey Minyard <minyard@acm.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* This file holds the code that reads the configuration file and
   calls the code in dataxfer to actually create all the ports in the
   configuration file. */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <syslog.h>

#include "dataxfer.h"
#include "readconfig.h"

#define MAX_LINE_SIZE 256	/* Maximum line length in the config file. */

static int config_num = 0;
static int lineno = 0;

struct tracefile_s
{
    char *name;
    char *str;
    struct tracefile_s *next;
};

/* All the tracefiles in the system. */
struct tracefile_s *tracefiles = NULL;

static void
handle_tracefile(char *name, char *fname)
{
    struct tracefile_s *new_tracefile;

    new_tracefile = malloc(sizeof(*new_tracefile));
    if (!new_tracefile) {
	syslog(LOG_ERR, "Out of memory handling tracefile on %d", lineno);
	return;
    }

    new_tracefile->name = strdup(name);
    if (!new_tracefile->name) {
	syslog(LOG_ERR, "Out of memory handling tracefile on %d", lineno);
	free(new_tracefile);
	return;
    }

    new_tracefile->str = strdup(fname);
    if (!new_tracefile->str) {
	syslog(LOG_ERR, "Out of memory handling tracefile on %d", lineno);
	free(new_tracefile->name);
	free(new_tracefile);
	return;
    }

    new_tracefile->next = tracefiles;
    tracefiles = new_tracefile;
}

char *
find_tracefile(char *name)
{
    struct tracefile_s *tracefile = tracefiles;

    while (tracefile) {
	if (strcmp(name, tracefile->name) == 0)
	    return tracefile->str;
	tracefile = tracefile->next;
    }
    syslog(LOG_ERR, "Tracefile %s not found, it will be ignored", name);
    return NULL;
}

static void
free_tracefiles(void)
{
    struct tracefile_s *tracefile;

    while (tracefiles) {
	tracefile = tracefiles;
	tracefiles = tracefiles->next;
	free(tracefile->name);
	free(tracefile->str);
	free(tracefile);
    }
}


void
handle_config_line(char *inbuf)
{
    char *devname, *devcfg, *facility, *severity;
    char *strtok_data = NULL;
    char *errstr;

    lineno++;

    if (inbuf[0] == '#') {
	/* Ignore comments. */
	return;
    }

    devname = strtok_r(inbuf, ":", &strtok_data);
    if (devname == NULL) {
	/* An empty line is ok. */
	return;
    }

    if (strcmp(devname, "TRACEFILE") == 0) {
	char *name = strtok_r(NULL, ":", &strtok_data);
	char *str = strtok_r(NULL, "\n", &strtok_data);
	if (name == NULL) {
	    syslog(LOG_ERR, "No tracefile name given on line %d", lineno);
	    return;
	}
	if ((str == NULL) || (strlen(str) == 0)) {
	    syslog(LOG_ERR, "No tracefile given on line %d", lineno);
	    return;
	}
	handle_tracefile(name, str);
	return;
    }

    devcfg = strtok_r(NULL, ":", &strtok_data);
    if (devcfg == NULL) {
	/* An empty device config is ok. */
	devcfg = "";
    }

    facility = strtok_r(NULL, ":", &strtok_data);
    if (facility == NULL) {
	syslog(LOG_ERR, "No facility given on line %d", lineno);
	return;
    }

    severity = strtok_r(NULL, ":", &strtok_data);
    if (severity == NULL) {
	syslog(LOG_ERR, "No severity given on line %d", lineno);
	return;
    }

    errstr = portconfig(devname, devcfg, facility, severity,
			config_num);
    if (errstr != NULL) {
	syslog(LOG_ERR, "Error on line %d, %s", lineno, errstr);
    }
}

/* Read the specified configuration file and call the routine to
   create the ports. */
int
readconfig(char *filename)
{
    FILE *instream;
    char inbuf[MAX_LINE_SIZE];
    int  rv = 0;

    lineno = 0;

    instream = fopen(filename, "r");
    if (instream == NULL) {
	syslog(LOG_ERR, "Unable to open config file '%s': %m", filename);
	return -1;
    }

    free_banners();
    free_tracefiles();

    config_num++;

    while (fgets(inbuf, MAX_LINE_SIZE, instream) != NULL) {
	int len = strlen(inbuf);
	if (inbuf[len-1] != '\n') {
	    lineno++;
	    syslog(LOG_ERR, "line %d is too long in config file", lineno);
	    continue;
	}
	/* Remove the '\n' */
	inbuf[len-1] = '\0';
	handle_config_line(inbuf);
    }

    /* Delete anything that wasn't in the new config file. */
    clear_old_port_config(config_num);

    fclose(instream);
    return rv;
}


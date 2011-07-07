/*
 *  ser2syslog - A program for forwarding serial communication to syslog.
 *  Copyright (C) 2011 Aaron Marburg <aaron.marburg@pg.cantebury.ac.nz>
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

#ifndef DATAXFER
#define DATAXFER

#include "controller.h"

/* Create a port given the criteria. */
char * portconfig( char *devname,
		  char *devcfg,
                  char *facility,
                  char *severity,
		  int  config_num);

/* Clear out any old ports on a reconfigure. */
void clear_old_port_config(int config_num);

/* Initialize the data transfer code. */
void dataxfer_init(void);

/* Show information about a port (or all ports if portspec is NULL).
   The parameters are all strings that the routine will convert to
   integers.  Error output will be generated on invalid data. */
void showports(struct controller_info *cntlr, char *portspec);

/* Show information about a port (as above) but in a one-line format. */
void showshortports(struct controller_info *cntlr, char *portspec);

/* Set the serial port's configuration.  The parameters are all
   strings that the routine will convert to integers.  Error output
   will be generated on invalid data. */
void setportdevcfg(struct controller_info *cntlr,
		   char *portspec,
		   char *devcfg);

/* Modify the DTR and RTS lines for the port. */
void setportcontrol(struct controller_info *cntlr,
		    char *portspec,
		    char *controls);

/* Start data monitoring on the given port, type may be either "tcp" or
   "term" and only one direction may be monitored.  This return NULL if
   the monitor fails.  The monitor output will go to the controller
   via the controller_write() call. */
void *data_monitor_start(struct controller_info *cntlr,
			 char *type,
			 char *portspec);

/* Stop monitoring the given id. */
void data_monitor_stop(struct controller_info *cntlr,
		       void   *monitor_id);

/* Shut down the port, if it is connected. */
void disconnect_port(struct controller_info *cntlr,
		     char *portspec);

#endif /* DATAXFER */

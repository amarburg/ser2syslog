/*
 *  ser2syslog - A program for forwarding serial data to syslog. 
 *  (C) 2011 Aaron Marburg <aaron.marburg@pg.canterbury.ac.nz>
 *
 *  Based very heavily on:
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

#ifndef DEVCFG
#define DEVCFG

#include <termios.h>

/* Called to initially configure a terminal. */
void devinit(struct termios *termctl);

/* Converts string to baud rate */
int string_to_baud( const char *b );

/* Called to change the configuration of a device based upon the
   string parameters. */
/*int devconfig(char *instr, dev_info_t *info); */

/* Prints the configuration of a device to a controller. */
/*void show_devcfg(struct controller_info *cntlr, struct termios *termctl); */

/* Sets the DTR and RTS lines dynamically. */
/*int setdevcontrol(char *instr, int fd); */

/* Show the state of the DTR and RTS lines. */
/*void show_devcontrol(struct controller_info *cntlr, int fd); */

/* Convert the serial parameters to a string. */
void serparm_to_str(char *str, int strlen, struct termios *termctl);

#endif /* DEVCFG */

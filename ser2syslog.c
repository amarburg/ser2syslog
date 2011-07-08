/*
 *  ser2syslog - A program for forwarding serial data to syslog. 
 *  (C) 2011 Aaron Marburg <aaron.marburg@pg.canterbury.ac.nz>
 *  Based very heavily on:
 *
 *  ser2syslog - A program for allowing telnet connection to serial ports
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

/* This is the entry point for the ser2syslog program.  It reads
   parameters, initializes everything, then starts the select loop. */

/* TODO
 *
 * Add some type of security
 */

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include "devcfg.h"

#define DEFAULT_FACILITY LOG_LOCAL0
#define DEFAULT_SEVERITY LOG_ERR
#define DEFAULT_BAUD     B9600
#define BUF_LEN          256

static char *dev_name = NULL;
static speed_t baud_rate = DEFAULT_BAUD;
static int facility = DEFAULT_FACILITY;
static int severity = DEFAULT_SEVERITY;

static char *pid_file = NULL;
static int detach = 1;
static int debug = 0;


static char *help_string =
"%s: Valid parameters are:\n"
"  -P <file> - set location of pid file\n"
"  -n - Don't detach from the controlling terminal\n"
"  -d - Don't detach and send debug I/O to standard output\n"
"  -v - print the program's version and exit\n";

  void
arg_error(char *name)
{
  fprintf(stderr, help_string, name);
  exit(1);
}

  void
make_pidfile(char *pidfile)
{
  FILE *fpidfile;
  if (!pidfile)
    return;
  fpidfile = fopen(pidfile, "w");
  if (!fpidfile) {
    syslog(LOG_WARNING,
        "Error opening pidfile '%s': %m, pidfile not created",
        pidfile);
    return;
  }
  fprintf(fpidfile, "%d\n", getpid());
  fclose(fpidfile);
}

void
show_ser_params( FILE *stream, struct termios *termctl   )
{
  char buf[80];

  serparm_to_str( buf, 79, termctl );
  fprintf( stream, "%s\n", buf );
}


  int
main(int argc, char *argv[])
{
  char c;
  int devfd;
  int syslog_options, fd_options;

  char buf[BUF_LEN+2];
  int buf_ptr = 0;

  char *eol = "\x0D\x0A";
  char *eol_ptr;

  int bytes_read;
  int prev_buf_ptr;

  while( c == getopt( argc, argv, "P:ndv" ) ) {
    switch( c ) {
      case 'n':
        detach = 0;
        break;

      case 'd':
        detach = 0;
        debug = 1;
        break;

      case 'P':
        pid_file = optarg;
        break;

      case 'v':
        printf("%s version %s\n", argv[0], VERSION);
        exit(0);

      default:
        fprintf(stderr, "Invalid option: '%c'\n", c);
        arg_error(argv[0]);
    }
  }


  //setup_sighup();

  if (detach) {
    int pid;

    /* Detach from the calling terminal. */
    if ((pid = fork()) > 0) {
      exit(0);
    } else if (pid < 0) {
      syslog(LOG_ERR, "Error forking first fork");
      exit(1);
    } else {
      /* setsid() is necessary if we really want to demonize */
      setsid();
      /* Second fork to really deamonize me. */
      if ((pid = fork()) > 0) {
        exit(0);
      } else if (pid < 0) {
        syslog(LOG_ERR, "Error forking second fork");
        exit(1);
      }
    }

    /* Close all my standard I/O. */
    chdir("/");
    close(0);
    close(1);
    close(2);
  }

  /* write pid file */
  make_pidfile(pid_file);

  /* Ignore SIGPIPEs so they don't kill us. */
  signal(SIGPIPE, SIG_IGN);

  syslog_options = LOG_NDELAY;
  if( debug && !detach ) syslog_options |= LOG_PERROR;
  openlog( dev_name, syslog_options, facility );

  // Open the serial port
  struct termios termctl;
  devinit( &termctl );

  cfsetospeed( &termctl, baud_rate );
  cfsetispeed( &termctl, baud_rate );

  if( debug ) {
    show_ser_params( stdout, &termctl );
  }

  fd_options = O_NONBLOCK | O_NOCTTY | O_RDONLY;
  devfd = open(  dev_name, fd_options );

  if( devfd == -1 ) {
    close( devfd );
    syslog( LOG_ERR, "Could not open device %s", dev_name );
  }

  tcsetattr( devfd, TCSANOW, &termctl );

  while( bytes_read = read( devfd, &(buf[buf_ptr]), BUF_LEN-buf_ptr ) ) {
    pre_buf_ptr = buf_ptr;
    buf_ptr += bytes_read;
    buf[buf_ptr] = '\0';

    if( buf_ptr => BUF_LEN ) {
      syslog( severity, "%BUF_LENs", buf );
      buf_ptr = 0;
    }

    if( pre_buf_ptr > strlen( eol ) ) { pre_buf_ptr -= strlen(eol) };
    else { pre_buf_ptr = 0; }

    if( eol_ptr = strstr( &(buf[pre_buf_ptr]), eol ) ) {
      eol_ptr = '\0';
      eol_ptr += strlen(eol);

      syslog( severity, "%s", buf );

      if( eol_ptr < buf_ptr ) {
        memcpy( buf, eol_tr, (buf_ptr - eol_ptr) );
        buf_ptr = 0;
      }

    }
  }

  return 0;
}


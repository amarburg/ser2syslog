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
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <stdbool.h>

#include <errno.h>

#include "devcfg.h"

#define DEFAULT_FACILITY LOG_LOCAL0
#define DEFAULT_SEVERITY LOG_ERR
#define DEFAULT_BAUD     B9600
#define BUF_LEN          2048
#define IDENT_BUF_LEN    64

static char *dev_name = NULL;
static speed_t baud_rate = DEFAULT_BAUD;
static int facility = DEFAULT_FACILITY;
static int severity = DEFAULT_SEVERITY;

static char *pid_file = NULL;
static bool detach = true;
static bool debug = false;
static bool do_fifo = false;

static char *help_string =
"%s: "
"Usage: %s -[Pndv] <portname>\n"
"Parameters are:\n"
"  -P <file>   Set location of pid file\n"
"  -b <baud>   Set baud rate\n"
"  -n          Don't detach from the controlling terminal\n"
"  -d          Don't detach and send debug I/O to standard output\n"
"  -f          Create a FIFO named <portname>\n"
"  -v          Print the program's version and exit\n";

void arg_error(char *name)
{
  fprintf(stderr, help_string, name,name);
  exit(1);
}

void make_pidfile(char *pidfile)
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

void show_ser_params( FILE *stream, struct termios *termctl   )
{
  char buf[80];

  serparm_to_str( buf, 79, termctl );
  fprintf( stream, "%s\n", buf );
}

void at_exit( void )
{
  if( do_fifo == true ) {
    unlink( dev_name );
  }
}

void at_signal( int signum )
{
  exit(signum);
}

void show_eol_chars( const char *eol )
{
  int i;
  fprintf(stderr,"EOL sequence: ");

  for( i = 0; eol[i] != '\0'; i++ ) {
    fprintf( stderr, "\\x%02X ", eol[i] );
  }
  fprintf( stderr, "\n" );
}


int main(int argc, char *argv[])
{
  char c;
  int devfd=-1;
  int syslog_options, fd_options;
  char ident_buf[IDENT_BUF_LEN];

  char buf[BUF_LEN+1];
  char *buf_write_ptr = buf;


  char *eol = "\x0A"; //"\x0D\x0A";
  char *eol_ptr;
  char *msg_ptr;
  int eol_offset;

  int bytes_read;

  while( (c = getopt( argc, argv, "P:nb:dfv" )) != -1 ) {
    switch( c ) {
      case 'n':
        detach = false;
        break;

      case 'd':
        detach = false;
        debug = true;
        break;

      case 'b':
        baud_rate = string_to_baud( optarg );
        if( baud_rate < 0 ) {
          fprintf(stderr, "Can't parse baud rate \"%s\"\n", optarg );
          arg_error(argv[0]);
        }
        break;

      case 'P':
        pid_file = optarg;
        break;

      case 'f':
        do_fifo = true;
        break;

      case 'v':
        printf("%s version %s\n", argv[0], VERSION);
        exit(0);

      case ':':
        fprintf(stderr, "Missing parameter for: '%c'\n", optopt);
        arg_error(argv[0]);
      case '?':
        fprintf(stderr, "Invalid option: '%c'\n", optopt);
        arg_error(argv[0]);

      default:
        fprintf(stderr, "Invalid option: '%c'\n", c);
        arg_error(argv[0]);
    }
  }

  if( optind >= argc ) {
    fprintf(stderr, "Need to specify port on command line\n");
    arg_error(argv[0]);
  }

  dev_name = argv[optind];

  if( debug) {
    fprintf(stderr,"Opening device %s", dev_name );
    show_eol_chars( eol );
  }

  atexit( at_exit );
  signal( SIGHUP, at_signal );
  signal( SIGINT, at_signal );

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
  // Terminate with two colons to make cutting easier 
  snprintf( ident_buf, IDENT_BUF_LEN, "ser2syslog [%s]::", dev_name );
  openlog( ident_buf, syslog_options, facility );

  if( do_fifo == false ) {
    // Open the serial port
    struct termios termctl;
    devinit( &termctl );

    cfsetospeed( &termctl, baud_rate );
    cfsetispeed( &termctl, baud_rate );

    if( debug == true ) {
      fprintf(stderr, "Serial params: ");
      show_ser_params( stderr, &termctl );
    }

    fd_options = O_NOCTTY | O_RDONLY;
    devfd = open(  dev_name, fd_options );

    if( devfd == -1 ) {
      syslog( LOG_ERR, "Could not open device %s %s", dev_name, strerror(errno) );
      close( devfd );
      exit(-1);
    }

    tcsetattr( devfd, TCSANOW, &termctl );

  }


  while( true ) {

    // If working with a FIFO, it needs to be recycled after each client connect/disconnects
    if( do_fifo == true ) {
      if( devfd >= 0 ) close(devfd);
      // Open as a FIFO
      if( mkfifo( dev_name, S_IRUSR | S_IWUSR ) < 0 ) {
        if( errno != EEXIST ) {
          syslog( LOG_ERR, "Error opening FIFO \"%s\": %s", dev_name, strerror(errno) );
          exit(-1);
        }
      }

      // Open the FIFO blocking, it won't truly open until there's a writer..
      devfd = open( dev_name, O_RDONLY );

      if( devfd == -1 ) {
        syslog( LOG_ERR, "Could not open FIFO \"%s\": %s", dev_name, strerror(errno) );
        close( devfd );
        exit(-1);
      }
    }

    while( (bytes_read = read( devfd, buf_write_ptr, BUF_LEN-(buf_write_ptr-buf) ))  > 0) {

      buf_write_ptr += bytes_read;

      // Check for overflow
      if( (buf_write_ptr-buf) > BUF_LEN ) {
        buf[BUF_LEN] = '\0';
        syslog( severity, "overflow: %*s", BUF_LEN, buf );
        buf_write_ptr = buf;
        continue;
      }

      *buf_write_ptr = '\0';

      //fprintf(stderr,"buf [%d]: %s\n", buf_write_ptr-buf, buf );

      msg_ptr = eol_ptr = buf;
      while( (eol_ptr = strstr( eol_ptr, eol )) != NULL ) {
        *eol_ptr = '\0';
        eol_ptr += strlen(eol);

        // Special exception for \0x0D sequence
        if( eol[0] == '\x0A' && 
            strlen(buf)>0 &&  
            buf[ strlen(buf)-1 ] == '\x0D' ) {
          buf[strlen(buf)-1] = '\0';
        }

        syslog( severity, "%s", msg_ptr  );

        msg_ptr = eol_ptr;
      }

      if( eol_ptr < buf_write_ptr ) {
        int bytes_to_save = (buf_write_ptr - msg_ptr );
        memcpy( buf, msg_ptr, bytes_to_save );
        buf_write_ptr -= (msg_ptr - buf);
      } else {
        buf_write_ptr = buf;
      }
      


    }
  }

  syslog( LOG_ERR, "Exiting as we read %d: %s", bytes_read, strerror(errno) );

  exit(0);
}


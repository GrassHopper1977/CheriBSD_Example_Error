#include <stdio.h>   	/* Standard input/output definitions */
#include <string.h>  	/* String function definitions */
#include <unistd.h>  	/* UNIX standard function definitions */
#include <fcntl.h>   	/* File control definitions */
#include <errno.h>   	/* Error number definitions */
#include <termios.h> 	/* POSIX terminal control definitions */
#include <sys/socket.h>	// Sockets
#include <netinet/in.h>
#include <sys/un.h>	// ?
#include <sys/event.h>	// Events
#include <assert.h>	// The assert function
#include <unistd.h>	// ?
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>


#ifdef __CHERI_PURE_CAPABILITY__
#define PRINTF_PTR "#p"
#else
#define PRINTF_PTR "p"
#endif

#define MAX_EVENTS	(32)

// Process the serial data here
size_t processSerial(int fd)
{
  static char buffer[255]; // input buffer
  static char *pbuffer = buffer;  // current place in buffer
  int nbytes = 0;  // bytes read so far
  size_t size = 0;

  while((nbytes = read(fd, pbuffer, sizeof(buffer) - (buffer - pbuffer) - 1)) > 0)
  {
    pbuffer += nbytes;
    //printf("We read %i bytes this time.\n", nbytes);
    if((pbuffer[-1] == '\n') || (pbuffer[-1] == '\r'))
    {
      *pbuffer = '\0';
      //printf("pbuffer[-2] = 0x%02x\n", pbuffer[-2]);
      //printf("pbuffer[-1] = 0x%02x\n", pbuffer[-1]);
      //printf("pbuffer[0] = 0x%02x\n", pbuffer[0]);
      //printf("buffer: %.*s\n", (int)strnlen(buffer, sizeof(buffer) - 1), buffer);
      if(strnlen(buffer, sizeof(buffer) - 1) > 1)
      {
        //printf("buffer: %s\n", buffer);
        size = strnlen(buffer, sizeof(buffer) - 1);
        printf("buffer: %.*s\n", (int)size, buffer);
        //nmea_parse(buffer, size);
      }
      pbuffer = buffer;  // Reset line pointer
      //break;
    }
  }
  if(nbytes == 0)
  {
    printf("Waste! nbytes = %i\n", nbytes);
  }
  else
  {
    printf("Read error! nbytes = %i errno = %i (", nbytes, errno);
    switch(errno) {
      case EBADF:
        printf("EBADF");
        break;
      case ECONNRESET:
        printf("ECONNRESET");
        break;
      case EFAULT:
        printf("EFAULT");
        break;
      case EIO:
        printf("EIO");
        break;
      case EINTEGRITY:
        printf("EINTEGRITY");
        break;
      case EBUSY:
        printf("EBUSY");
        break;
      case EINTR:
        printf("EINTR");
        break;
      case EINVAL:
        printf("EINVAL");
        break;
      case EAGAIN:
        printf("EAGAIN or EWOULDBLOCK");
        break;
      case EISDIR:
        printf("EISDIR");
        break;
      case EOPNOTSUPP:
        printf("EOPNOTSUPP");
        break;
      case EOVERFLOW:
        printf("EOVERFLOW");
        break;
      default:
        printf("unknown %i", nbytes);
        break;
    }
    printf(")\n");
  }
  return size;
}

// ports_open
// Opens and configures serial port
// returns file descriptor to port or -1 is an error
int ports_open()
{
  int fd; /* File descriptor for the port */
  struct termios options;
  char* portname = "/dev/ttyU0";

  printf("Opening port %s\n", portname);
  fd = open(portname, O_RDWR | O_NOCTTY | O_NDELAY);
  if (fd == -1)
  {
    perror("open_port: Unable to open serial port ");
    fprintf(stderr, "Unable to open %s\n", portname);
  }
  else
  {
    // Non-Blocking read & write
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    /* get the current options */
    tcgetattr(fd, &options);

    printf("Buad rate set to 9600bps.\n");
    cfsetispeed(&options, B9600);
    cfsetospeed(&options, B9600);

    // Character size = 8 bit
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;

#ifdef CSTOPB
    // Set the number of stopbits.
    options.c_cflag &= ~CSTOPB; // Signal stop bit
#endif

#ifdef PARENB
    // No parity.
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~PARODD;
#endif

#ifdef CRTSCTS
    // No hardware flow control
    options.c_cflag &= ~CRTSCTS;
#endif

    /* set raw input, 1 second timeout */
    options.c_cflag     |= (CLOCAL | CREAD);
    options.c_lflag     &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_lflag     |= ICANON;
    options.c_iflag     &= ~(IXON | IXOFF | IXANY | INLCR | ICRNL);
    options.c_oflag     &= ~OPOST;
    options.c_cc[VMIN]  = 0;
    options.c_cc[VTIME] = 10;

    /* set the options */
    tcsetattr(fd, TCSANOW, &options);
  }

  return (fd);
}  

int processing_loop(int kq, int serialFd)
{
  struct kevent evList[MAX_EVENTS];

  printf("Entering Main Loop...\n");
  printf("serialFd = %i\n", serialFd);
  while(1) {
    int nev = kevent(kq, NULL, 0, evList, MAX_EVENTS, NULL);
    if(nev < 0) {
      perror("ERROR: kevent error");
      exit(1);
    }

    for(int i = 0; i < nev; i++)
    {
      if(serialFd == (int)(evList[i].ident)) {
        if(evList[i].flags & EV_ERROR) {
          printf("Serial Error!");
        } else if(evList[i].flags & EV_EOF) {
          printf("Serial closed!");
        } else {
          printf("Serial data! Bytes waiting: %li\n", evList[i].data);
          processSerial(serialFd);
        }
      }
    }
  }

  return 0;
}

// Main program entry point. 1st argument will be path to config.json, if it's not present then we'll use the default filename.
int main(int argc, char *argv[])
{
  int fd; // File descriptor for the port
  fd = ports_open();
  if(fd < 0) {
    exit(1);
  }

  printf("Creating Event Queue...\n");
  int kq = kqueue();
  struct kevent evSet;
  EV_SET(&evSet, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
  assert(-1 != kevent(kq, &evSet, 1, NULL, 0, NULL));

  processing_loop(kq, fd);

  printf("Closing the port.\n");
  close(fd);
}

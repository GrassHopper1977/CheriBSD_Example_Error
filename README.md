# CheriBSD Example Error
We thought that we'd found an error with the `read` when used with USB to Serial devices but inactual fact we'd found a good example of the power of Cheri to protect us from faults caused by simple typos. Mistakes that are easy to make but can be difficult to spot. We're keeping this repository as an example, as we think it helps to show a real world example of a Cheri fixing a bug.

p.s. Apologies for misspelling the name of the OS in the title of the project. We will rename it at some stage.

## Steps To Verify
- [x] Write basic description
- [x] Build minimum code showing the behavior
- [x] Write a method to follow to test behaviour
- [x] Verify bahavior on differnt kernels
- [x] Check if the potential bug has already been reported.
- [x] Complete the detailed description of the issue
- [x] Post on Slack and see if anyone else has been this bahvior
- [X] Read Slack reply showing our obvious mistake and feel a bit embarrassed
- [X] Turn this repo into an example of the power of Cheri to catch and warn us of our errors.

# Original "Suspected Bug"
The code used in this example is in [test.c](test.c)
## Description
- On serial code compiled for purecap mode: `read()` always returns `EFAULT` if you run your code within a short period of time after USB to serial device is connected.
- The code will never recover once this state has been entered. kqueue will report that there are bytes to read but every attempt to read will result in the same error.
- Stop the code and the re-run it and the problem isn't there.
- Wait 10 seconds after plugging the USB to serial device in and the problem isn't there.
- Compiling for hybrid mode doesn't exhibit this behaviour either.
- Problem appears to be independent of which kernel we are using.

Note: Our USB to Serial device is connected through a USB hub for these tests. When we unplug we are unplugging it from the hub and not directly into the Morello Cheri device.

## Method
- Use `build` to create a hybrid build (`test_hy`) and a purecap build (`test_pc`).
- Plug in a USB to Serial device that is connected to a serial data source (in our example a GNSS device).
- Execute `.\test_hy`. It take a couple of attempts to align with the end of lines but then it will begin to display the NMEA sentances. We can ignore the `EAGAIN` error as that is just there because our code is non-blocking and `The file was marked for non-blocking I/O, and no data were ready to be read.`:
```
root@cheribsd:~/github/CherryBSD_USB_Serial_Issue # ./test_hy
Opening port /dev/ttyU0
Buad rate set to 9600bps.
Creating Event Queue...
Entering Main Loop...
serialFd = 3
Serial data! Bytes waiting: 762
buffer: ▒

Read error! nbytes = -1 errno = 35 (EAGAIN or EWOULDBLOCK)
Serial data! Bytes waiting: 41
buffer: ▒▒▒jR▒$GPTXT,01,01,02,ANTSTATUS=OPEN*2B

Read error! nbytes = -1 errno = 35 (EAGAIN or EWOULDBLOCK)
Serial data! Bytes waiting: 77
buffer: $GNRMC,104851.000,A,5249.750190,N,00206.543755,W,1.32,12.62,050223,,,A,V*21

Read error! nbytes = -1 errno = 35 (EAGAIN or EWOULDBLOCK)
Serial data! Bytes waiting: 38
buffer: $GNVTG,12.62,T,,M,1.32,N,2.44,K,A*16
```
- Now disconnect the USB to Serial device (Note that we don't get a error for this, it just stops working. You would think it would return something to indicate that the pipe is closed but it doesn't).
- Now execute `.\test_pc` and swe get quite a different response.
```
root@cheribsd:~/github/CherryBSD_USB_Serial_Issue # ./test_pc
Opening port /dev/ttyU0
Buad rate set to 9600bps.
Creating Event Queue...
Entering Main Loop...
serialFd = 3
Serial data! Bytes waiting: 332
Read error! nbytes = -1 errno = 14 (EFAULT)
Serial data! Bytes waiting: 76
Read error! nbytes = -1 errno = 14 (EFAULT)
Serial data! Bytes waiting: 75
Read error! nbytes = -1 errno = 14 (EFAULT)
Serial data! Bytes waiting: 74
Read error! nbytes = -1 errno = 14 (EFAULT)
Serial data! Bytes waiting: 289
Read error! nbytes = -1 errno = 14 (EFAULT)
Serial data! Bytes waiting: 237
Read error! nbytes = -1 errno = 14 (EFAULT)
Serial data! Bytes waiting: 173
Read error! nbytes = -1 errno = 14 (EFAULT)
Serial data! Bytes waiting: 109
Read error! nbytes = -1 errno = 14 (EFAULT)
Serial data! Bytes waiting: 104
Read error! nbytes = -1 errno = 14 (EFAULT)
```
- Press `Ctrl-C` to exit and then execute `.\test_pc` again. It will work like the hybrid version:
```
root@cheribsd:~/github/CherryBSD_USB_Serial_Issue # ./test_pc
Opening port /dev/ttyU0
Buad rate set to 9600bps.
Creating Event Queue...
Entering Main Loop...
serialFd = 3
Serial data! Bytes waiting: 257
buffer: $GPTXT,01,01,02,ANTSTATUS=OPEN*2B

buffer: $GNRMC,110429.000,A,5249.749311,N,00206.547166,W,0.00,206.08,050223,,,A,V*1D

buffer: $GNVTG,206.08,T,,M,0.00,N,0.00,K,A*2F

buffer: $GNGGA,110429.000,5249.749311,N,00206.547166,W,1,8,1.32,71.046,M,48.332,M,,*6D

buffer: $GNGSA,A,3,12,23,13,24,2

Read error! nbytes = -1 errno = 35 (EAGAIN or EWOULDBLOCK)
Serial data! Bytes waiting: 35
buffer: $GPTXT,01,01,02,ANTSTATUS=OPEN*2B

Read error! nbytes = -1 errno = 35 (EAGAIN or EWOULDBLOCK)
Serial data! Bytes waiting: 78
buffer: $GNRMC,110431.000,A,5249.749311,N,00206.547166,W,0.00,206.08,050223,,,A,V*14

Read error! nbytes = -1 errno = 35 (EAGAIN or EWOULDBLOCK)
```
- You can play with the timing a little bit. For example: If you wait 10 seconds after plugging in your serial device the purecap build will work correctly. It opnly appears to go wrong if you attempt to read immediately after plugging in the USB to Serial device.

## What Does It Mean?
Consulting the documentation [here](https://man.freebsd.org/cgi/man.cgi?sektion=2&query=read) informs us of the follwing:
```
[EFAULT]		The buf	argument points	outside	the allocated address space.
```
Obviously, we're not actually doing this as the same code works after a short period of time and works on hybrid builds.

## Verify bahavior on differnt kernels
### Our Default Kernel
We are currently running a test build of the kernel built (note to self: only tag kernel author once bug is confirmed) to fix this issue: https://github.com/CTSRD-CHERI/cheribsd/issues/1616
Version tested with:
```
root@cheribsd:~/github # uname -a
FreeBSD cheribsd.local 14.0-CURRENT FreeBSD 14.0-CURRENT #0 ugen-ep-copyincap-n256372-c708375e636: Mon Jan  9 16:16:12 GMT 2023     jrtc4@technos.cl.cam.ac.uk:/local/scratch/jrtc4/libusb-cheribuild-root/build/cheribsd-morello-purecap-build/local/scratch/jrtc4/libusb-cheribuild-root/cheribsd/arm64.aarch64c/sys/GENERIC-MORELLO arm64
```
Effect observed: Yes

### Default V22.12 Purecaps Kernel
From Boot Menu: `6. kernel: kernel.GENERIC-MORELLO-PURECAP (4 of 5)`
```
root@cheribsd:~ # uname -a
FreeBSD cheribsd.local 14.0-CURRENT FreeBSD 14.0-CURRENT #0 8993e1c3bba: Thu Dec 15 23:39:13 UTC 2022     jenkins@ctsrd-build-linux-xx:/local/scratch/jenkins/workspace/CheriBSD-pipeline_releng_22.12/cheribsd-morello-purecap-build/local/scratch/jenkins/workspace/CheriBSD-pipeline_releng_22.12/cheribsd/arm64.aarch64c/sys/GENERIC-MORELLO-PURECAP arm64
root@cheribsd:~ #
```
Effect observed: Yes

### Default V22.12 Hybrid Kernel
From Boot Menu: `6. kernel: default/kernel (1 of 5)`
```
root@cheribsd:~ # uname -a
FreeBSD cheribsd.local 14.0-CURRENT FreeBSD 14.0-CURRENT #0 8993e1c3bba: Thu Dec 15 23:25:54 UTC 2022     jenkins@ctsrd-build-linux-xx:/local/scratch/jenkins/workspace/CheriBSD-pipeline_releng_22.12/cheribsd-morello-purecap-build/local/scratch/jenkins/workspace/CheriBSD-pipeline_releng_22.12/cheribsd/arm64.aarch64c/sys/GENERIC-MORELLO arm64
root@cheribsd:~ #
```
Effect observed: Yes

## Check if the potential bug has already been reported.
Not that we could see. There were 5 issues that mention `serial` and  7 that mention `usb`. None of them seemed applicable to our case.

## Asking on Slack if anymone has seen this behaviour
It seems that we messed up our code. There is a typo (and also a dangerous assumption).

# Fixing Our Issue
## First we need to find our issue. there are two main problems here:
### 1. A Typo
`buffer - pbuffer` is the wrong way around as pbuffer is going to be the larger to the two. Therefore, we are not correctly calculating the number of bytes left in our buffer and instead of passing a gradually reducing number we are passing a gradually increasing number. Cheri picks this up because Capabilities have a pointer and infromation to range check the area that you are allowed to write to. The OS is checking and realising that we will be writing outside of our valid range and is correctly returning `EFAULT`.
The erroneous code is [here in line 55](test.c#L55):
```c
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
```
### 2. A Danegrous Assumption
We assume that a message will not arrive that is larger than our buffer. We are reading from a GNSS device here and are assuming that the NMEA protocol has been followed byte it is possible that there may be devices that emit data that is longer than the maximum line length in teh spec. In this case we can change the code to check for the buffer being full and discard message that are too long.
In this case the error is that check `if(nbytes == 0)`:
```c
  if(nbytes == 0)
  {
    printf("Waste! nbytes = %i\n", nbytes);
  }
  else
  {
    printf("Read error! nbytes = %i errno = %i (", nbytes, errno);
```
The problem is that we don't check why we read no bytes. It's possible that there were no bytes to read but also possible that the buffer was full.
## Fixing the Issues
Fixing the issues is fairly trivial. We fix the erroreous pointer algebra (or, better yet, don't use pointer algebra). We also check if the buffer is full and discard the bytes if the line is going to be too long. The fix is shown in [test_fix.c](test_fix.c).
```c
// Process the serial data here
size_t processSerial(int fd)
{
  static char buffer[BUFFER_SIZE]; // input buffer
  static char *pbuffer = buffer;  // current place in buffer
  int nbytes = 0;  // bytes read so far
  size_t size = 0;

  while((nbytes = read(fd, pbuffer, sizeof(buffer) - (pbuffer - buffer) - 1)) > 0)
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
    if((pbuffer - buffer) >= sizeof(buffer) - 1) {
      //printf("ERROR! Line too long! Discarding %li bytes of data.\n", pbuffer - buffer);
      pbuffer = buffer; // Discard the excess bytes here.
    } else {
      printf("Waste! nbytes = %i\n", nbytes);
    }
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
```

# CherryBSD_USB_Serial_Issue
We think we've foiund an issue with reading from USB to Serial devices. This is just for us to test out where the error lies and keep it documented as we do.

## Steps To Verify
- [x] Write basic description
- [ ] Build minimum code showing the behavior
- [ ] Write a method to follow to test behaviour
- [ ] Verify bahavior on differnt kernels
- [ ] Check if the potential bug has already been reported.
- [ ] Complete the detailed description of the issue
- [ ] Post on Slack and see if anyone else has been this bahvior
- [ ] Raise a ticket

## Description
- On serial code compiled for purecap mode and run on purecap kernel: `read()` returns `EFAULT` for a short period after USB to serial device is connected.
- Comiling for hybrid mode doesn't exhibit this behaviour.


## Stage 1 - Create Some Simplified Test Code to Show the Issue
1. We are currently running a test build of the kernel built (note to self: only tag kernel author once bug is confirmed) to fix this issue: https://github.com/CTSRD-CHERI/cheribsd/issues/1616
2. Version tested with:
`
root@cheribsd:~/github # uname -a
FreeBSD cheribsd.local 14.0-CURRENT FreeBSD 14.0-CURRENT #0 ugen-ep-copyincap-n256372-c708375e636: Mon Jan  9 16:16:12 GMT 2023     jrtc4@technos.cl.cam.ac.uk:/local/scratch/jrtc4/libusb-cheribuild-root/build/cheribsd-morello-purecap-build/local/scratch/jrtc4/libusb-cheribuild-root/cheribsd/arm64.aarch64c/sys/GENERIC-MORELLO arm64
`

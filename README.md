# Yet-Another-File-System

The FUSE file system implemented in C. 

## Usage 
```
gcc -g yafs.c -D_FILE_OFFSET_BITS=64 -lfuse3 -o yafs -w
```
```
./yafs  <MOUNT_POINT>
```

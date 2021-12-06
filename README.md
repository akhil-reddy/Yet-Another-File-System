# Yet-Another-File-System
FUSE file system implemented 


## Usage 
```
gcc -g yafs.c -D_FILE_OFFSET_BITS=64 -lfuse3 -o yafs -w
and then
./yafs  <MOUNT_POINT>
```

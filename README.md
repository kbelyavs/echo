# echo
A simple echo server

- server_v1.cpp , a simple non-bloking version (O_NONBLOCK), can hold ~1000/cpu with low delay
- server_v2.cpp , use select(), same performance
- server_v3.cpp , a version based on kqueue() and kevent(), low CPU utilization

First version scan all descriptors
Second in case of any read should go through all descriptors
Third do not need to copy all handlers, works faster, BSD-specific (macOS, FreeBSD)
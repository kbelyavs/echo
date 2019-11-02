# echo
A simple echo server for [C10k problem](https://en.wikipedia.org/wiki/C10k_problem) [(2)](http://www.kegel.com/c10k.html) demonstration

1. **server_nonblock.cpp** , a straight-forward approach using non-blocking socket operations (O_NONBLOCK), can hold ~1k with 1-10ms latency, 15-20% CPU core.
2. **server_select.cpp** , use select(), similar performance, slightly better latency. Have several limitations which prevent from using >1000 per thread.
3. **server_kqueue.cpp** , a version based on kqueue() and kevent(), the lowest latency, tipically <0.1ms, very low CPU utilization. Suited for c10k.

##### Notes
First version scans all descriptors in turn, complexity O(N).  
Second replies with a list of ready descriptors, but I have to copy all subscribers each time, so the complexity is O(N^2).  
Third doesn't require to copy all handlers, works faster, but BSD-specific (macOS, FreeBSD). Believe complexity is O(N).
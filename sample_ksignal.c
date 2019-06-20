#ifndef _GNU_SOURCE
#define _GNU_SOURCE             /* See feature_test_macros(7) */
#endif
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <sched.h>
#include <sys/sysinfo.h>
#include <perfmon/pfmlib_perf_event.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>
#include <assert.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <poll.h>
#include <sys/time.h>
#include <sys/resource.h>

#define debug_print(...) fprintf (stderr, __VA_ARGS__)

int fetch_signal_phy_address(char *path, int nr_cpu, unsigned long * signal_addr)
{
  int i;
  int fd;
  char buf[1024];
  if ((fd = open(path, O_RDONLY)) < 0){
    fprintf(stderr, "Can't open signal address file %s\n", path);
    return 1;
  }
  int nr_read = read (fd, buf, 1024);
  debug_print("read %d bytes %s from shim_sginal\n", nr_read, buf);
  signal_addr[0] = atol(buf);
  char *cur = buf;
  for (i=1; i<nr_cpu; i=i+1){
    while (*(cur++) != ',')
      ;
    unsigned long phyaddr = atol(cur);
    signal_addr[i] = phyaddr;
  }
  close(fd);
  return 0;
}

int map_signal_phy_address(int cpuid, unsigned long *signal_addr, unsigned long **signal)
{

  unsigned long phy_addr = signal_addr[cpuid];
  unsigned long mmap_offset = phy_addr & ~(0x1000 - 1);
  int signal_offset = phy_addr & (0x1000 - 1);
  int mmap_size = 0x1000;
  int mmap_fd;

  if ((mmap_fd = open("/dev/mem", O_RDONLY)) < 0) {
    fprintf(stderr,"Can't open /dev/mem");
    return 1;
  }
  char *mmap_addr = mmap(0, mmap_size, PROT_READ, MAP_SHARED, mmap_fd, mmap_offset);
  if (mmap_addr == MAP_FAILED) {
    fprintf(stderr,"Can't mmap /dev/mem");
    return 1;
  }
  *signal = (unsigned long *)(mmap_addr + signal_offset);
  debug_print("map cpu%d signal_addr:0x%lx to virtual_addr:%p\n", cpuid, phy_addr, *signal);
  return 0;
}

int
main(int argc, char **argv)
{
  int i;
  int nr_cpu = get_nprocs();
  unsigned long * signal_phy_addr = calloc(nr_cpu, sizeof(unsigned long));
  unsigned long **task_signals = calloc(nr_cpu, sizeof(struct shim_signal_base *));
  fetch_signal_phy_address("/sys/module/ksignal/parameters/task_signal", nr_cpu, signal_phy_addr);
  printf("Current PID:%u\n", getpid());
  for (i=0; i<nr_cpu; i++) {
    printf("CPU %d task signal at phy addr 0x%lx\n", i, signal_phy_addr[i]);
    map_signal_phy_address(i, signal_phy_addr, (task_signals + i));
    volatile unsigned long *signal = task_signals[i];
    unsigned long t = *signal;
    printf("CPU%d: pid:%u, tid:%d\n", i, (unsigned int)(t&0xffffffff), (unsigned int)(t>>32));
  }
}

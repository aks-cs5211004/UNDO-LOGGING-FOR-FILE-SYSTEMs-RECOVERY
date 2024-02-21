#include "types.h"
#include "defs.h"
#include "x86.h"
#include "fs.h"
#include "buf.h"
#include "param.h"
#include "stat.h"
#include "file.h"
#include "fcntl.h"
#include "logflag.h"

extern char end[]; // first address after kernel loaded from ELF file

static inline void 
log_test(void) {
  struct file* gtxt;
  int n;
  char buffer[512];

  if((gtxt=open("/hello.txt", O_RDONLY)) == 0) {
    panic("Unable to open /hello.txt");
  } 

  n = fileread(gtxt, buffer, 5);
  cprintf("[UNDOLOG] READ: %d %s\n", n, buffer);
  fileclose(gtxt);

  buffer[0] = '0' + PANIC_1;
  buffer[1] = '0' + PANIC_2;
  buffer[2] = '0' + PANIC_3;
  buffer[3] = '0' + PANIC_4;
  buffer[4] = '0' + PANIC_5;

  // Open for writing 
  if((gtxt = open("/hello.txt", O_WRONLY)) == 0){
    panic("Failed to create /foo/hello.txt");
  }  
  n = filewrite(gtxt, buffer, 5);
  cprintf("[UNDOLOG] WRITE: %d %s\n", n, buffer);
  fileclose(gtxt);
}

// Bootstrap processor starts running C code here.
int
main(int argc, char* argv[])
{
  mpinit();        // detect other processors
  lapicinit();     // interrupt controller
  picinit();       // disable pic
  ioapicinit();    // another interrupt controller
  uartinit();      // serial port
  ideinit();       // disk 
  tvinit();        // trap vectors
  binit();         // buffer cache
  idtinit();       // load idt register
  sti();           // enable interrupts
  iinit(ROOTDEV);  // Read superblock to start reading inodes
  initlog(ROOTDEV);  // Initialize log
  log_test();
  for(;;)
    wfi();
}

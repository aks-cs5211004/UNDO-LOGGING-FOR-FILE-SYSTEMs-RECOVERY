#define KSTACKSIZE 4096  // size of per-process kernel stack
#define NCPU          8  // maximum number of CPUs
#define NINODE       50  // maximum number of active i-nodes
#define ROOTDEV       1  // device number of file system root disk
#define MAXOPBLOCKS  10  // max # of blocks any FS op writes
#define NFILE       100  // open files per system
#define NINODE       50  // maximum number of active i-nodes
#define ROOTDEV       1  // device number of file system root disk
#define LOGSIZE      (MAXOPBLOCKS*3)  // max data blocks in on-disk log
#define NBUF         (MAXOPBLOCKS*3)  // size of disk block cache
#define FSSIZE       1000  // size of file system in blocks
#include "types.h"
#include "defs.h"
#include "param.h"
#include "fs.h"
#include "buf.h"
#include "logflag.h"


// Simple logging that allows concurrent FS system calls.
//
// A log transaction contains the updates of multiple FS system
// calls. The logging system only commits when there are
// no FS system calls active. Thus there is never
// any reasoning required about whether a commit might
// write an uncommitted system call's updates to disk.
//
// A system call should call begin_op()/end_op() to mark
// its start and end. Usually begin_op() just increments
// the count of in-progress FS system calls and returns.
// But if it thinks the log is close to running out, it
// sleeps until the last outstanding end_op() commits.
//
// The log is a physical re-do log containing disk blocks.
// The on-disk log format:
//   header block, containing block #s for block A, B, C, ...
//   block A
//   block B
//   block C
//   ...
// Log appends are synchronous.

// Contents of the header block, used for both the on-disk header block
// and to keep track in memory of logged block# before commit.
struct logheader {
  int n;
  int block[LOGSIZE];
};

struct log {
  int start;
  int size;
  int committing;  // in commit(), please wait.
  int dev;
  struct logheader lh;
};
struct log log;

static void recover_from_log(void);
static void commit();

void
initlog(int dev)
{
  if (sizeof(struct logheader) >= BSIZE)
    panic("initlog: too big logheader");

  struct superblock sb;
  readsb(dev, &sb);
  log.start = sb.logstart;
  log.size = sb.nlog;
  log.dev = dev;
  recover_from_log();
}

// Copy "new" blocks from cache to their home location
static void
install_trans(void)
{
  int tail;
  for (tail = 0; tail < log.lh.n; tail++) {
    if (LOG_FLAG == 5) {
      if (tail == log.lh.n/2) panic("[UNDOLOG] Panic in install_trans type 5");
    }
    struct buf *dbuf = bread_wr_new(log.dev, log.lh.block[tail]); // read dst
    bwrite(dbuf);  // write new to disk, undirty it, it can now be evicted!
    // note that while reading the value with bread_wr_new(), i am not incrementing ref_count
    // so no need to release it again.
  }
}


// Copy old blocks from log to their home location in case the transaction was not commmited 
static void
install_trans_old(void)
{
  int tail;
  // Just read the logs and copy its contyents to the disk.
  for (tail = 0; tail < log.lh.n; tail++) {
    struct buf *lbuf = bread(log.dev, log.start+tail+1); // read log block
    struct buf *dbuf = bread(log.dev, log.lh.block[tail]); // read dst
    // not that the avobe bread goes directly to disk
    memmove(dbuf->data, lbuf->data, BSIZE);  // copy block to dst
    bwrite(dbuf);  // write dst to disk
    // cprintf("data %s",lbuf->data);
    // since i am incrementing bnoths refcount!
    brelse(lbuf);
    brelse(dbuf);
  }
}

// Read the log header from disk into the in-memory log header
static void
read_head(void)
{
  struct buf *buf = bread(log.dev, log.start);
  struct logheader *lh = (struct logheader *) (buf->data);
  int i;
  log.lh.n = lh->n;
  for (i = 0; i < log.lh.n; i++) {
    log.lh.block[i] = lh->block[i];
  }
  brelse(buf);

}

// Write in-memory log header to disk.

static void
write_head(void)
{
  struct buf *buf = bread(log.dev, log.start);
  struct logheader *hb = (struct logheader *) (buf->data);
  int i;
  hb->n = log.lh.n;

  for (i = 0; i < log.lh.n; i++) {
    hb->block[i] = log.lh.block[i];
  }
  bwrite(buf);
  brelse(buf);
}

static void
recover_from_log(void)
{
  read_head();
  // If not committed (i.e., log.lh.n!=0, write the old blocks to disk now by install_trans_old)
  install_trans_old();
  // now committed, clear the log by setting n=0
}

// called at the start of each FS system call.
void
begin_op(void)
{
  
}

// called at the end of each FS system call.
// commits if this was the last outstanding operation.
void
end_op(void)
{
  // call commit w/o holding locks, since not allowed
  // to sleep with locks.
  commit();
}

/* DO NOT MODIFY THIS FUNCTION*/
static void
commit()
{
  if (log.lh.n > 0) {
    if (PANIC_1) {
      panic("[UNDOLOG] Panic in commit type 1");
    }
    write_head();    // Write header to disk 
    if (PANIC_2) {
      panic("[UNDOLOG] Panic in commit type 2");
    }
    install_trans(); // Now install writes to home locations    
    if (PANIC_3) {
      panic("[UNDOLOG] Panic in commit type 3");
    }
    log.lh.n = 0;
    write_head();    // Erase the transaction from the log 
    if (PANIC_4) {
      panic("[UNDOLOG] Panic in commit type 4");
    }  
  }
}

// Caller has modified b->data and is done with the buffer.
// Record the block number and pin in the cache with B_DIRTY.
// commit()/write_log() will do the disk write.
//
// log_write() replaces bwrite(); a typical use is:
//   bp = bread(...)
//   modify bp->data[]
//   log_write(bp)
//   brelse(bp)
void
log_write(struct buf *b)
{

  int i;
  if (log.lh.n >= LOGSIZE || log.lh.n >= log.size - 1)
    panic("too big a transaction");
  for (i = 0; i < log.lh.n; i++) {
    if (log.lh.block[i] == b->blockno)   // log absorbtion
      break;
  }
  log.lh.block[i] = b->blockno;
  if (i == log.lh.n)
    log.lh.n++;



  // log block: i or i+1?
  struct buf *to = bread(log.dev, log.start+i+1); 
  // READ OLD CACHED VALUE
  struct buf *from = bread_wr_old(log.dev, log.lh.block[i]);
  memmove(to->data, from->data, BSIZE);
  bwrite(to);  // write the log with old value
  // cprintf("log data to write %s",to->data);
  from->flags &= ~B_DIRTY; //undirty the old cached vlaue so that it can now be safely evicted
  brelse(from);  
  brelse(to);

  b->flags |= B_DIRTY; //PREVENT EVICTION OF tHE NEW GUY,
  // EVEN if brelse is done, since it is now dirty, it will not be evicted
  // why prevent ezvicton? to write it to the disk while commirting 

  // WHAT if old block is evicted? Nothing happens! because it is alreay written to the log!

}
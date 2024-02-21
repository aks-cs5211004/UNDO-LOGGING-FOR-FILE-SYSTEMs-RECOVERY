// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.
//
// The implementation uses two state flags internally:
// * B_VALID: the buffer data has been read from the disk.
// * B_DIRTY: the buffer data has been modified
//     and needs to be written to disk.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "fs.h"
#include "buf.h"

struct {
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // head.next is most recently used.
  struct buf head;
} bcache;

void
binit(void)
{
  struct buf *b;

  // Create linked list of buffers
  bcache.head.prev = &bcache.head;
  bcache.head.next = &bcache.head;
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  // Is the block already cached?
  for(b = bcache.head.next; b != &bcache.head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      return b;
    }
  }

  // Not cached; recycle an unused buffer.
  // Even if refcnt==0, B_DIRTY indicates a buffer is in use
  // because log.c has modified it but not yet committed it.
  for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
    if(b->refcnt == 0 && (b->flags & B_DIRTY) == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->flags = 0;
      b->refcnt = 1;
      return b;
    }
  }
  panic("bget: no buffers");
}


static struct buf*
bget_again(uint dev, uint blockno)
{
  struct buf *b;
  for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
    if(b->refcnt == 0 && (b->flags & B_DIRTY) == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->flags = 0;
      b->refcnt = 1;
      return b;
    }
  }
  panic("bget: no buffers");
}




// search for the old value of the block no., note that old value will always 
// be present beacause it was never evicted (brelse was not done for it!)
static struct buf*
bget_old(uint dev, uint blockno)
{
  struct buf *b;
  for(b = bcache.head.next; b != &bcache.head; b = b->next){
    if(b->dev == dev && b->blockno == blockno && (b->flags&B_OLD)!=0){
      return b;
    }
  }
  return b;
}


// search for the new value for a given block no. so that it 
// can now to written to the disk while installing transaction
static struct buf*
bget_new(uint dev, uint blockno)
{
  struct buf *b;
  // Is the block already cached? Is it already cached boy!
  for(b = bcache.head.next; b != &bcache.head; b = b->next){
    if(b->dev == dev && b->blockno == blockno && (b->flags&B_OLD)==0){
      return b;
    }
  }
  return b;
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;
  b = bget(dev, blockno);
  if((b->flags & B_VALID) == 0) {
    iderw(b);
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  b->flags |= B_DIRTY;
  iderw(b);
}

// If data block not present, read from disk and make 2 copy,
// If present, make 2 copy
// set one of the copy (old) as old
// RETURN the other one (new), so that it can get modified now!
// refcnt was increased for both of them while bgetting
struct buf* 
bread_wr(uint dev, uint blockno) {
  struct buf* bnew;
  struct buf* bold;
  bnew=bget(dev,blockno);
  bold=bget_again(dev,blockno); // Just copy the data, flag now from bnew and flag
  memmove(bold->data, bnew->data, BSIZE);


  // Do we need to copy flags????????????
  memmove(&bold->flags,(const void * )&bnew->flags, sizeof(int));

  // cprintf("data copy %s",bold->data);
  if((bnew->flags & B_VALID) == 0) {
    iderw(bnew);
    iderw(bold);
    bold->flags|=B_OLD;
  }
  else {
    bold->flags|=B_OLD;
  }
  return bnew;
}


// Read the old value of block to keep it in the log
struct buf* 
bread_wr_old(uint dev, uint blockno) {
  struct buf* bold;
  bold=bget_old(dev,blockno);
  return bold;
}


struct buf* 
bread_wr_new(uint dev, uint blockno) {
  struct buf* bnew;
  bnew=bget_new(dev,blockno);
  return bnew;
}

// Release a buffer.
// Move to the head of the MRU list.
void
brelse(struct buf *b)
{
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head.next;
    b->prev = &bcache.head;
    bcache.head.next->prev = b;
    bcache.head.next = b;
  }
}
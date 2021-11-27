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


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13 //number of hash bucket

struct {
  struct spinlock lock[NBUCKET];
  struct buf buf[NBUF];
  //struct spinlock buflock;


  struct buf head[NBUCKET];
} bcache;

void
binit(void)
{
  struct buf *b;

  for(int i = 0; i < NBUCKET; i++) {
      initlock(&bcache.lock[i], "bcachebucket");
      bcache.head[i].prev = &bcache.head[i];
      bcache.head[i].next = &bcache.head[i];
  }

  for(b = bcache.buf; b < bcache.buf + NBUF; b++){
    b->next = bcache.head[0].next;
    b->prev = &bcache.head[0];
    initsleeplock(&b->lock, "buffer");
    bcache.head[0].next->prev = b;
    bcache.head[0].next = b;
  }
}

// moves bucket from previous bucket to bucket
// locks of both buckets have to be locked
void
move_bucket(struct buf *b, int bucket)
{
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head[bucket].next;
    b->prev = &bcache.head[bucket];
    bcache.head[bucket].next->prev = b;
    bcache.head[bucket].next = b;
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno) {
    struct buf *b;

    int bucket = blockno % NBUCKET;
    acquire(&bcache.lock[bucket]);

    // Is the block already cached?
    for (b = bcache.head[bucket].next; b != &bcache.head[bucket]; b = b->next) {
        if (b->dev == dev && b->blockno == blockno) {
            b->refcnt++;
            release(&bcache.lock[bucket]);
            acquiresleep(&b->lock);
            return b;
        }
    }

    // Recycle the least recently used (LRU) unused buffer.
    for (int i = (bucket + 1) % NBUCKET; i != bucket; i = (i + 1) % NBUCKET){

      acquire(&bcache.lock[i]);
      //not cached, start recycle unused buffer
      for(b = bcache.head[i].prev; b != &bcache.head[i] ; b = b->prev){
        if(b->refcnt == 0) {
          // move buffer from bucket[i] to bucket[bucket]
          b->dev = dev;
          b->blockno = blockno;
          b->valid = 0;
          b->refcnt = 1;
          // move this free buffer to the head of current bucket
          b->prev->next = b->next;
          b->next->prev = b->prev;
          release(&bcache.lock[i]);
          b->prev = &bcache.head[bucket];
          b->next = bcache.head[bucket].next;
          //b->next->prev = b;
          //b->prev->next = b;
          bcache.head[bucket].next->prev = b;
          bcache.head[bucket].next = b;
          release(&bcache.lock[bucket]);
          acquiresleep(&b->lock);
          return b;
        }
      }
    release(&bcache.lock[i]);
    }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  uint bucket = b->blockno % NBUCKET;
  acquire(&bcache.lock[bucket]);

  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head[bucket].next;
    b->prev = &bcache.head[bucket];
    bcache.head[bucket].next->prev = b;
    bcache.head[bucket].next = b;

  }
  
  release(&bcache.lock[bucket]);
}

void
bpin(struct buf *b) {
  int bucket = b->blockno % NBUCKET;
  acquire(&bcache.lock[bucket]);
  b->refcnt++;
  release(&bcache.lock[bucket]);
}

void
bunpin(struct buf *b) {
  int bucket = b->blockno % NBUCKET;
  acquire(&bcache.lock[bucket]);
  b->refcnt--;
  release(&bcache.lock[bucket]);
}

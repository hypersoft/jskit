#ifndef SEEK_SET
    #define SEEK_SET	0	/* Seek from beginning of file.  */
    #define SEEK_CUR	1	/* Seek from current position.  */
    #define SEEK_END	2	/* Seek from end of file.  */
#endif

typedef struct StringBuffer {

   char * address;
   size_t index;
   size_t allocated;
   size_t limit;

} StringBuffer;

StringBuffer NewStringBuffer() {
    return (StringBuffer) { 0,0,0,0 };
}

// Clone StringBuffer values
// Cast StringBuffer pointer to structure and return by value.
// requires compiler option -fno-strict-aliasing
StringBuffer StringBufferProperties(register StringBuffer *buffer) {
    return (StringBuffer) *buffer;
}

// Setup a new string buffer
int StringBufferInit(register StringBuffer * source, size_t allocate, size_t limit) {

    if (limit && allocate > limit) return 0;

   source->address = (char *) malloc(allocate*sizeof(unsigned char));

   source->allocated = allocate;
   source->index = 0;
   source->limit = limit;

    if (source->address) {

      source->address[0] = 0; // initialize ANSI compatible "string"

      return source->allocated;

    }

    return 0;

}

// #requiring <string.h>, <stdlib.h>

// Append a string to the current buffer position (indicated by index)
// This procedure guards against possible memory leak with realloc, employing
// a result status checked temp variable. The procedure is also capable of
// overlapped memory i/o.

int StringBufferWrite(register StringBuffer * buffer, const char * src) {

  if (!buffer || !src) return 0;

  size_t len = strlen(src) + 1;

  register size_t terminationNode = ( len + buffer->index );

  if ( buffer->limit && ( terminationNode > buffer->limit ) ) return 0;

  if ( buffer->allocated < terminationNode ) {

      void * NewMemoryBlock = realloc(buffer->address, terminationNode);
      if (!NewMemoryBlock) return 0;

      buffer->address = (char *) NewMemoryBlock;
      buffer->allocated = terminationNode;

  }

  memmove((buffer->address + buffer->index), src, len);

  buffer->index += --len; // set buffer index 1 position ahead of terminator

  return 1;

}

// Erase everything from buffer index to extent, if extent exceeds
// allocated space, the request is truncated. If extent is null, extent is
// rectified to buffer allocated through index
size_t StringBufferClear(register StringBuffer * buffer, register size_t extent) {

    // I cannot possibly clear "nothing" so if extent is zero,
    // clear  buffer content from: address+index to address+allocated

    if ( ! buffer->address || ! buffer->allocated ) return 0;

    register size_t id = ( extent + buffer->index );

    if ( id > buffer->allocated || ! extent ) {
        id = ( buffer->allocated - buffer->index );
    }

    extent = id;

    for ( id = buffer->index; id < extent; id++ ) {

        buffer->address[id] = 0; // turn off the bits

    }

    return 1;

}

// Free the address pointer, unset all members
void StringBufferRelease(register StringBuffer * buffer) {

  if (!buffer->address || ! buffer->allocated) return;

  free(buffer->address);

  *(buffer) = NewStringBuffer();

}

// Set the buffer index stdio fseek() style and (+2)
// return the resulting address located at the index.
// ZERO == FAIL
char * StringBufferSeek(register StringBuffer * b, size_t i, int o) {

  // Use stdio SEEK_* flags for o (origin)

  register size_t t; // resident temp once again..

  if (!b->address || !b->allocated) return 0;

  if (o == SEEK_CUR) {
     t = b->index + i;
     if (t > b->allocated || t < 0) return 0;
     b->index = t;
     return (b->address+t);
  }

  if (o == SEEK_SET) {
     if (i < 0 || i > b->allocated) return 0;
     b->index = i;
     return(b->address+i);
  }

  if (o == SEEK_END) {
     if (i > 0) return 0;
     t = b->allocated + i;
     if (t < 0) return 0;
     b->index = t;
     return(b->address+t);
  }

  return 0;

}

// Clone StringBuffer values and data
// Clone StringBuffer values, create an identical buffer,
// copy the source data to the new buffer and assign the
// new buffer address to the StringBuffer copy, returning
// the duplicate by value
StringBuffer StringBufferDuplicate(register StringBuffer *buffer) {

    register StringBuffer duplicate = StringBufferProperties(buffer);

    duplicate.address = strncpy(

        malloc(
            duplicate.allocated
        ),

        duplicate.address,
        duplicate.allocated

    );

    return duplicate;

}

// Directly set the allocated size of the buffer
// Return: non-zero success
int StringBufferExtent(register StringBuffer *buffer, size_t size) {

    if (buffer->limit && size > buffer->limit) return 0;

    void *t = 0;

    if (buffer->address) {

        if (size == 0) { // destructive request
            free(buffer->address);
            buffer->allocated = 0;
            buffer->index = 0;
            buffer->address = 0;
            return 1;
        }

        t = realloc(buffer->address, size);

    } else {

        if (size == 0) {
            // make sure allocated is zero
            buffer->allocated = 0; return 0;
        }

        t = malloc(size);

    }

    if (!t) return 0;

    buffer->address = (char *) t;
    buffer->address[0] = 0;
    buffer->allocated = size;

    return 1;

}
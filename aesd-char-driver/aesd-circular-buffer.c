/**
 * @file aesd-circular-buffer.c
 * @brief Functions and data related to a circular buffer imlementation
 *
 * @author Dan Walkes
 * @date 2020-03-01
 * @copyright Copyright (c) 2020
 *
 */

#ifdef __KERNEL__
#include <linux/string.h>
#else
#include <string.h>
#endif

#include "aesd-circular-buffer.h"

/**
 * @param buffer the buffer to search for corresponding offset.  Any necessary locking must be performed by caller.
 * @param char_offset the position to search for in the buffer list, describing the zero referenced
 *      character index if all buffer strings were concatenated end to end
 * @param entry_offset_byte_rtn is a pointer specifying a location to store the byte of the returned aesd_buffer_entry
 *      buffptr member corresponding to char_offset.  This value is only set when a matching char_offset is found
 *      in aesd_buffer.
 * @return the struct aesd_buffer_entry structure representing the position described by char_offset, or
 * NULL if this position is not available in the buffer (not enough data is written).
 */
struct aesd_buffer_entry *aesd_circular_buffer_find_entry_offset_for_fpos(struct aesd_circular_buffer *buffer,
            size_t char_offset, size_t *entry_offset_byte_rtn )
{
    //find out which entry the char is in
    size_t total_s = 0;
    uint8_t i;
    struct aesd_buffer_entry *entry;
    AESD_CIRCULAR_BUFFER_FOREACH(entry, buffer, i) {
        total_s = total_s + entry->size;
        //check if char_offset is in this entry
        if(total_s > char_offset) {
            break; //leave loop with entry, index, and total_s
        }
    }
    
    //check for failure
    if(i == AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
        return NULL;
    
    //set the byte_rtn value from "modulus"
    size_t byte_rtn_i = entry->size - (total_s - char_offset);
    *entry_offset_byte_rtn = byte_rtn_i;
    
    //set the return struct
    return entry;
}

/**
* Adds entry @param add_entry to @param buffer in the location specified in buffer->in_offs.
* If the buffer was already full, overwrites the oldest entry and advances buffer->out_offs to the
* new start location.
* Any necessary locking must be handled by the caller
* Any memory referenced in @param add_entry must be allocated by and/or must have a lifetime managed by the caller.
*/
void aesd_circular_buffer_add_entry(struct aesd_circular_buffer *buffer, const struct aesd_buffer_entry *add_entry)
{
    //set the add_entry to the in offset
    uint8_t in_temp = buffer->in_offs;
    buffer->entry[in_temp] = add_entry;
    
    if(buffer->full) {
        buffer->out_offs = buffer->out_offs + 1;
    }
    
    //increase the in offset
    buffer->in_offs = in_temp + 1;
    
    //ensure in wraps back around
    if(buffer->in_offs >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
        buffer->in_offs = 0;
    
    if(buffer->out_offs >= AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
        buffer->out_offs = 0;
    
    //determine if full
    if(buffer->in_offs == buffer->out_offs)
        buffer->full = true;
}

/**
* Initializes the circular buffer described by @param buffer to an empty struct
*/
void aesd_circular_buffer_init(struct aesd_circular_buffer *buffer)
{
    memset(buffer,0,sizeof(struct aesd_circular_buffer));
}

/* @(#)$Id: buffer.c,v 1.3 1996/11/13 00:40:34 seks Exp $ */

/* Undernet Channel Service (X)
 * Copyright (C) 1995-2002 Robin Thellend
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * The author can be contact by email at <csfeedback@robin.pfft.net>
 *
 * Please note that this software is unsupported and mostly
 * obsolete. It was replaced by GNUworld/CMaster. See
 * http://gnuworld.sourceforge.net/ for more information.
 */

/* These routines were first developed for the telnet3d project.
 * Special thanks to Danny Mitchell (wildthang@irc) for his help
 */

#include "h.h"

static struct buffer_block *avail=NULL;
/*static unsigned long MEM_buffers=0;*/
static unsigned long NB_avail_buffer_blocks=0;
static unsigned long NB_alloc_buffer_blocks=0;


/* get_buffer_block() by SeKs <intru@step.polymtl.ca>
 * returns a ptr to an empty buffer_block or NULL if malloc()
 * returns an error.
 */
struct buffer_block *get_buffer_block(void)
{
  register struct buffer_block *block;

	/*  I would like to move this to the GetMemory
	**  and linklist functions as soon as we get them
	**  finished.  Mainly for use in keeping track
	**  of overall memory usage, and such.
	**  just a reminder.	--DVM
	*/

  if(avail==NULL){
    block=(struct buffer_block *)malloc(sizeof(struct buffer_block));
    block->next=NULL;
  }else{
    block=avail;
    avail=avail->next;
    NB_avail_buffer_blocks--;
  }

  block->offset_read=0;
  block->offset_write=0;
  block->next=NULL;

  NB_alloc_buffer_blocks++;

  return block;
}

/* return_buffer_block() by SeKs <intru@step.polymtl.ca>
 * returns the given "block" to the "system". Blocks are not
 * free()'d. They are kept in order to be re-allocated by
 * get_buffer_block().
 */
void return_buffer_block(struct buffer_block *block)
{
  block->next=avail;
  avail=block;
  NB_avail_buffer_blocks++;
  NB_alloc_buffer_blocks--;
}

/* copy_from_buffer() by SeKs <intru@step.polymtl.ca>
 * copies characters from the buffer starting at the buffer_block
 * '*block' into 'string' until a char in 'stop' is encountered or 
 * 'max' bytes are read.
 * Blocks that are completely copied are returned to the system with
 * return_buffer_block().
 * Returns the number of characters that were copied.
 */
int
copy_from_buffer(struct buffer_block **block,char *string,char stop,int max)
{
  register struct buffer_block *tmp;
  register int count=0;

  if(block==NULL || *block==NULL || string==NULL)
    return -1;

  while(count<max && *block!=NULL){
    if((*block)->offset_read == (*block)->offset_write){
      tmp=*block;
      *block=(*block)->next;
      return_buffer_block(tmp);
      break;
    }
    count++;

    *string=(*block)->buf[(*block)->offset_read++];

    if((*block)->offset_read == BUFFER_BLOCK_SIZE){
      tmp=*block;
      *block=(*block)->next;
      return_buffer_block(tmp);
    }

    string++;
    if(stop!='\0' && stop==*(string-1))
      break;
  }
  *string='\0';

  return count;
}


/* look_in_buffer() by SeKs <intru@step.polymtl.ca>
 * Same as copy_from_buffer() except that the buffer remains unchanged
 * when the function returns.
 * ** Added offset counter otherwise offset_read IS changed when returned -DVM
 */
int
look_in_buffer(struct buffer_block **block,char *string,char stop,int max)
{
  register int count=0;
  register int offset=0;	/* added DVM */

  if(block==NULL || *block==NULL || string==NULL)
    return -1;

  if((*block)->offset_read == (*block)->offset_write){
    return_buffer_block(*block);
    *block=NULL;
  }

  while(count<max && *block!=NULL){
    if((*block)->offset_read+offset == (*block)->offset_write)
      break;

    count++;
    *string=(*block)->buf[(*block)->offset_read+offset]; 
    offset++;
    if((*block)->offset_read+offset == BUFFER_BLOCK_SIZE){
      block=&(*block)->next;
      offset=0;
    }
    string++;
    if(stop!='\0' && stop==*(string-1))
      break;
  }
  *string='\0';

  return count;
}


/* copy_to_buffer() by SeKs <intru@step.polymtl.ca>
 * adds a string to the specified buffer.
 * New buffer_blocks are requested when necessary.
 * Returns the number of characters in the buffer after the
 * new string is added.
 * arguments are:
 *  ( struct buffer_block **block, char *format, args... )
 */
long copy_to_buffer(struct buffer_block **block, char *string,int length)
{
  register long count=0;

  if(block==NULL || length<=0)
    return -1;

  if(*block==NULL)
    *block=get_buffer_block();

  while((*block)->next!=NULL){
    count+=(BUFFER_BLOCK_SIZE)-(*block)->offset_read;
    block=&(*block)->next;
  }
  count+=(*block)->offset_write-(*block)->offset_read;

  while(length--){
    count++;
    (*block)->buf[(*block)->offset_write++]=*(string++);
    if((*block)->offset_write == BUFFER_BLOCK_SIZE){
      (*block)->offset_write=-1;
      (*block)->next=get_buffer_block();
      block=&(*block)->next;
      if(*block==NULL){
        break;
      }
    }
  }

  return count;
}

/* zap_buffer() by SeKs <intru@step.polymtl.ca>
 * clears the buffer starting at the buffer_block '*block'
 * return -1 if block is NULL
 *         0 otherwise
 */
int zap_buffer(struct buffer_block **block)
{
  struct buffer_block *tmp;

  if(block==NULL)
    return -1;

  while((tmp=*block)!=NULL){
    *block=(*block)->next;
    return_buffer_block(tmp);
  }

  return 0; 
}

/* find_char_in_buffer() by SeKs <intru@step.polymtl.ca>
 * returns 1 if any character of string 'findit' is present within the first
 * 'max' characters of the buffer.
 *  returns 0 otherwise.
 */
int find_char_in_buffer(struct buffer_block **block, char findit,int max)
{
  register int offset;
  register int count=0;

  if( (block == NULL) || (*block == NULL))
    return 0; /* DVM */

  offset=(*block)->offset_read;
loop:
  if(offset == (*block)->offset_write)
    return 0;
  if(findit == (*block)->buf[offset])
    return 1;
  if(++count > max)
    return 0;
  if(++offset == BUFFER_BLOCK_SIZE){
    block=&(*block)->next;
    if(*block == NULL)
      return 0;
    offset=0;
  }
  goto loop;
}

/* skip_char_in_buffer() by SeKs <intru@step.polymtl.ca>
 * flushes the first 'n' characters in buffer starting at
 * buffer_block '*block'
 */
int skip_char_in_buffer(struct buffer_block **block, int n)
{
  register struct buffer_block *tmp;
  register int count=0;

  while(count<n){
    if((*block)->offset_read == (*block)->offset_write){
      tmp=*block;
      *block=(*block)->next;
      return_buffer_block(tmp);
      break;
    }
    count++;
    (*block)->offset_read++;
    if((*block)->offset_read == BUFFER_BLOCK_SIZE){
      tmp=*block;
      *block=(*block)->next;
      return_buffer_block(tmp);
      if(*block==NULL)
        break;
    }
  }

  return count;
}


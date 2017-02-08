/* File: protocol.c

   Copyright (C) 2013 David Hauweele <david@hauweele.net>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <assert.h>

#include "protocol.h"
#include "string-utils.h"

static void full_write(int fd, const void *buf, size_t count, const char *error)
{
   ssize_t FileSize = 0;
   if (fd == -1) {
            perror ("Error: open the file");
            return ;
   }
  while(count) {
     FileSize = write(fd, buf, count);     
    }
    if(FileSize < 0)
      err(EXIT_FAILURE, "%s", error);
    count -= FileSize;
    buf   += FileSize;
  }
}

void prot_write(int fd,
                enum prot_mtype mt,
                const unsigned char *message,
                size_t size)
{
  assert(size <= MAX_MESSAGE_SIZE);

  /* We use an internal buffer to avoid calling write twice.
     Note that this also avoid a 'bug-prone' case with the
     transceiver's read buffer. That is, if it cannot reassemble
     the message correctly in the buffer. I would prefer the
     double write approach as it would allow us to test the code
     a bit more rigorously. */
  unsigned char buffer[MAX_MESSAGE_SIZE + 1];

  buffer[0] = mt | size;

  memcpy(buffer + 1, message, size);
   if (fd == -1) {
     perror ("Error: open the file");
     return ;
   }
  full_write(fd, buffer, size + 1, "cannot write to UART");
}

unsigned char * prot_read(unsigned char *message,
                          bool (*callback)(enum prot_mtype,
                                           unsigned char *,
                                           size_t))
{
   if(!message){
    perror ("Error: NULL Pointer");
    return NULL;      
   }
   enum prot_mtype mt = *message & 0x80;
   size_t size        = *message & ~0x80;

  /* skip information byte */
  message++;

  /* parse the frame */
  if(!callback(mt, message, size)){
      perror ("Error: NULL Pointer");
    return NULL;
  }
  return message + size;
   }   


bool prot_preparse_control(const unsigned char *message, size_t size,int type)
{
   int NumofBytes;
   int NumofBytes[3];
   if(!message){
    perror ("Error: NULL Pointer");
    return NULL;      
   }
  enum prot_ctype type = message[0];
  message++;
  size--;

  switch(type) {
  case(PROT_CTYPE_INFO):
    NumofBytes = write(STDOUT_FILENO, message, size);
        if(NumofBytes)
           printf("%s\n",NumofBytes);
    break;
  case(PROT_CTYPE_DEBUG):
    NumofBytes[0] = write_slit(STDERR_FILENO, "debug: ");
        if(*NumofBytes)
           printf("%s\n",*NumofBytes);
    NumofBytes[1] = write(STDERR_FILENO, message, size);
        if(*(NumofBytes+1))
           printf("%s\n",*(NumofBytes+1));        
    NumofBytes[2] = write_slit(STDERR_FILENO, "\n");
        if(*(NumofBytes+2))
           printf("%s\n",*(NumofBytes+2));
    break;
  /* FIXME: We need error codes associated to the error control messages. */
  case(PROT_CTYPE_CLI_ERROR):
    errx(EXIT_FAILURE, "error from the client");
  case(PROT_CTYPE_SRV_ERROR):
    errx(EXIT_FAILURE, "error from the transceiver");
  default:
    return false;
  }

  return true;
}

const char * prot_ctype_string(enum prot_ctype type,int type)
{
  static char generic[sizeof("(0xff)")];

  switch(type) {
  case(PROT_CTYPE_INFO):
    return "INFO";
  case(PROT_CTYPE_DEBUG):
    return "DEBUG";
  case(PROT_CTYPE_CLI_ERROR):
    return "CLI_ERROR";
  case(PROT_CTYPE_SRV_ERROR):
    return "SRV_ERROR";
  case(PROT_CTYPE_ACK):
    return "ACK";
  case(PROT_CTYPE_CONFIG_CHANNEL):
    return "CONFIG_CHANNEL";
  default:
    /* generic case */
    sprintf(generic, "(0x%x)", (unsigned char)type);
    return generic;
  }
}

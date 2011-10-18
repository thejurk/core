/*
   Copyright (C) Cfengine AS

   This file is part of Cfengine 3 - written and maintained by Cfengine AS.

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; version 3.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA

  To the extent this program is licensed as part of the Enterprise
  versions of Cfengine, the applicable Commerical Open Source License
  (COSL) may apply to this file if you as a licensee so wish it. See
  included file COSL.txt.
*/

/*********************************************************************/
/*                                                                   */
/*  TOOLKITS: network library                                        */
/*                                                                   */
/*********************************************************************/

#include "cf3.defs.h"
#include "cf3.extern.h"

/*************************************************************************/

int SendTransaction(int sd,char *buffer,int len,char status)

{ char work[CF_BUFSIZE];
  int wlen;

memset(work, 0, sizeof(work));

if (len == 0)
   {
   wlen = strlen(buffer);
   }
else
   {
   wlen = len;
   }

if (wlen > CF_BUFSIZE-CF_INBAND_OFFSET)
   {
   CfOut(cf_error, "", "SenTransaction: wlen (%d) > %d - %d", wlen, CF_BUFSIZE, CF_INBAND_OFFSET);
   FatalError("SendTransaction software failure");
   }

snprintf(work,CF_INBAND_OFFSET,"%c %d",status,wlen);

memcpy(work+CF_INBAND_OFFSET,buffer,wlen);

Debug("Transaction Send[%s][Packed text]\n",work);

if (SendSocketStream(sd,work,wlen+CF_INBAND_OFFSET,0) == -1)
   {
   return -1;
   }

return 0;
}

/*************************************************************************/

int ReceiveTransaction(int sd,char *buffer,int *more)

{ char proto[CF_INBAND_OFFSET+1];
  char status = 'x';
  unsigned int len = 0;

memset(proto,0,CF_INBAND_OFFSET+1);

if (RecvSocketStream(sd,proto,CF_INBAND_OFFSET,0) == -1)   /* Get control channel */
   {
   return -1;
   }

sscanf(proto,"%c %u",&status,&len);

Debug("Transaction Receive [%s][%s]\n",proto,proto+CF_INBAND_OFFSET);

if (len > CF_BUFSIZE - CF_INBAND_OFFSET)
   {
   CfOut(cf_error,"","Bad transaction packet -- too long (%c %d) Proto = %s ",status,len,proto);
   return -1;
   }

if (strncmp(proto,"CAUTH",5) == 0)
   {
   Debug("Version 1 protocol connection attempted - no you don't!!\n");
   return -1;
   }

if (more != NULL)
   {
   switch(status)
      {
      case 'm': *more = true;
          break;
      default: *more = false;
      }
   }

return RecvSocketStream(sd,buffer,len,0);
}

/*************************************************************************/

int RecvSocketStream(int sd,char buffer[CF_BUFSIZE],int toget,int nothing)

{ int already, got;
  static int fraction;

Debug("RecvSocketStream(%d)\n",toget);

if (toget > CF_BUFSIZE-1)
   {
   CfOut(cf_error,"","Bad software request for overfull buffer");
   return -1;
   }

for (already = 0; already != toget; already += got)
   {
   got = recv(sd,buffer+already,toget-already,0);

   if (got == -1 && errno == EINTR)
      {
      continue;
      }

   if (got == -1)
      {
      CfOut(cf_verbose,"recv","Couldn't recv");
      return -1;
      }

   if (got == 0)   /* doesn't happen unless sock is closed */
      {
      Debug("Transmission empty or timed out...\n");
      fraction = 0;
      break;
      }

   Debug("    (Concatenated %d from stream)\n",got);
   }

buffer[already] = '\0';
return already;
}


/*************************************************************************/

int SendSocketStream(int sd,char buffer[CF_BUFSIZE],int tosend,int flags)

{ int sent,already=0;

do
   {
   Debug("Attempting to send %d bytes\n",tosend-already);

   sent = send(sd,buffer+already,tosend-already,flags);

   if (sent == -1 && errno == EINTR)
      {
      continue;
      }

   if (sent == -1)
      {
      CfOut(cf_verbose,"send","Couldn't send");
      return -1;
      }

   Debug("SendSocketStream, sent %d\n",sent);
   already += sent;
   }
while (already < tosend);

return already;
}

/*************************************************************************/


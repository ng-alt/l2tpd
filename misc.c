/*
 * Layer Two Tunnelling Protocol Daemon
 * Copyright (C) 1998 Adtran, Inc.
 *
 * Mark Spencer
 *
 * This software is distributed under the terms
 * of the GPL, which you should have received
 * along with this source.
 *
 * Miscellaneous but important functions
 *
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#if defined(SOLARIS)
# include <varargs.h>
#endif
#include <netinet/in.h>
#include "l2tp.h"

void log(int level, const char *fmt,...)
{
	char buf[256];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	vfprintf(stderr,fmt, args);
	fflush(stderr);
	openlog(BINARY, LOG_PID, LOG_DAEMON);
	syslog(level, "%s", buf);
	va_end(args);
}

void set_error(struct call *c, int error, const char *fmt,...)
{
	va_list args;
	va_start(args, fmt);
	c->error = error;
	c->result = RESULT_ERROR;
	c->needclose = -1;
	vsnprintf(c->errormsg, sizeof(c->errormsg),fmt, args);
	if (c->errormsg[strlen(c->errormsg)-1] == '\n') c->errormsg[strlen(c->errormsg)-1]=0;
	va_end(args);
}

struct buffer *new_buf(int size) {
	struct buffer *b = malloc(sizeof(struct buffer));
	if (!b || !size || size<0 ) return NULL;
	b->rstart=malloc(size);
	if (!b->rstart) {
		free(b);
		return NULL;
	}
	b->start=b->rstart;
	b->rend=b->rstart+size-1;
	b->len = size;
	b->maxlen = size;
	return b;
}

inline void recycle_buf(struct buffer *b) 
{
	b->start = b->rstart;
	b->len = b->maxlen;
}

#define bufferDumpWIDTH 16
void bufferDump(buf, buflen)
	char	*buf;
	int	buflen;
{
	int		i = 0, j = 0;
	unsigned char	line[2 * bufferDumpWIDTH + 1], *c; /* we need TWO characters to DISPLAY ONE byte */

	for (i = 0; i < buflen / bufferDumpWIDTH; i++) {
		c = line;
		for (j = 0; j < bufferDumpWIDTH; j++) {
			sprintf(c, "%02x ", (buf[i * bufferDumpWIDTH + j]) & 0xff);
			c++; c++; /* again two characters to display ONE byte */
		}
		*c = '\0';
		log(LOG_WARN, "%s: buflen=%d, buffer[%d]: *%s*\n",__FUNCTION__, buflen, i, line); 	}

	c = line;
	for (j = 0; j < buflen % bufferDumpWIDTH; j++) {
		sprintf(c, "%02x ", buf[ (buflen /bufferDumpWIDTH) * bufferDumpWIDTH + j] & 0xff);
		c++; c++;
	}
	if (c != line) {
		*c = '\0';
		log(LOG_WARN, "%s:             buffer[%d]: *%s*\n",__FUNCTION__ , i, line);
	}
}

void do_packet_dump(struct buffer *buf) {
	int x;
	unsigned char *c=buf->start;
	printf("packet dump: \nHEX: { ");
	for (x=0;x<buf->len;x++) {
		printf("%.2X ",*c);
		c++;
	};
	printf("}\nASCII: { ");
	c=buf->start;
	for (x=0; x<buf->len;x++) {
		if (*c>31 && *c<127) {
			putchar(*c);
		} else {
			putchar(' ');
		}
		c++;
	}
	printf("}\n");
}

inline void swaps(void *buf_v, int len)
{
	/* Reverse byte order (if proper to do so) 
	   to make things work out easier */
	int x;
	_u16 *tmp=(_u16 *)buf_v;
	for (x=0;x<len/2;x++) 
		tmp[x]=ntohs(tmp[x]);	
}



inline void toss(struct buffer *buf) {
	/*
	 * Toss a frame and free up the buffer that contained it
	 */
	 
	free(buf->rstart);
	free(buf);
}

inline void safe_copy(char *a, char *b, int size)
{
	/* Copies B into A (assuming A holds MAXSTRLEN bytes)
	  safely */
	strncpy(a,b,MIN(size,MAXSTRLEN-1));
	a[MIN(size,MAXSTRLEN-1)]='\000';
}

struct ppp_opts *add_opt(struct ppp_opts *option, char *fmt, ...)
{
	va_list args;
	struct ppp_opts *new, *last;
	new=(struct ppp_opts *)malloc(sizeof(struct ppp_opts));
	if (!new) {
		log(LOG_WARN, "%s : Unable to allocate ppp option memory.  Expect a crash\n" ,__FUNCTION__);
		return NULL;
	}
	new->next=NULL;	
	va_start(args, fmt);
	vsnprintf(new->option,sizeof(new->option),fmt, args);
	va_end(args);
	if (option) {
		last=option;
		while(last->next) last=last->next;
		last->next = new;
		return option;
	} else return new;
}
void opt_destroy(struct ppp_opts *option) 
{
	struct ppp_opts *tmp;
	while(option) {
		tmp=option->next;
		free(option);
		option=tmp;
	};
}



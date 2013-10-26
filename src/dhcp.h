#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <netinet/in.h>

#include "array.h"

#ifndef DHCPD_DHCP_H_
#define DHCPD_DHCP_H_

/* A DHCP message consists of a fixed-length header and a variable-length
 * option part:
 *
 * 0                   1                   2                   3
 * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |     op (1)    |   htype (1)   |   hlen (1)    |   hops (1)    |
 * +---------------+---------------+---------------+---------------+
 * |                            xid (4)                            |
 * +-------------------------------+-------------------------------+
 * |           secs (2)            |           flags (2)           |
 * +-------------------------------+-------------------------------+
 * |                          ciaddr  (4)                          |
 * +---------------------------------------------------------------+
 * |                          yiaddr  (4)                          |
 * +---------------------------------------------------------------+
 * |                          siaddr  (4)                          |
 * +---------------------------------------------------------------+
 * |                          giaddr  (4)                          |
 * +---------------------------------------------------------------+
 * |                                                               |
 * |                          chaddr  (16)                         |
 * |                                                               |
 * |                                                               |
 * +---------------------------------------------------------------+
 * |                                                               |
 * |                          sname   (64)                         |
 * +---------------------------------------------------------------+
 * |                                                               |
 * |                          file    (128)                        |
 * +---------------------------------------------------------------+
 * |                                                               |
 * |                          options (variable)                   |
 * +---------------------------------------------------------------+
 */

/* Macros to obtain flags from header */
#define DHCP_MSG_F_OP(m)      ((uint8_t*)((m)+0))
#define DHCP_MSG_F_HTYPE(m)   ((uint8_t*)((m)+1))
#define DHCP_MSG_F_HLEN(m)    ((uint8_t*)((m)+2))
#define DHCP_MSG_F_HOPS(m)    ((uint8_t*)((m)+3))
#define DHCP_MSG_F_XID(m)     ((uint32_t*)((m)+4))
#define DHCP_MSG_F_SECS(m)    ((uint16_t*)((m)+8))
#define DHCP_MSG_F_FLAGS(m)   ((uint16_t*)((m)+10))
#define DHCP_MSG_F_CIADDR(m)  ((uint32_t*)((m)+12))
#define DHCP_MSG_F_YIADDR(m)  ((uint32_t*)((m)+16))
#define DHCP_MSG_F_SIADDR(m)  ((uint32_t*)((m)+20))
#define DHCP_MSG_F_GIADDR(m)  ((uint32_t*)((m)+24))
#define DHCP_MSG_F_CHADDR(m)  ((char*)((m)+28))
#define DHCP_MSG_F_MAGIC(m)   ((uint8_t*)((m)+236))
#define DHCP_MSG_F_OPTIONS(m) ((uint8_t*)((m)+240))

/* Useful magic numbers */
#define DHCP_MSG_LEN    (576)
#define DHCP_MSG_HDRLEN (240)
#define DHCP_MSG_MAGIC  ((uint8_t[]){ 99, 130, 83, 99 })

/* Macro to check magic cookie */
#define DHCP_MSG_MAGIC_CHECK(m) ((m)[0] == 99 && (m)[1] == 130 && (m)[2] == 83 && (m)[3] == 99)

/* Macros to obtain fields from options */
#define DHCP_OPT_F_CODE(o) ((uint8_t*)(o))
#define DHCP_OPT_F_LEN(o)  ((uint8_t*)((o)+1))
#define DHCP_OPT_F_DATA(o) ((char*)((o)+2))

/* Return expression to calculate length of option */
#define DHCP_OPT_LEN(o) (((uint8_t*)(o))[0] != 255 && ((uint8_t*)(o))[0] != 0 ? ((uint8_t*)(o))[1] + 2 : 1)
#define DHCP_OPT_NEXT(o) (((uint8_t*)(o))+DHCP_OPT_LEN(o))
#define DHCP_OPT_CONT(o, l) {\
		(l) += DHCP_OPT_LEN(o);\
		(o) = DHCP_OPT_NEXT(o);\
	}
#define DHCP_OPT(m) {\
		.code = *DHCP_OPT_F_CODE(m),\
		.len = *DHCP_OPT_F_LEN(m),\
		.data = ((*DHCP_OPT_F_LEN(m)) > 0 ? DHCP_OPT_F_DATA(m) : NULL)\
	}

enum dhcp_opt_type
{
	DHCP_OPT_STUB = 0,
	DHCP_OPT_NETMASK = 1,
	DHCP_OPT_ROUTER = 3,
	DHCP_OPT_DNS = 6,
	DHCP_OPT_REQIPADDR = 50,
	DHCP_OPT_LEASETIME = 51,
	DHCP_OPT_MSGTYPE = 53,
	DHCP_OPT_SERVERID = 54,
	DHCP_OPT_END = 255
};

enum dhcp_msg_type
{
	DHCPDISCOVER = 1,
	DHCPOFFER = 2,
	DHCPREQUEST = 3,
	DHCPDECLINE = 4,
	DHCPACK = 5,
	DHCPNAK = 6,
	DHCPRELEASE = 7,
	DHCPINFORM = 8
};

struct dhcp_opt
{
	uint8_t code;
	uint8_t len;
	char *data;
};

struct dhcp_msg
{
	uint8_t *data;
	size_t length;

	enum dhcp_msg_type type;

	struct sockaddr *source;
};

static inline void dhcp_msg_prepare(uint8_t *reply, uint8_t *original)
{
	*DHCP_MSG_F_XID(reply) = *DHCP_MSG_F_XID(original);
	*DHCP_MSG_F_HTYPE(reply) = *DHCP_MSG_F_HTYPE(original);
	*DHCP_MSG_F_HLEN(reply) = *DHCP_MSG_F_HLEN(original);
	*DHCP_MSG_F_OP(reply) = (*DHCP_MSG_F_OP(reply) == 2 ? 1 : 2);
	ARRAY_COPY(DHCP_MSG_F_MAGIC(reply), DHCP_MSG_MAGIC, 4);
	ARRAY_COPY(DHCP_MSG_F_CHADDR(reply), DHCP_MSG_F_CHADDR(original), 16);
}

static inline bool dhcp_opt_next(uint8_t **cur, struct dhcp_opt *opt, uint8_t *end)
{
	if (*DHCP_OPT_F_CODE(*cur) == DHCP_OPT_END)
		return false;

	if (opt != NULL)
		*opt = (struct dhcp_opt)DHCP_OPT(*cur);

	/* Overflow detection */
	if ((*cur) + DHCP_OPT_LEN(*cur) >= end)
		return false;

	*cur = DHCP_OPT_NEXT(*cur);

	return true;
}

void dhcp_msg_dumpf(struct dhcp_msg *msg, FILE *stream);

#endif

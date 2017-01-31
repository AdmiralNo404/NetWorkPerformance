#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#define BUF_SZ 256
#define SERVER 0
#define CLIENT 1

enum msg_type_t {
    MSG_TYPE_INVALID = 0,
    MSG_TYPE_GET = 1,
    MSG_TYPE_GET_ERR = 2,
    MSG_TYPE_GET_RESP = 3,
    MSG_TYPE_GET_ACK = 4,
    MSG_TYPE_FINISH = 5,
    MSG_TYPE_MAX
};

static const char * const str_map[MSG_TYPE_MAX+1] = {
    "invalid",
    "get",
    "get_err",
    "get_resp",
    "get_ack",
    "finish",
    "max"
};

struct msg_t {
    enum msg_type_t msg_type;      /* message type */
    int cur_seq;                   /* current seq number */
    int max_seq;                   /* max seq number */
    int payload_len;               /* length of payload */
    unsigned char payload[BUF_SZ]; /* buffer for data */
};

void initMsg(struct msg_t *msg, int type, int seq, int max, int len, char* content) {
	msg->msg_type = type;
	msg->cur_seq = seq;
	msg->max_seq = max;
	msg->payload_len = len;
	strcpy(msg->payload, content);
}

void printMsg(struct msg_t *msg, int host) {
	if (host == 0) {
		printf("server: RX %s %d %d %d\n", 
		str_map[msg->msg_type], 
		msg->cur_seq, 
		msg->max_seq, 
		msg->payload_len);
	} else {
		printf("client: RX %s %d %d %d\n", 
		str_map[msg->msg_type], 
		msg->cur_seq, 
		msg->max_seq, 
		msg->payload_len);
	}
}

struct msmt {
	long int timediff;		/* download time in seconds */
	int no_msg;				/* number of messages */
	int no_byte;			/* total bytes transfered */
};

void printMsmt(struct msmt *m) {
	long int sec = m->timediff / 1000000;
	long int usec = m->timediff % 1000000;
	printf("Download time: %ld microseconds (%ld.%06ld seconds)\nNo. of messages: %d\nTotal bytes transferred: %d\n",
			m->timediff,
			sec,
			usec,
			m->no_msg,
			m->no_byte);
}

#endif /* __MESSAGE_H__ */

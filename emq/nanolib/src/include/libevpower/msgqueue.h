extern  int mqcreate(int flag, int maxmsg, long int msgsize, const char *name);
extern  int mqsend(const char *name, char *msg, int msglen, unsigned int prio);
extern  char *mqreceive(const char *name);
extern  char *mqreceive_timed(const char *name, int lenth, int tval);

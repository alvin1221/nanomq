
/***
 *
 *
 ***/

#ifndef _LIBEV_API_H_
#define _LIBEV_API_H_

#include <curl/curl.h>
#include <stdbool.h>

enum api_return_type {
	API_NONE = 0,
	API_FILE,
	API_BUFF,
};

/**
 * struct api_return - used to store the reply received by the server
 * @type: tells if the reply has to be stored on a file or in memory
 * @filename: if type is API_FILE this is the filename
 * @buff.ptr: if type is API_BUFF this points to the buffer containing the reply
 * @buff.len: if type is API_BUFF this is the len of the buffer pointed by
 *	      buff.ptr
 */
struct api_return {
	enum api_return_type type;
	int ret;
	union {
		/* used if type == API_FILE */
		const char *filename;
		/* used if type == API_BUFF */
		struct {
			char *ptr;
			size_t len;
		} buff;
	} u;
};

struct api_conn {
	CURL *hdl;
	char *proto;
	char *host;
	char *key;
	char *secret;
};

int api_send_file(const char *proto, const char *host, const char *path,
		  const char *content_type, const char *filename,
		  const char *file_reply, char *additional_headers,
		  const char *key, const char *secret);
int api_send_buff(const char *proto, const char *host, const char *path,
				  const char *buff, const char *key, const char *secret);
char *api_post_file_glassfish(const char *proto, const char *host,
							  const char *url, const char *file_path);
int api_send_url(const char *proto, const char *host, const char *path);
int api_send_request(const char *proto, const char *host, const char *path,
		     char **reply, const char *key, const char *secret);
struct api_conn *api_conn_create(const char *proto, const char *host,
				 const char *key, const char *secret,
				 long timeout);
long api_conn_run(struct api_conn *conn, struct api_return *api_data,
		  bool sign, const char *path, const char *method,
		  const char *content_type, char *additional_headers,
		  const char *body, int body_len);
int api_conn_close(struct api_conn *conn);
char *api_request_buff(const char *proto, const char *host, const char *path,
					   const char *buff, const char *key, const char *secret,
					   const char *reply);
#endif /* _LIBEV_API_H_ */

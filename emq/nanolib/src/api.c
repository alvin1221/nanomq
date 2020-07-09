
/***
 *
 * Copyright (C) 2016 EV Power Ltd
 *
 * API 
 ***/

#include "libevpower.h"

#include <libevpower/api.h>
#include <libevpower/file.h>
#include <libevpower/system.h>

#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <polarssl/md5.h>
#include <polarssl/sha256.h>
#include <curl/curl.h>
#include <linux/if_ether.h>
#include <libevpower/msgqueue.h>

#include <json-c/json.h>

#define ev_curl_slist_append(_list, _header) ({                        \
	debug_msg("curl_slist_append(): %s", _header);\
	curl_slist_append(_list, _header);\
})

#define USE_SHA256			0
#define API_WRONG_TIMESTAMP_CODE	432
#define API_TIME_ENDPOINT		"/time"

static char curl_err[CURL_ERROR_SIZE];
static char tmp_buff[4096];

/**
 * api_write_output - print the server reply to a file or buffer
 * @ptr		object return by libcurl
 * @data	the pointer of apt_return which you need to write by yourself
 */
static size_t api_write_output(char *ptr, size_t size, size_t nmemb,
			       void *data)
{
	struct api_return *api_data = (struct api_return *)data;
	size_t len = size * nmemb;
	FILE *fp;
	debug_msg("server reply: %s %d", ptr, len);
	if(strncmp(ptr, "charging", 8) == 0)
		mqsend("/dashboard.checkin", ptr, len, 10);
	if (!data) {
		debug_msg("reply ignored");
		return len;
	}

	/* depending on the user request, the reply is treated differently */
	switch (api_data->type) {
	case API_NONE:
	case API_FILE:
		/* for API_NONE we only need to save the reply in a buffer for
		 * debugging purposes
		 */
		if (api_data->type == API_NONE)
			break;
		api_data->u.buff.len = len;
		api_data->u.buff.ptr = ptr;					//important

		fp = fopen(api_data->u.filename, "a");
		if (!fp)
			break;

		len = fwrite(ptr, size, nmemb, fp);
		fclose(fp);

		break;
	case API_BUFF:
		/* if this is not the first chunk consider that one byte is used
		 * for the final NULL char
		 */
		api_data->u.buff.len = len;
		api_data->u.buff.ptr = ptr;					//important	 only authorize return http buff
		/*
		if (strncmp(ptr, "update", 6) == 0)
		{
			debug_msg("sync system time from %ld", time(0));
			tmp = strchr(ptr, '2');
			memcpy(date, tmp, 19);
			date[19] = '\0';
			cmd_frun("date -s %s", date);
			debug_msg("current system time: %ld", time(0));
			//stime();
		}
		*/
		break;
	}

	return len;
}

/**
 * api_get_time - query time API and return timestamp
 * @proto: protocol to use to query the API. Expect http or https
 * @host: the API server hostname
 *
 * Returns the timestamp (unix format) obtained from the timeAPI or (time_t)-1
 * in case of failure
 */
static time_t api_get_time(const char *proto, const char *host)
{
	CURL *handle = curl_easy_init();
	struct json_object *obj;
	time_t ret = (time_t)-1;
	struct api_return api_ret;
	struct tm api_time;
	const char *ptr;
	int r;

	api_ret.type = API_BUFF;
	api_ret.u.buff.ptr = NULL;
	api_ret.u.buff.len = 0;

	snprintf(tmp_buff, sizeof(tmp_buff), "%s://%s" API_TIME_ENDPOINT, proto,
		 host);
	debug_msg("url: %s", tmp_buff);
	curl_easy_setopt(handle, CURLOPT_URL, tmp_buff);

	curl_easy_setopt(handle, CURLOPT_FAILONERROR, true);
	curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, curl_err);
	if (strcmp(proto, "https") == 0) {
		curl_easy_setopt(handle, CURLOPT_USE_SSL, CURLUSESSL_ALL);
		curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, true);
		curl_easy_setopt(handle, CURLOPT_CAPATH, CERTS_PATH);
	}
	curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, api_write_output);
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, &api_ret);
	curl_easy_setopt(handle, CURLOPT_TIMEOUT, 20);
	curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT, 10);

	r = curl_easy_perform(handle);
	curl_easy_cleanup(handle);
	if (r != CURLE_OK) {
		debug_msg("curl error (%d) %s", r, curl_err);
		goto out;
	}

	if (!api_ret.u.buff.ptr) {
		debug_msg("empty reply from time API ?!");
		goto out;
	}

	debug_msg("json (%d bytes):\n%s", api_ret.u.buff.len,
		  api_ret.u.buff.ptr);

	obj = json_tokener_parse(api_ret.u.buff.ptr);
	obj = json_object_object_get(obj, "time");

	ptr = json_object_to_json_string(obj);
	debug_msg("time=%s", ptr);

	ptr = strptime(ptr, "\"%FT%TZ\"", &api_time);
	json_object_put(obj);

	free(api_ret.u.buff.ptr);

	if (!ptr || (*ptr != '\0')) {
		debug_msg("time parsing failed (%s)\n", ptr);
		goto out;
	}

	ret = mktime(&api_time);

out:
	return ret;
}

int api_nonce_gen(time_t now, unsigned char *output)
{
	unsigned char rand[32];
	int i, r;

	/* create a random array by using the eth0 mac address for the initial 6
	 * bytes and random numbers for the rest
	 */
	if (sys_get_mac_addr("eth0", rand) < 0)
		return -1;

	srandom(now);
	for (i = ETH_ALEN; i < (int)sizeof(rand); i += 2) {
		r = random();
		rand[i] = r & 0x000000FF;
		rand[i + 1] = (r & 0x0000FF00) >> 8;
	}
	md5(rand, sizeof(rand), output);

	return 0;
}

void api_sign_gen(const char *secret, const char *auth, const char *path,
		  const char *body, int body_len, unsigned char *output)
{
	sha256_context sha256_ctx;

	/* create the signature for this request. Use SHA256-HMAC */
	sha256_hmac_starts(&sha256_ctx, (unsigned char *)secret, strlen(secret),
			   USE_SHA256);

	/* skip the initial 15 chars ("Authorization: ") in the string because
	 * only the auth value has to be hashed
	 */
	debug_msg("hashing: auth '%s' (len=%d)", auth,  strlen(auth));
	sha256_hmac_update(&sha256_ctx, (unsigned char *)auth, strlen(auth));

	debug_msg("hashing: path '%s' (len=%d)", path, strlen(path));
	sha256_hmac_update(&sha256_ctx, (unsigned char *)path, strlen(path));

	debug_msg("hashing: body (len=%d)", body_len);
	sha256_hmac_update(&sha256_ctx, (unsigned char *)body, body_len);

	sha256_hmac_finish(&sha256_ctx, output);
}

static int api_debug_cb(CURL NG_UNUSED(*handle), curl_infotype type, char *data,
			size_t size, void NG_UNUSED(*userptr))
{
	char *buff;

#if 0
	debug_msg("type: %d", type);
	if (type == 3) {
		debug_msg("server returns: %s", data);
		if (strncmp(data, "charging", 8) == 0)
		{
			data[9] = '\0';
			mqsend("/dashboard.checkin", data, strlen(data), 10);
		}
	}
#endif
	buff = malloc(size + 1);
	if (!buff)
		return 0;

	memcpy(buff, data, size);
	buff[size] = '\0';

	debug_msg("data: %s", buff);

	free(buff);

	return 0;
}

/**
 * nearly as same as api_call, but with return value from server api
 * only used for canStartCharging API
 * api_request_buff - call an API and possibly send a body content
 * reply - copy the http return string to reply string
 * */
char *api_request_buff(const char *proto, const char *host, const char *path,
					   const char *buff, const char *key, const char *secret,
					   const char *reply)
{
	struct api_return api_data;
	api_data.type = API_BUFF;
	api_call(proto, host, path, "POST", "application/json", buff,
			strlen(buff), &api_data,
			"application/x-www-form-urlencoded", key, secret);
	if (reply != NULL && api_data.u.buff.ptr != NULL) {
		memcpy(reply, api_data.u.buff.ptr, strlen(api_data.u.buff.ptr));
		return api_data.u.buff.ptr;
	} else {
		reply = NULL;
		return reply;
	}
}

/**
 * api_call - call an API and possibly send a body content
 * @proto: protocol to use. Expected either "http" or "https"
 * @host: the API server hostname
 * @path: path to use for the API call, must start with '/'
 * @method: HTTP method to be used. Expected either "GET" or "POST"
 * @content_type: content type in MIME format
 * @body: the body to send. Can be NULL but body_len must be 0
 * @body_len: the len of the body. Can be 0
 * @api_data: object containing the instructions about where to store the reply.
 *	      Can be NULL.
 * @additional_headers: other HTTP headers to send to the server
 * @key: the API key
 * @secret: the API secret used to create the signature
 */
int api_call(const char *proto, const char *host, const char *path,
	     const char *method, const char *content_type, const char *body,
	     int body_len, struct api_return *api_data, char *additional_headers,
	     const char *key, const char *secret)
{
	struct api_conn *conn;
	long run_ret = 0;
	int ret = 0;

	conn = api_conn_create(proto, host, key, secret, 15);
	if (!conn)
		return -1;

	run_ret = api_conn_run(conn, api_data, false, path, method,
			       content_type, additional_headers, body,
			       body_len);

	ret = api_conn_close(conn);

	if (run_ret != 200)
		return -1;

	if (ret != 0)
		return -1;

	return 0;
}

/**
 * api_post_file_glassfish - call an API and pass a file as content with POST
 * only applys to EV's glassfish API server
 * @url: url to request includes server hostname and uri path
 * @file_path: the path of the file to be send  
 */
char *api_post_file_glassfish(const char *proto, const char *host,
							  const char *url, const char *file_path)
{
	struct curl_httppost *post = NULL;
	struct curl_httppost *last = NULL;
	struct api_return api_data;
	struct stat st;
	char buff[256];
	long err_code;
	CURL *curl;
	int res;

	api_data.type = API_FILE;
	if (!proto) {
		debug_msg("No protocol provided!");
		return NULL;
	}

	if (!host) {
		debug_msg("No host provided!");
		return NULL;
	}

	if ((strcmp(proto, "http") != 0) && (strcmp(proto, "https") != 0)) {
		debug_msg("invalid proto: %s", proto);
		return NULL;
	}

	/* get the size in byte of the file so that a buffer that will contain
	 * the whole content can be allocated
	 */
	if (stat(file_path, &st) < 0) {
		debug_msg("cannot run stat() on file '%s': %s",
			  file_path, strerror(errno));
		return NULL;
	}

	curl = curl_easy_init();
	if(curl)
	{
		curl_formadd(&post, &last, CURLFORM_COPYNAME, "upload",
					 CURLFORM_FILE, file_path, CURLFORM_END);
 
		curl_formadd(&post, &last, CURLFORM_COPYNAME, "name", 
					 CURLFORM_COPYCONTENTS, "123", CURLFORM_END);

		if (strcmp(proto, "https") == 0) {
			curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, true);
			curl_easy_setopt(curl, CURLOPT_CAPATH, CERTS_PATH);
		}


		sprintf(buff, "%s://%s%s", proto, host, url);
		debug_msg("post file %s to %s", file_path, buff);
		curl_easy_setopt(curl, CURLOPT_URL, buff);
		curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30);
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, api_write_output);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &api_data);
 
		res = curl_easy_perform(curl);
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &err_code);
		debug_msg("curl returned HTTP status: %lu", err_code);
		if(res)
		{
			curl_easy_cleanup(curl);
			curl_formfree(post);
			return NULL;
		}
		curl_formfree(post);
	}
	else
	{
		curl_easy_cleanup(curl);
		curl_formfree(post);
		return NULL;
	}

	curl_easy_cleanup(curl);
	return api_data.u.buff.ptr;
}

/**
 * api_send_file - call an API and pass a file as content with POST
 * @proto: protocol to use. Expected either "http" or "https"
 * @host: the API server hostname
 * @path: path to use for the API call, must start with '/'
 * @content_type: content type in MIME format
 * @filename: name of the file to send
 * @file_reply: file where the reply has to be written. Can be NULL
 * @additional_headers: additional HTTP headers to pass during the call. The
 *			headers must be ';'-separated
 * @key: the API key
 * @secret: the API secret used to create the signature
 */
int api_send_file(const char *proto, const char *host, const char *path,
		  const char *content_type, const char *filename,
		  const char *file_reply, char *additional_headers,
		  const char *key, const char *secret)
{
	struct api_return api_data;
	struct stat st;
	char *buff;
	FILE *fp;
	int ret;

	debug_msg("sending %s via API, path: %s, content-type: %s reply: %s",
		  filename, path, content_type, file_reply);

	/* get the size in byte of the file so that a buffer that will contain
	 * the whole content can be allocated
	 */
	if (stat(filename, &st) < 0) {
		debug_msg("cannot run stat() on file '%s': %s",
			  filename, strerror(errno));
		return -1;
	}

	debug_msg("allocate buffer for %jd bytes", (intmax_t)st.st_size + 1);

	buff = malloc(st.st_size + 1);
	if (!buff) {
		debug_msg("cannot allocate buffer to read API request file");
		return -1;
	}

	fp = fopen(filename, "r");
	if (!fp) {
		debug_msg("cannot open API request file");
		ret = -1;
		goto out;
	}

	/* copy the content of the file into the buffer */
	ret = fread(buff, 1, st.st_size, fp);
	if ((ret < st.st_size) && !feof(fp)) {
		debug_msg("error while reading API request file");
		ret = -1;
		goto close;
	}

	buff[st.st_size] = '\0';

	/* instruct the API callback about where to put the reply */
	api_data.type = API_NONE;
	if (file_reply) {
		api_data.type = API_FILE;
		api_data.u.filename = file_reply;
	}

	ret = api_call(proto, host, path, "POST", content_type, buff, ret,
		       &api_data, additional_headers, key, secret);

close:
	fclose(fp);
out:
	free(buff);

	return ret;
}

/**
 * api_send_request - send a GET request to the API with no body
 * @proto: protocol to use. Expected either "http" or "https"
 * @host: the API server hostname
 * @path: path to use for the API call, must start with '/'
 * @reply: output variable. Will point to the reply buffer
 * @key: the API key
 * @secret: the API secret used to create the signature
 *
 * The reply buffer is returned only on success and must be free'd by the callee
 */
int api_send_request(const char *proto, const char *host, const char *path,
		     char **reply, const char *key, const char *secret)
{
	struct api_return api_data;
	int ret;

	api_data.type = API_NONE;
	if (reply) {
		api_data.type = API_BUFF;
		api_data.u.buff.ptr = NULL;
		api_data.u.buff.len = 0;
	}

	ret = api_call(proto, host, path, "GET", "application/json", NULL, 0,
		       &api_data, NULL, key, secret);

	if (reply)
		*reply = api_data.u.buff.ptr;

	return ret;
}

int api_send_buff(const char *proto, const char *host, const char *path,
		  const char *buff, const char *key, const char *secret)
{
	struct api_return api_data;
	api_data.type = API_BUFF;
	//debug_msg("%s, %d", buff, strlen(buff));
	return api_call(proto, host, path, "POST", "application/x-www-form-urlencoded", buff,
		   strlen(buff), &api_data,
		   NULL, key, secret);
}

int api_send_url(const char *proto, const char *host, const char *path)
{
	struct api_return api_data;
	api_data.type = API_BUFF;

	return api_call(proto, host, path, "POST", "application/json", NULL,
			0, &api_data, NULL, NULL, NULL);
}

/**
 * api_create_conn - create new connection handle
 * @proto: protocol to use. Expected either "http" or "https"
 * @host: the API server hostname
 * @key: the API key
 * @secret: the API secret used to create the signature
 * @timeout: the maximum time that request allow to take
 */
struct api_conn *api_conn_create(const char *proto, const char *host,
				 const char *key, const char *secret,
				 long timeout)
{
	struct api_conn *conn;

	if (!proto) {
		debug_msg("No protocol provided!");
		return NULL;
	}

	if (!host) {
		debug_msg("No host provided!");
		return NULL;
	}

	if ((strcmp(proto, "http") != 0) && (strcmp(proto, "https") != 0)) {
		debug_msg("invalid proto: %s", proto);
		return NULL;
	}

	conn = calloc(1, sizeof(*conn));
	if (!conn)
		return NULL;

	conn->hdl = curl_easy_init();
	if (!conn->hdl)
		goto err;

	conn->proto = strdup(proto);
	if (!conn->proto)
		goto err;

	conn->host = strdup(host);
	if (!conn->host)
		goto err;

	if (key) {
		conn->key = strdup(key);
		if (!conn->key)
			goto err;
	}

	if (secret) {
		conn->secret = strdup(secret);
		if (!conn->secret)
			goto err;
	}

	curl_easy_setopt(conn->hdl, CURLOPT_ERRORBUFFER, curl_err);

	if (strcmp(proto, "https") == 0) {
		curl_easy_setopt(conn->hdl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
		curl_easy_setopt(conn->hdl, CURLOPT_SSL_VERIFYPEER, true);
		curl_easy_setopt(conn->hdl, CURLOPT_CAPATH, CERTS_PATH);
	}

	curl_easy_setopt(conn->hdl, CURLOPT_WRITEFUNCTION, api_write_output);
	curl_easy_setopt(conn->hdl, CURLOPT_TIMEOUT, timeout);
	curl_easy_setopt(conn->hdl, CURLOPT_CONNECTTIMEOUT, 10);
#ifdef LIBEV_DEBUG
	curl_easy_setopt(conn->hdl, CURLOPT_DEBUGFUNCTION, api_debug_cb);
	curl_easy_setopt(conn->hdl, CURLOPT_VERBOSE, true);
#endif

	return conn;
err:
	if (conn->hdl)
		curl_easy_cleanup(conn->hdl);
	free(conn->secret);
	free(conn->key);
	free(conn->host);
	free(conn->proto);
	free(conn);

	return NULL;
}

/**
 * api_conn_run - perform curl operation
 * @api_conn: object containing the connection information and handle
 * @api_data: object containing the instructions about where to store the reply.
 *	      Can be NULL.
 * @sign: if true, add auth and signature to the header field
 * @path: path to use for the API call, must start with '/'
 * @method: HTTP method to be used. Expected either "GET" or "POST"
 * @content_type: content type in MIME format
 * @additional_headers: other HTTP headers to send to the server
 * @body: the body to send. Can be NULL but body_len must be 0
 * @body_len: the len of the body. Can be 0
 *
 * Return: negative value for curlcode error (request error)
 *         postive value for HTTP error code (no request error)
 *         200 for successful HTTP return
 */
long api_conn_run(struct api_conn *conn, struct api_return *api_data,
		  bool sign, const char *path, const char *method,
		  const char *content_type, char *additional_headers,
		  const char *body, int body_len)
{
	struct curl_slist *headers;
	unsigned char output[32];
	long err_code = 0;
	int attempts = 0;
	long ret = 0;
	time_t now;
	int i,len;

	if (!conn)
		return -1;

	if (path[0] != '/') {
		debug_msg("path must start with '/' (%s)", path);
		return -1;
	}

	if ((strcmp(method, "GET") != 0) && (strcmp(method, "POST") != 0)) {
		debug_msg("invalid method: %s", method);
		return -1;
	}

	now = time(NULL);
retry:
	ret = 0;
	/* the reply might be received in multiple fragments and the receiving
	 * function will be append them one after the other. For this
	 * reason the file that is going to contain the reply must be cleared
	 * before sending the request.
	 *
	 * We must also ensure the file is deleted after having got a timestamp
	 * error.
	 */
	if ((api_data->type == API_FILE) && file_exists(api_data->u.filename))
		file_trunc_to_zero(api_data->u.filename);

	debug_msg("connection attempt %d", attempts);
	headers = NULL;

	/* compose and append content-length and type to header list */
	snprintf(tmp_buff, sizeof(tmp_buff), "Content-Length: %d", body_len);

	headers = ev_curl_slist_append(headers, tmp_buff);
	snprintf(tmp_buff, sizeof(tmp_buff), "Content-Type: %s", content_type);
	headers = ev_curl_slist_append(headers, tmp_buff);

	if ((sign == true) && conn->key && conn->secret) {
		snprintf(tmp_buff, sizeof(tmp_buff),
			 "Authorization: key=%s,timestamp=%lu,nonce=",
			 conn->key, now);

		if (api_nonce_gen(now, output) < 0)
			return -1;

		/* concatenate the nonce to the authorization string */
		len = strlen(tmp_buff);
		for (i = 0; i < 16; i++)
			snprintf(tmp_buff + len + (i * 2),
				 sizeof(tmp_buff) - len - (i * 2), "%02x",
				 output[i]);
		headers = ev_curl_slist_append(headers, tmp_buff);

		api_sign_gen(conn->secret, tmp_buff + 15, path, body, body_len,
			     output);

		snprintf(tmp_buff, sizeof(tmp_buff), "Signature: ");

		/* concatenate the computed hash */
		len = strlen(tmp_buff);
		for (i = 0; i < 32; i++)
			snprintf(tmp_buff + len + (i * 2),
				 sizeof(tmp_buff) - len - (i * 2), "%02x",
				 output[i]);
		headers = ev_curl_slist_append(headers, tmp_buff);
	}

	/* append to the headers list any header passed as argument to this
	 * function
	 */
	if (additional_headers)
		headers = ev_curl_slist_append(headers, additional_headers);

	snprintf(tmp_buff, sizeof(tmp_buff), "%s://%s%s", conn->proto,
		 conn->host, path);

	curl_easy_setopt(conn->hdl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(conn->hdl, CURLOPT_URL, tmp_buff);
	if (strcmp(method, "POST") == 0) {
		curl_easy_setopt(conn->hdl, CURLOPT_POST, 1);
		curl_easy_setopt(conn->hdl, CURLOPT_POSTFIELDS, body);
		curl_easy_setopt(conn->hdl, CURLOPT_POSTFIELDSIZE, strlen(body));
	}
	curl_easy_setopt(conn->hdl, CURLOPT_WRITEDATA, api_data);

	debug_msg("sending HTTP request..");
	ret = curl_easy_perform(conn->hdl);
	curl_slist_free_all(headers);
	if (ret != CURLE_OK) {
		sys_logger("API", "connection error: %s (%d) to %s", curl_err,
		ret, tmp_buff);
		debug_msg("curl error (%ld): %s", ret, curl_err);
		/* Return negative curl error code */
		api_data->u.buff.ptr = NULL;
		api_data->ret = ret;
		ret = -ret;
		goto out;
	}

	curl_easy_getinfo(conn->hdl, CURLINFO_RESPONSE_CODE, &err_code);
	debug_msg("curl returned HTTP status: %lu", err_code);
	ret = err_code;

	/* wrong timestamp detected. get a new timestamp from timeAPI (but do
	 * not try more than three times in total)
	 */

	if ((err_code == API_WRONG_TIMESTAMP_CODE) && (attempts < 3)) {
		debug_msg("wrong timestamp, adjusting time..");
		do {
			now = api_get_time(conn->proto, conn->host);
			attempts++;
		} while  ((now == (time_t)-1) && (attempts < 3));

		/* try again only if we managed to get a proper timestamp */
		if (now != (time_t)-1)
			goto retry;
	}

	if (err_code != 200) {
		sys_logger("API", "call to %s returned HTTP error %d", tmp_buff,
			   err_code);
		debug_msg("curl call error");
	}

out:
	return ret;
}

/**
 * api_conn_close - close curl connecton
 * @api_conn: object containing the connection information and handle
 */
int api_conn_close(struct api_conn *conn)
{
	if (!conn)
		return -1;

	curl_easy_cleanup(conn->hdl);
	free(conn->proto);
	free(conn->host);

	if (conn->key)
		free(conn->key);

	if (conn->secret)
		free(conn->secret);

	free(conn);

	return 0;
}

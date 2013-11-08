// rb_mse_api.c

#include "stdlib.h"
#include "string.h"
#include "sys/queue.h"

#include "rb_mse_api.h"

const char mse_api_call_url[] = "/api/contextaware/v1/location/clients/";




/*
 *                               rb_mse_api_pos
 */
 
struct rb_mse_api_pos{
  /// String that has to be released
  char * string_tofree;

  /// position in string_tofree that holds the mac's floor
  const char * floor;
  /// position in string_tofree that holds the mac's build
  const char * build;
  /// position in string_tofree that holds the mac's zone
  const char * zone;

  /// Last time the mac was see.
  time_t last_timestamp;
};


void rb_mse_pos_destroy(struct rb_mse_api_pos *pos){
  free(pos->string_tofree);
  free(pos);
}

const char * rb_mse_pos_curl_ret_msg(struct rb_mse_api_pos *pos)
{
  return pos->string_tofree;
}

const char * rb_mse_pos_floor(const struct rb_mse_api_pos *pos){return pos->floor;}
const char * rb_mse_pos_build(const struct rb_mse_api_pos *pos){return pos->build;}
const char * rb_mse_pos_zone(const struct rb_mse_api_pos *pos){return pos->zone;}
time_t       rb_mse_pos_ts(const struct rb_mse_api_pos * pos){return pos->last_timestamp;}

/*
 *                     mse_api structs definitions
 */

struct message_tailq_node{
  /// Message stored in the node
  char * msg;

  /// STAILQ entry
  STAILQ_ENTRY(message_tailq_node) next;
};

struct rb_mse_api
{
  /// Curl handler.
  CURL *hnd;
  char * url;
  size_t mac_pos_in_url;
  char * userpwd;
  /// Message lists queue.
  STAILQ_HEAD(,message_tailq_node) msg_list;
};

/*
 *                               CURL CALLBACKS
 */

size_t write_function( char *ptr, size_t size, size_t nmemb, void *userdata)
{
  if(size*nmemb < sizeof("<?xml ") || 0!=memcmp("<?xml ",ptr,strlen("<?xml ")))
  {
    // string is a header, not the data
    return size*nmemb;
  }

  struct message_tailq_node * node = malloc(sizeof(struct message_tailq_node));
  if(node)
  {
    struct rb_mse_api * rb_mse = (struct rb_mse_api *)userdata;
    node->msg = malloc(size*nmemb + sizeof('\0'));
    if(node->msg){
      memcpy(node->msg,ptr,size*nmemb);
      node->msg[size*nmemb]='\0';
      STAILQ_INSERT_TAIL(&rb_mse->msg_list,node,next);
    }else{
      free(node);
      node=0;
    }
  }
  return node && node->msg ? size*nmemb : 0;
}


/*
 *                                 rb_mse API
 */ 

struct rb_mse_api * rb_mse_api_new()
{
  struct rb_mse_api * rb_mse = calloc(1,sizeof(struct rb_mse_api));
  if(rb_mse)
  {
    rb_mse->hnd = curl_easy_init();
    STAILQ_INIT(&rb_mse->msg_list);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_WRITEDATA, rb_mse);               /* void passed to WRITEFUNCTION */
    curl_easy_setopt(rb_mse->hnd, CURLOPT_WRITEFUNCTION, write_function);   /* function called for each data received */ 

    curl_easy_setopt(rb_mse->hnd, CURLOPT_PROXY, NULL);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_NOPROGRESS, 1);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_HEADER, "Accept: application/json");
    curl_easy_setopt(rb_mse->hnd, CURLOPT_FAILONERROR, 0);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_UPLOAD, 0);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_DIRLISTONLY, 0);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_APPEND, 0);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_NETRC, 0);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_FOLLOWLOCATION, 0);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_UNRESTRICTED_AUTH, 0);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_TRANSFERTEXT, 0);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_PROXYUSERPWD, NULL);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_NOPROXY, NULL);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_RANGE, NULL);
    /* curl_easy_setopt(rb_mse->hnd, CURLOPT_ERRORBUFFER, 0x7fff57485c50); [REMARK] */
    curl_easy_setopt(rb_mse->hnd, CURLOPT_TIMEOUT, 0);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_REFERER, NULL);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_AUTOREFERER, 0);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_USERAGENT, "curl/7.19.7 (x86_64-redhat-linux-gnu) libcurl/7.23.1 OpenSSL/1.0.1c zlib/1.2.6");
    curl_easy_setopt(rb_mse->hnd, CURLOPT_FTPPORT, NULL);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_LOW_SPEED_LIMIT, 0);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_LOW_SPEED_TIME, 0);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_MAX_SEND_SPEED_LARGE, (curl_off_t)0);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_MAX_RECV_SPEED_LARGE, (curl_off_t)0);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_RESUME_FROM_LARGE, (curl_off_t)0);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_COOKIE, NULL);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_HTTPHEADER, 0 /*"Accept: application/json"*/);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_SSLCERT, NULL);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_SSLCERTTYPE, NULL);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_SSLKEY, NULL);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_SSLKEYTYPE, NULL);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_KEYPASSWD, NULL);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_SSH_PRIVATE_KEYFILE, NULL);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_SSH_PUBLIC_KEYFILE, NULL);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_SSH_HOST_PUBLIC_KEY_MD5, NULL);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_SSL_VERIFYHOST, 0);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_MAXREDIRS, 50);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_CRLF, 0);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_QUOTE, NULL);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_POSTQUOTE, NULL);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_PREQUOTE, NULL);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_WRITEHEADER, NULL);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_COOKIEFILE, NULL);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_COOKIESESSION, 0);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_SSLVERSION, 0);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_TIMECONDITION, 0);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_TIMEVALUE, 0);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_CUSTOMREQUEST, NULL);
    /* curl_easy_setopt(rb_mse->hnd, CURLOPT_STDERR, 0x7f2abcc03860); [REMARK] */
    curl_easy_setopt(rb_mse->hnd, CURLOPT_HTTPPROXYTUNNEL, 0);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_INTERFACE, NULL);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_KRBLEVEL, NULL);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_TELNETOPTIONS, NULL);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_RANDOM_FILE, NULL);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_EGDSOCKET, NULL);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_CONNECTTIMEOUT, 0);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_ENCODING, NULL);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_FTP_CREATE_MISSING_DIRS, 0);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_IPRESOLVE, 0);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_FTP_ACCOUNT, NULL);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_IGNORE_CONTENT_LENGTH, 0);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_FTP_SKIP_PASV_IP, 0);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_FTP_FILEMETHOD, 0);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_FTP_ALTERNATIVE_TO_USER, NULL);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_SSL_SESSIONID_CACHE, 1);
    /* curl_easy_setopt(rb_mse->hnd, CURLOPT_SOCKOPTFUNCTION, 0x405c90); [REMARK] */
    /* curl_easy_setopt(rb_mse->hnd, CURLOPT_SOCKOPTDATA, 0x7fff57485670); [REMARK] */
    curl_easy_setopt(rb_mse->hnd, CURLOPT_POSTREDIR, 0);
  }

  return rb_mse;
}

CURLcode rb_mse_set_userpwd(struct rb_mse_api *rb_mse, const char *userpwd)
{
  return curl_easy_setopt(rb_mse->hnd, CURLOPT_USERPWD, userpwd);;
}


CURLcode rb_mse_set_addr(struct rb_mse_api *rb_mse, const char * addr)
{
  const size_t url_size = sizeof("https://")  + strlen(addr) + sizeof(mse_api_call_url) + sizeof("00:00:00:00:00:00");
  rb_mse->url = malloc(sizeof(char)*url_size);
  if(rb_mse->url){
    rb_mse->mac_pos_in_url = snprintf(rb_mse->url,url_size - sizeof("00:00:00:00:00:00"),"https://%s%s",addr,mse_api_call_url);
    return CURLE_OK;
  }else{
    return CURLE_OUT_OF_MEMORY;
  }
}

struct rb_mse_api_pos * rb_mse_req_for_mac(struct rb_mse_api *rb_mse,const char *mac)
{
  struct rb_mse_api_pos * pos= calloc(1,sizeof(struct rb_mse_api_pos));
  if(pos)
  {
    snprintf(&rb_mse->url[rb_mse->mac_pos_in_url], sizeof("00:00:00:00:00:00"), mac);
    curl_easy_setopt(rb_mse->hnd, CURLOPT_URL, rb_mse->url);
    CURLcode ret = curl_easy_perform(rb_mse->hnd); 

    struct message_tailq_node * node = STAILQ_FIRST(&rb_mse->msg_list);
    if(node)
    {
      char * mapHierarchyString = NULL;
      char * tok=NULL,*aux=NULL;
      STAILQ_REMOVE_HEAD(&rb_mse->msg_list,next);
      pos->string_tofree = node->msg;
      mapHierarchyString= strstr(pos->string_tofree,"mapHierarchyString=\"");
      if(mapHierarchyString)
      {/// @TODO do it in a recursive way
        mapHierarchyString+=strlen("mapHierarchyString=\"");
        strtok_r(mapHierarchyString,"\"",&aux); // Set NULL last quote
        tok = strstr(mapHierarchyString,"&gt;");
        const size_t mapHierarchyString_len = strlen(mapHierarchyString);
        if(tok)
        {
          pos->floor = mapHierarchyString;
          aux=tok++;
          *aux='\0';
          aux++;
          if(tok) tok = strstr(tok,"&gt;");
          if(tok|| aux < mapHierarchyString_len )
          {
            pos->build = pos->floor;
            pos->floor = aux+strlen("gt;");
            aux = tok++;
            *aux = '\0';
            aux = tok;
            if(tok) tok = strstr(tok, "&gt;");
            if(tok || aux-mapHierarchyString  < (ssize_t) mapHierarchyString_len)
            {
              pos->zone = pos->build;
              pos->build = pos->floor;
              pos->floor = aux+strlen("gt;");
            }
          }
        }
      }
      pos->last_timestamp=time(NULL);
      free(node);
    }
    else
    {
      free(pos);
      pos=NULL;
    }
  }

  return pos;
}

void rb_mse_api_destroy(struct rb_mse_api * rb_mse)
{
  struct message_tailq_node * node;
  while((node = STAILQ_FIRST(&rb_mse->msg_list)))
  {
    STAILQ_REMOVE_HEAD(&rb_mse->msg_list,next);
    free(node->msg);
    free(node);
  }
  curl_easy_cleanup(rb_mse->hnd);
  free(rb_mse->url);
  free(rb_mse);
}


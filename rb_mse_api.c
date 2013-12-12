// rb_mse_api.c


#include "rb_mse_api.h"

#include "librd/rdmem.h"
#include "librd/rdavl.h"
#include "jansson.h"
#include "strbuffer.h"

#include "stdlib.h"
#include "string.h"

const char mse_api_call_url[] = "/api/contextaware/v1/location/clients/";

static inline uint64_t mac_from_str(const char *mac)
{
  uint64_t one,two,three,four,five,six;
  sscanf(mac,"%lx:%lx:%lx:%lx:%lx:%lx",&one,&two,&three,&four,&five,&six);
  return (((((((((one<<8)+two)<<8)+three)<<8)+four)<<8)+five)<<8)+six;
}



/* ============================================================ *
 *                               rb_mse_api_pos
 * ============================================================ */
 
struct rb_mse_api_pos{
  /// position in string_tofree that holds the mac's floor
  const char * floor;
  /// position in string_tofree that holds the mac's build
  const char * build;
  /// position in string_tofree that holds the mac's zone
  const char * zone;
};


void rb_mse_pos_destroy(struct rb_mse_api_pos *pos){
  free(pos);
}

const char * rb_mse_pos_floor(const struct rb_mse_api_pos *pos){return pos->floor;}
const char * rb_mse_pos_build(const struct rb_mse_api_pos *pos){return pos->build;}
const char * rb_mse_pos_zone(const struct rb_mse_api_pos *pos){return pos->zone;}



/* ====================================================================== *
 *                             Positions nodes
 * ====================================================================== */

#define MSE_POSITION_LIST_MAGIC 0x12345678

struct mse_positions_list_node{
#ifdef MSE_POSITION_LIST_MAGIC
  uint64_t magic;
#endif
  uint64_t mac;
  struct rb_mse_api_pos * position;
  rd_avl_node_t rd_avl_node;
};

//typedef rd_avl_t mse_positions_avl;
//typedef TAILQ_HEAD(,mse_positions_list_node) mse_positions_list;

struct rb_mse_api_pos * mse_position(struct mse_positions_list_node *node)
{
  return node->position;
}

int mse_positions_cmp(const void *_node1,const void *_node2)
{
  const struct mse_positions_list_node * node1 = (const struct mse_positions_list_node *)_node1;
  const struct mse_positions_list_node * node2 = (const struct mse_positions_list_node *)_node2;
#ifdef MSE_POSITION_LIST_MAGIC
  assert(_node1!=NULL);
  assert(_node2!=NULL);
  assert(MSE_POSITION_LIST_MAGIC == node1->magic);
  assert(MSE_POSITION_LIST_MAGIC == node2->magic);
#endif
  return node2->mac > node1->mac ? 1 : node2->mac < node1->mac ? -1 : 0; // sizeof(mac1-mac2)>sizeof(int)
}




/* ======================================================================= *
 *                     mse_api structs definitions
 * ======================================================================= */

struct message_tailq_node{
  /// Message stored in the node
  char * msg;

  /// string of the message
  size_t size,nmemb;

  /// STAILQ entry
  STAILQ_ENTRY(message_tailq_node) next;
};

struct rb_mse_api
{
  /// Curl handler.
  CURL *hnd;
  char * url;
  char * userpwd;

  /// Message lists queue.
  strbuffer_t buffer;
  struct curl_slist * slist;

  /// MACs positions avl
  json_t *root;
  rd_avl_t avl;
  rd_memctx_t memctx;
  
  json_error_t error;
};

/*
 *                               CURL CALLBACKS
 */

static size_t write_function( char *ptr, size_t size, size_t nmemb, void *userdata)
{
  assert(userdata);
  struct rb_mse_api * rb_mse = (struct rb_mse_api *)userdata;

  const int ret = strbuffer_append_bytes(&rb_mse->buffer,ptr,nmemb*size);

  return ret==0 ? size*nmemb : 0;
}

#if 0
static size_t header_function( char * ptr,size_t size, size_t nmemb, void* userdata){
  (void)ptr;
  (void)userdata;
  return size*nmemb; // Bypass the headers
}
#endif


/*
 *                                 rb_mse API
 */ 

struct rb_mse_api * rb_mse_api_new()
{
  struct rb_mse_api * rb_mse = calloc(1,sizeof(struct rb_mse_api));
  if(rb_mse)
  {
    rb_mse->slist = curl_slist_append(NULL, "Accept: application/json");

    curl_global_init(CURL_GLOBAL_SSL);
    rb_mse->hnd = curl_easy_init();
    if(rb_mse->hnd)
    {
      curl_easy_setopt(rb_mse->hnd, CURLOPT_WRITEDATA, rb_mse);               /* void passed to WRITEFUNCTION */
      curl_easy_setopt(rb_mse->hnd, CURLOPT_WRITEFUNCTION, write_function);   /* function called for each data received */ 
      // curl_easy_setopt(rb_mse->hnd, CURLOPT_HEADERFUNCTION, header_function); /* function called for each header received */ 

      curl_easy_setopt(rb_mse->hnd, CURLOPT_PROXY, NULL);
      curl_easy_setopt(rb_mse->hnd, CURLOPT_NOPROGRESS, 1);
      curl_easy_setopt(rb_mse->hnd, CURLOPT_HEADER, 0 /*"Accept: application/json"*/);
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
      curl_easy_setopt(rb_mse->hnd, CURLOPT_HTTPHEADER, rb_mse->slist);
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
    else // curl_easy_init error
    {
      free(rb_mse);
      rb_mse=NULL;
    }

    if(rb_mse)
    {
      strbuffer_init(&rb_mse->buffer);
      rd_memctx_init (&rb_mse->memctx, "rb_mse", RD_MEMCTX_F_TRACK);
      rd_avl_init (&rb_mse->avl, mse_positions_cmp, 0);
    }
  }

  return rb_mse;
}

CURLcode rb_mse_set_userpwd(struct rb_mse_api *rb_mse, const char *userpwd)
{
  return curl_easy_setopt(rb_mse->hnd, CURLOPT_USERPWD, userpwd);;
}


CURLcode rb_mse_set_addr(struct rb_mse_api *rb_mse, const char * addr)
{
  const size_t url_size = sizeof("https://")  + strlen(addr) + sizeof(mse_api_call_url);
  rb_mse->url = malloc(sizeof(char)*url_size);
  if(rb_mse->url){
    snprintf(rb_mse->url,url_size,"https://%s%s",addr,mse_api_call_url);
    return CURLE_OK;
  }else{
    return CURLE_OUT_OF_MEMORY;
  }
}

int rb_mse_isempty(const struct rb_mse_api *rb_mse)
{
  return rb_mse->memctx.rmc_out==0;
}

/**
  Update all macs pos in the MSE
  Note: we expect the message like:
  {
    "Locations":{
        "totalPages":1,
        "currentPage":1,
        "pageSize":1882,
        "entries":[
            {
                "macAddress":"ab:ca:dd:fe:10:10",
                "currentlyTracked":true,
                "confidenceFactor":824.0,
                "band":"UNKNOWN",
                "isGuestUser":false,
                "dot11Status":"PROBING",
                "MapInfo":{
                    "mapHierarchyString":"Area>Build>Floor",
                    "floorRefId":4698041219291283488,
                    "Dimension":{
                        "length":5000.0,
                        "width":6900.0,
                        "height":30.0,
                        "offsetX":21360.9,
                        "offsetY":19891.6,
                        "unit":"FEET"
                    },
                    "Image":{
                        "imageName":"domain_5_1350620105694.jpg"
                    }
                },
                "MapCoordinate":{
                    "x":5275.3,
                    "y":2420.19,
                    "unit":"FEET"
                },
                "Statistics":{
                    "currentServerTime":"2013-12-11T10:00:26.650-0800",
                    "firstLocatedTime":"2013-12-10T11:47:43.172-0800",
                    "lastLocatedTime":"2013-12-11T08:54:04.312-0800"
                },
                "GeoCoordinate":{
                    "lattitude":10.1,
                    "longitude":-12.25,
                    "unit":"DEGREES"
                }
            },
          ]
    }

 */
CURLcode rb_mse_update_macs_pos(struct rb_mse_api *rb_mse)
{
  assert(rb_mse);

  CURLcode ret = curl_easy_setopt(rb_mse->hnd, CURLOPT_URL, rb_mse->url);
  if(ret==CURLE_OK)
  {
    ret = curl_easy_perform(rb_mse->hnd);
    if(ret==CURLE_OK)
    {
      json_error_t error;
      const char * text = strbuffer_value(&rb_mse->buffer);
      rb_mse->root = json_loads(text, 0, &error);
      if(rb_mse->root)
      {
        json_t * locations = json_object_get(rb_mse->root, "Locations");
        if(locations)
        {
          json_t * entries = json_object_get(locations, "entries");
          if(entries)
          {
            if(json_is_array(entries))
            {
              unsigned int i;
              for(i = 0; ret == CURLE_OK && i < json_array_size(entries); i++)
              {
                json_t *element= json_array_get(entries, i);
                if(element && json_is_object(element))
                {
                  const char * macAddress=NULL;
                  json_t * json_mapHierarchyString = NULL;
                  const char * mapHierarchyString=NULL;
                  json_t * json_macAddress = json_object_get(element,"macAddress");

                  if(json_is_string(json_macAddress))
                  {
                    macAddress = json_string_value(json_macAddress);
                  }
                  else
                  {
                    ret = -5;
                  }

                  if(NULL!=macAddress)
                  {
                    json_t * mapInfo = json_object_get(element,"MapInfo");
                    if(mapInfo && json_is_object(mapInfo))
                    {
                      json_mapHierarchyString = json_object_get(mapInfo,"mapHierarchyString");
                      if(json_mapHierarchyString && json_is_string(json_mapHierarchyString))
                      {
                        mapHierarchyString = json_string_value(json_mapHierarchyString);
                      }
                      else
                      {
                        ret = -6;
                      }
                    }
                    else
                    {
                      ret = -7;
                    }
                  }

                  if(NULL!=macAddress && NULL!=mapHierarchyString)
                  {
                    struct mse_positions_list_node * node = rd_memctx_calloc(&rb_mse->memctx,1,sizeof(*node));
                    node->position = rd_memctx_calloc(&rb_mse->memctx,1,sizeof(*node->position));
                    #ifdef MSE_POSITION_LIST_MAGIC
                    node->magic = MSE_POSITION_LIST_MAGIC;
                    #endif
                    node->mac =  mac_from_str(macAddress);
                    // printf("DEBUG: macAddr: %12lx\tmacAddr: %s\n",node->mac,macAddress);
                    
                    char * map_string = rd_memctx_strdup(&rb_mse->memctx,mapHierarchyString); // Will free() with pos
                    char * aux;
                    node->position->zone  = strtok_r(map_string,">",&aux);
                    node->position->build = strtok_r(NULL      ,">",&aux);
                    node->position->floor = strtok_r(NULL      ,">",&aux);

                    // printf("Inserting node\n");
                    RD_AVL_INSERT(&rb_mse->avl,node,rd_avl_node);
                  }
                }
                else
                {
                  ret = -4;
                }
              }
            }
            else
            {
              ret = -3;
            }
          }
        }
        // json_decref(rb_mse->root); //Don't! it will be decref in clean().
      }
      else
      {
        ret = -2;
      }
    }
  }

  return ret;
}

const struct rb_mse_api_pos * rb_mse_req_for_mac(struct rb_mse_api *rb_mse,const char *mac)
{
  const struct mse_positions_list_node search_node = {
    #ifdef MSE_POSITION_LIST_MAGIC
    .magic = MSE_POSITION_LIST_MAGIC,
    #endif
    .mac = mac_from_str(mac)
  };

  const struct mse_positions_list_node * ret_node=rd_avl_find(&rb_mse->avl,&search_node,0);
  return ret_node->position;
}

void rb_mse_api_destroy(struct rb_mse_api * rb_mse)
{
  curl_slist_free_all(rb_mse->slist); /* free the list again */
  free(rb_mse->url);
  strbuffer_close(&rb_mse->buffer);
  curl_easy_cleanup(rb_mse->hnd);
  json_decref(rb_mse->root);
  rd_memctx_freeall(&rb_mse->memctx);
  rd_memctx_destroy(&rb_mse->memctx);
  free(rb_mse);

  curl_global_cleanup();
}


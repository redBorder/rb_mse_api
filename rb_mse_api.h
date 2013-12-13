// rb_mse_api.h

#pragma once

#include "curl/curl.h"

/// Struct that holds the position of the MAC
struct rb_mse_api_pos;

struct rb_mse_api;

/** Free an rb_mse_api_pos
    @param pos rb_mse_api_pos you want to free
*/
void rb_mse_pos_destroy(struct rb_mse_api_pos *pos);


const char * rb_mse_pos_floor(const struct rb_mse_api_pos *pos);
const char * rb_mse_pos_build(const struct rb_mse_api_pos *pos);
const char * rb_mse_pos_zone(const struct rb_mse_api_pos *pos);


/** 
  Return a new rb_mse_api struct

  @return new rb_mse_api
 
  @note after this call, errno can be:
     ENOMEM: malloc error
*/
struct rb_mse_api * rb_mse_api_new(time_t update_time);


/**
   Sets MSE address
   @param addr MSE address
   @return CURLE_OK or CURLE_OUT_OF_MEMORY
*/
CURLcode rb_mse_set_addr(struct rb_mse_api *rb_mse, const char * addr);

/**
   Set MSE user:password
   */
CURLcode rb_mse_set_userpwd(struct rb_mse_api *rb_mse, const char *userpwd);

/**
	Get the position of a mac from MSE
	@param rb_mse rb_mse_api struct that hold all curl information
	@param pos    pointer to a pointer to position. If *pos=NULL, 
	@param mac    MAC address you want to know the position
	@return       position of the mac
	@see struct rb_mse_api_pos
	@see rb_mse_pos_destroy
*/
const struct rb_mse_api_pos * rb_mse_req_for_mac(struct rb_mse_api *rb_mse,const char *mac);

int rb_mse_isempty(const struct rb_mse_api * rb_mse);

/**
  Update all macs pos in the MSE
  @TODO Make another errorcode so we can send
  errors parsing the response.
 */
CURLcode rb_mse_update_macs_pos(struct rb_mse_api *rb_mse);


/* call curl_easy_setopt in rb_mse_api */
CURLcode rb_mse_set_curlopt(struct rb_mse_api* ,const int CURLOPT,void * opt);

void rb_mse_api_destroy(struct rb_mse_api* );



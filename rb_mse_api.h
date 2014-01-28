/*
** Copyright (C) 2014 Eneo Tecnologia S.L.
** Author: Eugenio Perez <eupm90@gmail.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License Version 2 as
** published by the Free Software Foundation. You may not use, modify or
** distribute this program under any other version of the GNU General
** Public License.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#pragma once

#include <stdint.h>
#include "curl/curl.h"

#include "librd/rdlog.h"

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
struct rb_mse_api * rb_mse_api_new(time_t update_time,const char * addr,const char *userpwd);

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

/**
  Get the position of a mac from MSE
  @param rb_mse rb_mse_api struct that hold all curl information
  @param pos    pointer to a pointer to position. If *pos=NULL, 
  @param mac    MAC address you want to know the position
  @return       position of the mac
  @see struct rb_mse_api_pos
  @see rb_mse_pos_destroy
*/
const struct rb_mse_api_pos * rb_mse_req_for_mac_i(struct rb_mse_api *rb_mse,uint64_t mac);

int rb_mse_isempty(const struct rb_mse_api * rb_mse);


#define rb_mse_debug_set(rb_mse,onoff) rd_dbg_set (onoff)


/* call curl_easy_setopt in rb_mse_api */
CURLcode rb_mse_set_curlopt(struct rb_mse_api* ,const int CURLOPT,void * opt);

/* clean the handle */
void rb_mse_api_destroy(struct rb_mse_api* );

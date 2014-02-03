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

#include "rb_mse_api.h"

#include <assert.h>
#include <unistd.h>

#define HORIZONTAL_LINE "---------------\n"

void printUsage(char *argv0)
{
	fprintf(stderr,"Usage: %s MSE_IP user:password MAC\n",argv0);
}

int main(int argc,char *argv[]){
	if(argc!=4)
	{
		printUsage(argv[0]);
		return(1);
	}

	struct rb_mse_api * rb_mse = rb_mse_api_new(60, argv[1], argv[2]);
	
	assert(rb_mse);
	assert(rb_mse_isempty(rb_mse));

	rb_mse_debug_set(rb_mse,1);

	CURLcode retCode = CURLE_OK;
	if(retCode == CURLE_OK)
	{
		printf("Sleeping until mac_position filled\n");
		sleep(100);
		printf("Time to ask\n");

		const struct rb_mse_api_pos *position= rb_mse_req_for_mac(rb_mse,argv[3]);
		if(position)
		{
			printf(HORIZONTAL_LINE);
			printf("Location of mac %s\n",argv[3]);
			printf(HORIZONTAL_LINE);
			printf("floor: %s\n",rb_mse_pos_floor(position));
			printf("build: %s\n",rb_mse_pos_build(position));
			printf("zone: %s\n",rb_mse_pos_zone(position));

			if(rb_mse_pos_geo_valid(position))
			{
				printf("lattitude: %lf\n",rb_mse_pos_geo_lattitude(position));
				printf("longitude: %lf\n",rb_mse_pos_geo_longitude(position));
				printf("unit: %s\n",rb_mse_pos_geo_unit(position));
			}
			else
			{
				printf("geo_data not valid\n");
			}
		}
		else
		{
			fprintf(stderr,"position cannot be filled because some reason\n");
		}

		const struct rb_mse_stats *stats = rb_mse_get_stats(rb_mse);
		if(stats)
		{
			printf(HORIZONTAL_LINE);
			printf("Statistics:\n");
			printf(HORIZONTAL_LINE);
			printf("number of macs only map-localized: %d\n",rb_mse_stats_number_of_macs_map_localized(stats));
			printf("number of macs only geo-localized: %d\n",rb_mse_stats_number_of_macs_geo_localized(stats));
			printf("number of macs map and geo localized: %d\n",rb_mse_stats_number_of_macs_map_and_geo_localized(stats));
			printf("number of macs unlocalizables: %d\n",rb_mse_stats_number_of_macs_unlocalizables(stats));
		}
		else
		{
			fprintf(stderr,"Not valid stats\n");
		}
	}
	else
	{
		fprintf(stderr,"There was an error updating MAC positions: %s",curl_easy_strerror(retCode));
	}

	// sleep(600); // Wating for auto-update. 

	rb_mse_api_destroy(rb_mse);

	return 0;
}
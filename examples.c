// examples.c

#include "rb_mse_api.h"

#include <assert.h>
#include <unistd.h>

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

	struct rb_mse_api * rb_mse = rb_mse_api_new(15, argv[1], argv[2]);
	
	assert(rb_mse);
	assert(rb_mse_isempty(rb_mse));

	rb_mse_debug_set(rb_mse,1);

	const struct rb_mse_api_pos * position=NULL;

	CURLcode retCode = CURLE_OK; // rb_mse_update_macs_pos(rb_mse);
	if(retCode == CURLE_OK)
	{
		printf("Sleeping until mac_position filled\n");
		sleep(5);
		position = rb_mse_req_for_mac(rb_mse,argv[3]);
		if(position)
		{
			printf("floor: %s\n",rb_mse_pos_floor(position));
			printf("build: %s\n",rb_mse_pos_build(position));
			printf("zone: %s\n",rb_mse_pos_zone(position));
		}
		else
		{
			fprintf(stderr,"position cannot be filled because some reason\n");
		}
	}
	else
	{
		fprintf(stderr,"There was an error updating MAC positions: %s",curl_easy_strerror(retCode));
	}

	sleep(5); // Wating for auto-update. 

	rb_mse_api_destroy(rb_mse);

	return 0;
}
// examples.c

#include "rb_mse_api.h"

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

	struct rb_mse_api * rb_mse = rb_mse_api_new();
	struct rb_mse_api_pos * position=NULL;
	rb_mse_set_addr(rb_mse, argv[1]);
	rb_mse_set_userpwd(rb_mse, argv[2]);

	// CURLcode retCode;

	position = rb_mse_req_for_mac(rb_mse,argv[3]);
	if(position)
	{
		printf("floor: %s\n",rb_mse_pos_floor(position));
		printf("build: %s\n",rb_mse_pos_build(position));
		printf("zone: %s\n",rb_mse_pos_zone(position));
		rb_mse_pos_destroy(position);
	}
	else
	{
		fprintf(stderr,"position cannot be filled because some reason\n");
	}

	rb_mse_api_destroy(rb_mse);

	return 0;
}
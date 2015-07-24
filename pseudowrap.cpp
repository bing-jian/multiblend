#include "globals.h"
#ifdef NO_CUDA
void pseudowrap_split() {
	printf("pseudowrap_split\n");
	Proftimer proftimer(&mprofiler, "pseudowrap_split");
	int c;
	int y;
	int spread;
	int split=g_workwidth>>1;
	int bpp=g_images[0].bpp>>3;

	g_images[1]=g_images[0];

	g_images[0].width=split;
	g_images[0].xpos=g_workwidth-split;
	g_images[1].xpos=0;
	g_images[1].width=g_workwidth-split;
	g_images[1].channels=(struct_channel*)malloc(g_numchannels*sizeof(struct_channel));

	for (c=0; c<g_numchannels; c++) g_images[1].channels[c].data=malloc(g_images[1].width*g_images[1].height*bpp);

	for (y=0; y<g_images[1].height; y++) {
		for (c=0; c<g_numchannels; c++) memcpy((uint8*)g_images[1].channels[c].data+(y*g_images[1].width)*bpp,(uint8*)g_images[0].channels[c].data+(y*g_workwidth+split)*bpp,g_images[1].width*bpp);

		if (y>0) for (c=0; c<g_numchannels; c++) memcpy((uint8*)g_images[0].channels[c].data+(y*g_images[0].width)*bpp,(uint8*)g_images[0].channels[c].data+(y*g_workwidth)*bpp,g_images[0].width*bpp);
	}

	for (c=0; c<g_numchannels; c++) g_images[0].channels[c].data=realloc(g_images[0].channels[c].data,g_images[0].width*g_images[0].height*bpp);
	g_numimages++;

	g_levels=0;
	while (true) {
		spread=(2<<g_levels)-2;
		if (spread>split) break;
		g_levels++;
	}

// pixels spread by ((2^g_numlevels)-1)*2
}

void pseudowrap_seam() {
	Proftimer proftimer(&mprofiler, "pseudowrap_seam");
	int y;
	int p=0;
	int split=g_workwidth>>1;

	g_seams=(uint32*)malloc(g_workheight*2*sizeof(uint32));

	for (y=0; y<g_workheight; y++) {
		g_seams[p++]=(g_workwidth-split)<<8|1;
		g_seams[p++]=split<<8;
	}
}

void pseudowrap_unsplit() {
	Proftimer proftimer(&mprofiler, "pseudowrap_unsplit");
	int c,y;
	int split=g_workwidth>>1;
	int bpp=g_workbpp>>4;
	void* temp=malloc((g_workwidth-split)<<bpp); // larger end
	uint8* line;

	for (c=0; c<g_numchannels; c++) {
		for (y=0; y<g_workheight; y++) {
			line=(uint8*)(g_out_channels[c])+((g_workwidth*y)<<bpp);
			memcpy(temp,line+(split<<bpp),(g_workwidth-split)<<bpp);
			memcpy(line+((g_workwidth-split)<<bpp),line,split<<bpp);
			memcpy(line,temp,(g_workwidth-split)<<bpp);
		}
	}
	free(temp);
	g_images[0].xpos=0;
	g_images[0].width=g_workwidth;

	g_numimages--;
}
#else
void pseudowrap_split() 
{
	printf("pseudowrap_split\n");
	exit(1);
}

void pseudowrap_seam() 
{
	printf("pseudowrap_seam\n");
	exit(1);
}

void pseudowrap_unsplit() 
{
	printf("pseudowrap_unsplit\n");
	exit(1);
}
#endif

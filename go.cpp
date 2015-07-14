#include "functions.h"
#include "globals.h"

#include <algorithm>

void clean_globals()
{
	for (int c = 0; c < g_numchannels; ++c) 
		_aligned_free(g_out_channels[c]);

	free(g_out_channels);

	for (int i = 0; i < g_numimages; ++i)
		for (int l = 0; l < g_levels; ++l) 
			free(g_images[i].masks[l]);

	free(g_seams);
	free(g_palette);

	for (int i = 0; i < g_numimages; ++i) 
		free(g_images[i].masks);

	_aligned_free(g_line2);
	_aligned_free(g_line1);
	_aligned_free(g_line0);

	for (int i = 0; i < g_numimages; ++i)
	{
		free(g_images[i].binary_mask.data);
		free(g_images[i].binary_mask.rows);
		free(g_images[i].channels);
	}
	free(g_images);
}

void go(std::vector<cv::Mat> &mats, const std::vector<cv::Mat> &masks) {
	Proftimer proftimer(&mprofiler, "go");

	int blend_wh;
	int i;
	int pitch;
	my_timer timer;

	if (!g_nooutput) {
		if (g_jpegquality!=-1) {
			fopen_s(&g_jpeg,g_output_filename,"wb");
			if (!g_jpeg) die("couldn't open output file");
		} else {
			#if TIFF_LIBRARY
			if (!g_bigtiff) g_tiff=TIFFOpen(g_output_filename,"w"); else g_tiff=TIFFOpen(g_output_filename,"w8");
			if (!g_tiff) die("couldn't open output file");
			#endif
		}
	}

	if (mats.size()==1 && g_caching) {
		output(1,"Only one input image; caching disabled\n");
		g_caching=false;
	}

	timer.set();

	g_numimages = (int)mats.size();

	if (mats.size() != masks.size())
		die("mats.size() != masks.size()");

	g_images = (struct_image*)malloc(g_numimages*sizeof(struct_image));

	for (int i = 0; i < g_numimages; ++i) {
		g_images[i].reset();
		g_images[i].bpp = 8;
		g_images[i].channels = (struct_channel*)malloc(g_numchannels*sizeof(struct_channel));
		for (int c = 0; c < g_numchannels; ++c) g_images[i].channels[c].f = 0;
	}

	load_images(mats, masks);

	if (g_numimages==0) die("no valid input files");

	timer.report("load");
	// calculate number of levels

	if (!g_wideblend) {
		blend_wh=0;
		for (i=0; i<g_numimages; i++) blend_wh+=g_images[i].width+g_images[i].height;
		blend_wh=(int)(blend_wh*(0.5/g_numimages));
	} else blend_wh=std::min(g_workwidth,g_workheight);

	g_levels=0;
	while (blend_wh>4) { // was >4 but that caused banding
		blend_wh=(blend_wh+1)>>1;
		g_levels++;
	}
	g_levels=std::min(g_max_levels,g_levels);
	g_levels=std::max(0,g_levels-g_sub_levels);

	output(1,"%dx%d, %dbpp, %d levels\n",g_workwidth,g_workheight,g_workbpp,g_levels);

	pitch=(g_workwidth+7)&(~7);

	g_line0=_aligned_malloc(pitch*sizeof(int),16);
	g_line1=_aligned_malloc(pitch*sizeof(int),16);
	g_line2=_aligned_malloc(pitch*sizeof(int),16);

	if (g_numimages==1) {
		if (g_caching) die("Caching is still enabled but only one input image; multiblend can't continue!");
		output(1,"Only one image; pseudo-wrapping mode assumed\n");
		g_pseudowrap=true;

		//maybe memory leak
		g_images = (struct_image*)realloc(g_images, sizeof(struct_image) * 2);
		pseudowrap_split();
	}

	// dimension mask structs for all images
	for (i=0; i<g_numimages; i++) g_images[i].masks=(float**)malloc(g_levels*sizeof(float*));

	// calculate seams
	timer.set();
	if (g_pseudowrap) {
		//maybe memory leak
		pseudowrap_seam();
	} else {
		seam();
	}
	timer.report("seaming");

	if (!g_nooutput) {
		// calculate mask pyramids
		mask_pyramids();

		// blend
		timer.set();
		blend();
		timer.report("blend");

		if (g_pseudowrap)
		{
			//maybe memory leak
			pseudowrap_unsplit();
		}

		output(1,"writing %s...\n",g_output_filename);
		timer.set();

		#if TIFF_LIBRARY
		#if JPEG_LIBRARY
		if (g_jpegquality!=-1)
			jpeg_out();
		else
		#endif
			tiff_out();
		#else
 		opencv_out();
		#endif

		timer.report("write");
	}

	clean_globals();

	//	ppm_out(out_channels);
}

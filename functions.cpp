#include "functions.h"
#include "globals.h"
#include  <cstdarg>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>


void output(int level, const char* fmt, ...) {
	va_list args;

	if (level<=g_verbosity) {
		va_start(args,fmt);
		vprintf(fmt,args);
		va_end(args);
	}
}

void report_time(const char* name, double time) {
	if (g_timing) output(0,"%s: %.3fs\n",name,time);
}

void clear_temp() {
	int i,c;

	for (i=0; i<g_numimages; i++) {
		for (c=0; c<g_numchannels; c++) {
			if (g_images[i].channels[c].f) {
				fclose(g_images[i].channels[c].f);
#ifdef WIN32
				DeleteFile(g_images[i].channels[c].filename);
#endif
			}
		}
	}
}

#ifndef WIN32
#define SNPRINTF snprintf
int _stricmp(const char* a, const char* b) { return strcasecmp(a,b); }
void* _aligned_malloc(size_t size, int boundary) { return memalign(boundary,size); }
void _aligned_free(void* a) { free(a); }
void fopen_s(FILE** f,const char* filename, const char* mode) { *f=fopen(filename,mode); }
#else
#define SNPRINTF _snprintf_s
#endif

void die(const char* error, ...) {
	va_list args;

	va_start(args,error);
	vprintf(error,args);
	va_end(args);
	printf("\n");

	clear_temp();

	if (g_debug) {
		printf("\nPress Enter to end\n");
		getchar();
	}

	exit(1);
}

bool is_two_areas(const cv::Mat &mask, struct_image* image)
{
	bool two_areas = true;
	for (int y = image->ypos; y < (image->ypos + image->height); ++y)
	{
		int x = image->xpos + image->width / 2;
		if (mask.at<uint8_t>(y, x))
		{
			two_areas = false;
			break;
		}
	}
	return two_areas;
}

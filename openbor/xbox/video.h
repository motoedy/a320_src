#ifndef VIDEO_H
#define VIDEO_H

#include "types.h"

extern int scaleVideo;

int video_set_mode(s_videomodes videomodes);
int video_copy_screen(s_screen*);
void video_clearscreen();


#endif


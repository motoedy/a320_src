#ifndef __DINGOO_SOUND_H__
#define __DINGOO_SOUND_H__

extern void dingoo_sound_volume(int left, int right);
extern void dingoo_sound_thread_mute(void);
extern void dingoo_sound_thread_start(void);
extern void dingoo_sound_thread_stop(void);
extern void dingoo_sound_set_rate(int rate);
extern void dingoo_sound_set_stereo(int stereo);

#endif

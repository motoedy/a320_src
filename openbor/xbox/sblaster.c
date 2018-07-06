/*
	A soundblaster interface.

	Last changes:
	06-25-2000	Delay + DMA buffer clearing after sound stop
*/


int SB_playstart(int bits, int samplerate){
    xbox_pause_audio(0);
	return 1;
}

void SB_playstop(){
    xbox_pause_audio(1);
}

char SB_getvolume(char dev){
	return 0;
}

char SB_setvolume(char dev, char volume){
	return 0;
}

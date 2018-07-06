#ifndef SFONTPLUS_H
#define SFONTPLUS_H

#include <SDL.h>
#include <string>
#include <vector>

#define SFONTPLUS_CHARSET "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~¡¿ÀÁÈÉÌÍÒÓÙÚÝÄËÏÖÜŸÂÊÎÔÛÅÃÕÑÆÇČĎĚĽĹŇÔŘŔŠŤŮŽàáèéìíòóùúýäëïöüÿâêîôûåãõñæçčďěľĺňôřŕšťžůðßÐÞþАБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдеёжзийклмнопрстуфхцчшщъыьэюяØøąćęłńśżźĄĆĘŁŃŚŻŹ"
#ifdef _WIN32
    typedef unsigned int uint;
#endif
using std::vector;
using std::string;

class SFontPlus {
private:
	Uint32 getPixel(Sint32 x, Sint32 y);

	SDL_Surface *surface;
	vector<uint> charpos;
	string characters;
	uint height, lineHeight;

public:
	SFontPlus();
	SFontPlus(SDL_Surface *font);
	SFontPlus(string font);
	~SFontPlus();

	bool utf8Code(unsigned char c);

	void initFont(SDL_Surface *font, string characters = SFONTPLUS_CHARSET);
	void initFont(string font, string characters = SFONTPLUS_CHARSET);
	void freeFont();

	void write(SDL_Surface *s, string text, int x, int y);

	uint getTextWidth(string text);
	uint getHeight();
	uint getLineHeight();
};

#endif /* SFONTPLUS_H */

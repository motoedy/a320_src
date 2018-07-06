/***************************************************************************
 *   Copyright (C) 2006 by Massimiliano Torromeo   *
 *   massimiliano.torromeo@gmail.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef MENUSETTINGINT_H
#define MENUSETTINGINT_H

#include "iconbutton.h"
#include "menusetting.h"
#include "FastDelegate.h"

using std::string;
class GMenu2X;

class MenuSettingInt : public MenuSetting {
private:
	int originalValue;
	int *_value;
	string strvalue;
	IconButton *btnInc, *btnDec, *btnInc2, *btnDec2;
	GMenu2X *gmenu2x;

	void inc();
	void dec();

public:
	MenuSettingInt(GMenu2X *gmenu2x, string name, string description, int *value, int min, int max);
	virtual ~MenuSettingInt() {};

	virtual void draw(int y);
	virtual void handleTS();
	virtual void manageInput();
	virtual void adjustInput();
	virtual void drawSelected(int y);
	virtual bool edited();

	int min, max;
	virtual void setValue(int value);
	int value();
};

#endif

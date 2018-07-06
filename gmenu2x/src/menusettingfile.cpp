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
#include "menusettingfile.h"
#include "filedialog.h"
#include "utilities.h"

using namespace std;
using namespace fastdelegate;

MenuSettingFile::MenuSettingFile(GMenu2X *gmenu2x, string name, string description, string *value, string filter)
	: MenuSetting(gmenu2x,name,description) {
	this->gmenu2x = gmenu2x;
	this->filter = filter;
	_value = value;
	originalValue = *value;

	btnClear = new IconButton(gmenu2x, "skin:imgs/buttons/x.png", gmenu2x->tr["Clear"]);
	btnClear->setAction(MakeDelegate(this, &MenuSettingFile::clear));

	btnSelect = new IconButton(gmenu2x, "skin:imgs/buttons/b.png", gmenu2x->tr["Select a file"]);
	btnSelect->setAction(MakeDelegate(this, &MenuSettingFile::select));
}

void MenuSettingFile::draw(int y) {
	MenuSetting::draw(y);
	gmenu2x->s->write( gmenu2x->font, value(), 155, y+gmenu2x->font->getHalfHeight(), SFontHAlignLeft, SFontVAlignMiddle );
}

void MenuSettingFile::handleTS() {
	btnSelect->handleTS();
	btnClear->handleTS();
}

void MenuSettingFile::manageInput() {
	if ( gmenu2x->input[ACTION_X] ) setValue("");
	if ( gmenu2x->input[ACTION_B] ) select();
}

void MenuSettingFile::clear() {
	setValue("");
}

void MenuSettingFile::select() {
	FileDialog fd(gmenu2x, description, filter, value());
	if (fd.exec()) setValue( fd.path()+"/"+fd.file );
}

void MenuSettingFile::setValue(string value) {
	*_value = value;
}

string MenuSettingFile::value() {
	return *_value;
}

void MenuSettingFile::adjustInput() {}

void MenuSettingFile::drawSelected(int) {
	gmenu2x->drawButton(btnClear,
	gmenu2x->drawButton(btnSelect));
}

bool MenuSettingFile::edited() {
	return originalValue != value();
}

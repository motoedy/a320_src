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

#include <iostream>
#include <sstream>
#include <stdarg.h>

#include "translator.h"

using namespace std;

Translator::Translator(string lang) {
	_lang = "";
	if (!lang.empty())
		setLang(lang);
}

Translator::~Translator() {}

bool Translator::exists(string term) {
	return translations.find(term) != translations.end();
}

void Translator::setLang(string lang) {
	translations.clear();

	string line;
	ifstream infile (string("translations/"+lang).c_str(), ios_base::in);
	if (infile.is_open()) {
		while (getline(infile, line, '\n')) {
			line = trim(line);
			if (line=="") continue;
			if (line[0]=='#') continue;

			string::size_type position = line.find("=");
			translations[ trim(line.substr(0,position)) ] = trim(line.substr(position+1));
		}
		infile.close();
		_lang = lang;
	}
}

string Translator::translate(string term,const char *replacestr,...) {
	string result = term;

	if (!_lang.empty()) {
		hash_map<string, string>::iterator i = translations.find(term);
		if (i != translations.end()) {
			result = i->second;
		}
	#ifdef DEBUG
		else cout << "Untranslated string: " << term << endl;
	#endif
	}

	va_list arglist;
	va_start(arglist, replacestr);
	const char *param = replacestr;
	int argnum = 1;
	while (param!=NULL) {
		string id = "";
		stringstream ss; ss << argnum; ss >> id;
		result = strreplace(result,"$"+id,param);

		param = va_arg(arglist,const char*);
		argnum++;
	}
	va_end(arglist);

	return result;
}

string Translator::operator[](string term) {
	return translate(term);
}

string Translator::lang() {
	return _lang;
}

/*
linphone
Copyright (C) 2013 Belledonne Communications SARL
Simon Morlat (simon.morlat@linphone.org)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef generator_hh
#define generator_hh

#include <fstream>

#include "software-desc.hh"

class OutputGenerator{
public:
	virtual void generate(Project *proj)=0;
};

class CplusplusGenerator : public OutputGenerator{
public:
	CplusplusGenerator();
	virtual void generate(Project *proj);
private:
	void writeClass(Class *klass);
	void writeArgument(Argument *arg, bool isReturn=false);
	void writeTabs(int ntabs);
	void writeHelpComment(const std::string &comment, int ntabs);
	void writeMethod(Method *method);
	void writeEnumMember(ConstField *cf, bool isLast);
	ofstream mOutfile;
	Project *mCurProj;
	Class *mCurClass;
};

class JavascriptGenerator : public OutputGenerator{
public:
	JavascriptGenerator();
	virtual void generate(Project *proj);
private:
	void writeClass(Class *klass);
	void writeEnum(Class *klass);
	void writeType(Type *type);
	enum ArgKind { Normal, Return, PropertyArg};
	void writeArgument(Argument *arg, ArgKind kind=Normal);
	void writeTabs(int ntabs);
	void writeHelpComment(const std::string &comment, int ntabs);
	void writeProperty(Property *prop);
	void writeMethod(Method *method);
	void writeEvent(Method *event);
	string getEventHelp(const string &ref);
	string getEnumName(Class *klass);
	ofstream mOutfile;
	Project *mCurProj;
	Class *mCurClass;
};

string to_lower(const string &str);

#endif

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

#include "software-desc.hh"

Type Type::sStringType(Type::String);
Type Type::sIntegerType(Type::Integer);
Type Type::sVoidType(Type::Void);
Type Type::sBooleanType(Type::Boolean);
Type Type::sFloatType(Type::Float);
Type Type::sArrayType(Type::Array);

std::map<string,Type*> Type::mTypes;
const char *Type::sBasicTypeNames[]={
		"Void",
		"Boolean",
		"Integer",
		"Float",
		"String",
		"Enum",
		"Class",
		"Callback",
		"Array",
		"undef",
		"undef"
};

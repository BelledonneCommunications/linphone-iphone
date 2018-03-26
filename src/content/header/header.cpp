/*
 * header.cpp
 * Copyright (C) 2010-2018 Belledonne Communications SARL
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "linphone/utils/utils.h"
#include "linphone/utils/algorithm.h"

#include "header-p.h"
#include "header-param.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// -----------------------------------------------------------------------------

Header::Header(HeaderPrivate &p) : ClonableObject(p) {

}

void Header::cleanParameters() {
	L_D();
	d->parameters.clear();
}

const std::list<HeaderParam> &Header::getParameters () const {
	L_D();

	return d->parameters;
}

void Header::addParameter (const std::string &paramName, const std::string &paramValue) {
	addParameter(HeaderParam(paramName, paramValue));
}

void Header::addParameter (const HeaderParam &param) {
	L_D();
	removeParameter(param);
	d->parameters.push_back(param);
}

void Header::addParameters(const std::list<HeaderParam> &params) {
	for (auto it = std::begin(params); it!=std::end(params); ++it) {
		HeaderParam param = *it;
    	addParameter(param.getName(), param.getValue());
	}
}

void Header::removeParameter (const std::string &paramName) {
	L_D();
	auto it = findParameter(paramName);
	if (it != d->parameters.cend())
		d->parameters.remove(*it);
}

void Header::removeParameter (const HeaderParam &param) {
	removeParameter(param.getName());
}

std::list<HeaderParam>::const_iterator Header::findParameter (const std::string &paramName) const {
	L_D();
	return findIf(d->parameters, [&paramName](const HeaderParam &param) {
		return param.getName() == paramName;
	});
}

const HeaderParam &Header::getParameter (const std::string &paramName) const {
	L_D();
	std::list<HeaderParam>::const_iterator it = findParameter(paramName);
	if (it != d->parameters.cend()) {
		return *it;
	}
	return Utils::getEmptyConstRefObject<HeaderParam>();
}

LINPHONE_END_NAMESPACE
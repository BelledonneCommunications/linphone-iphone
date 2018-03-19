/*
 * dial-plan.h
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

#ifndef _L_DIAL_PLAN_H_
#define _L_DIAL_PLAN_H_

#include <list>

#include "object/clonable-object.h"

// =============================================================================

LINPHONE_BEGIN_NAMESPACE

class DialPlanPrivate;

class LINPHONE_PUBLIC DialPlan : public ClonableObject {
public:
	DialPlan (
		const std::string &country = "",
		const std::string &isoCountryCode = "",
		const std::string &ccc = "",
		int nnl = 0,
		const std::string &icp = ""
	);
	DialPlan (const DialPlan &other);

	DialPlan &operator= (const DialPlan &other);

	const std::string &getCountry () const;
	const std::string &getIsoCountryCode () const;
	const std::string &getCountryCallingCode () const;
	void setCountryCallingCode(const std::string &ccc);
	int getNationalNumberLength () const;
	const std::string &getInternationalCallPrefix () const;
	bool isGeneric () const;

	static const DialPlan MostCommon;

	static int lookupCccFromE164 (const std::string &e164);
	static int lookupCccFromIso (const std::string &iso);
	static const DialPlan &findByCcc (int ccc);
	static const DialPlan &findByCcc (const std::string &ccc);
	static const std::list<DialPlan> &getAllDialPlans ();

private:
	L_DECLARE_PRIVATE(DialPlan);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_DIAL_PLAN_H_

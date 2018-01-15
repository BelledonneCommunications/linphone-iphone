/*
 * register-op.h
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

#ifndef _L_SAL_REGISTER_OP_H_
#define _L_SAL_REGISTER_OP_H_

#include "sal/op.h"

LINPHONE_BEGIN_NAMESPACE

class SalRegisterOp: public SalOp {
public:
	SalRegisterOp(Sal *sal): SalOp(sal) {}

	int register_(const char *proxy, const char *from, int expires, const SalAddress* old_contact);
	int register_refresh(int expires) {return this->refresher ? belle_sip_refresher_refresh(this->refresher,expires) : -1;}
	int unregister() {return register_refresh(0);}

	virtual void authenticate(const SalAuthInfo *info) override {register_refresh(-1);}

private:
	virtual void fill_cbs() override {};
	static void register_refresher_listener(belle_sip_refresher_t* refresher, void* user_pointer, unsigned int status_code, const char* reason_phrase, int will_retry);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_SAL_REGISTER_OP_H_

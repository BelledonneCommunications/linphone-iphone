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

class SalRegisterOp : public SalOp {
public:
	SalRegisterOp(Sal *sal) : SalOp(sal) {}

	int sendRegister (const char *proxy, const char *from, int expires, const SalAddress *oldContact);
	int refreshRegister (int expires) {
		return mRefresher ? belle_sip_refresher_refresh(mRefresher, expires) : -1;
	}
	int unregister() { return refreshRegister(0); }

	void authenticate (const SalAuthInfo *info) override { 
        refreshRegister(-1); }

private:
	virtual void fillCallbacks () override {};
	static void registerRefresherListener (belle_sip_refresher_t *refresher, void *userCtx, unsigned int statusCode, const char *reasonPhrase, int willRetry);
};

LINPHONE_END_NAMESPACE

#endif // ifndef _L_SAL_REGISTER_OP_H_

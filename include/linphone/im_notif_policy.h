/*
im_notif_policy.h
Copyright (C) 2010-2016 Belledonne Communications SARL

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

#ifndef LINPHONE_IM_NOTIF_POLICY_H_
#define LINPHONE_IM_NOTIF_POLICY_H_


#include "linphone/types.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * @addtogroup chatroom
 * @{
 */

/**
 * Acquire a reference to the LinphoneImNotifPolicy object.
 * @param[in] policy LinphoneImNotifPolicy object.
 * @return The same LinphoneImNotifPolicy object.
**/
LINPHONE_PUBLIC LinphoneImNotifPolicy * linphone_im_notif_policy_ref(LinphoneImNotifPolicy *policy);

/**
 * Release reference to the LinphoneImNotifPolicy object.
 * @param[in] policy LinphoneImNotifPolicy object.
**/
LINPHONE_PUBLIC void linphone_im_notif_policy_unref(LinphoneImNotifPolicy *policy);

/**
 * Retrieve the user pointer associated with the LinphoneImNotifPolicy object.
 * @param[in] policy LinphoneImNotifPolicy object.
 * @return The user pointer associated with the LinphoneImNotifPolicy object.
**/
LINPHONE_PUBLIC void *linphone_im_notif_policy_get_user_data(const LinphoneImNotifPolicy *policy);

/**
 * Assign a user pointer to the LinphoneImNotifPolicy object.
 * @param[in] policy LinphoneImNotifPolicy object.
 * @param[in] ud The user pointer to associate with the LinphoneImNotifPolicy object.
**/
LINPHONE_PUBLIC void linphone_im_notif_policy_set_user_data(LinphoneImNotifPolicy *policy, void *ud);

/**
 * Clear an IM notif policy (deactivate all receiving and sending of notifications).
 * @param[in] policy LinphoneImNotifPolicy object.
 */
LINPHONE_PUBLIC void linphone_im_notif_policy_clear(LinphoneImNotifPolicy *policy);

/**
 * Enable all receiving and sending of notifications.
 * @param[in] policy LinphoneImNotifPolicy object.
 */
LINPHONE_PUBLIC void linphone_im_notif_policy_enable_all(LinphoneImNotifPolicy *policy);

/**
 * Tell whether is_composing notifications are being sent.
 * @param[in] policy LinphoneImNotifPolicy object
 * @return Boolean value telling whether is_composing notifications are being sent.
 */
LINPHONE_PUBLIC bool_t linphone_im_notif_policy_get_send_is_composing(const LinphoneImNotifPolicy *policy);

/**
 * Enable is_composing notifications sending.
 * @param[in] policy LinphoneImNotifPolicy object
 * @param[in] enable Boolean value telling whether to send is_composing notifications.
 */
LINPHONE_PUBLIC void linphone_im_notif_policy_set_send_is_composing(LinphoneImNotifPolicy *policy, bool_t enable);

/**
 * Tell whether is_composing notifications are being notified when received.
 * @param[in] policy LinphoneImNotifPolicy object
 * @return Boolean value telling whether is_composing notifications are being notified when received.
 */
LINPHONE_PUBLIC bool_t linphone_im_notif_policy_get_recv_is_composing(const LinphoneImNotifPolicy *policy);

/**
 * Enable is_composing notifications receiving.
 * @param[in] policy LinphoneImNotifPolicy object
 * @param[in] enable Boolean value telling whether to notify received is_composing notifications.
 */
LINPHONE_PUBLIC void linphone_im_notif_policy_set_recv_is_composing(LinphoneImNotifPolicy *policy, bool_t enable);

/**
 * Tell whether imdn delivered notifications are being sent.
 * @param[in] policy LinphoneImNotifPolicy object
 * @return Boolean value telling whether imdn delivered notifications are being sent.
 */
LINPHONE_PUBLIC bool_t linphone_im_notif_policy_get_send_imdn_delivered(const LinphoneImNotifPolicy *policy);

/**
 * Enable imdn delivered notifications sending.
 * @param[in] policy LinphoneImNotifPolicy object
 * @param[in] enable Boolean value telling whether to send imdn delivered notifications.
 */
LINPHONE_PUBLIC void linphone_im_notif_policy_set_send_imdn_delivered(LinphoneImNotifPolicy *policy, bool_t enable);

/**
 * Tell whether imdn delivered notifications are being notified when received.
 * @param[in] policy LinphoneImNotifPolicy object
 * @return Boolean value telling whether imdn delivered notifications are being notified when received.
 */
LINPHONE_PUBLIC bool_t linphone_im_notif_policy_get_recv_imdn_delivered(const LinphoneImNotifPolicy *policy);

/**
 * Enable imdn delivered notifications receiving.
 * @param[in] policy LinphoneImNotifPolicy object
 * @param[in] enable Boolean value telling whether to notify received imdn delivered notifications.
 */
LINPHONE_PUBLIC void linphone_im_notif_policy_set_recv_imdn_delivered(LinphoneImNotifPolicy *policy, bool_t enable);

/**
 * Tell whether imdn displayed notifications are being sent.
 * @param[in] policy LinphoneImNotifPolicy object
 * @return Boolean value telling whether imdn displayed notifications are being sent.
 */
LINPHONE_PUBLIC bool_t linphone_im_notif_policy_get_send_imdn_displayed(const LinphoneImNotifPolicy *policy);

/**
 * Enable imdn displayed notifications sending.
 * @param[in] policy LinphoneImNotifPolicy object
 * @param[in] enable Boolean value telling whether to send imdn displayed notifications.
 */
LINPHONE_PUBLIC void linphone_im_notif_policy_set_send_imdn_displayed(LinphoneImNotifPolicy *policy, bool_t enable);

/**
 * Tell whether imdn displayed notifications are being notified when received.
 * @param[in] policy LinphoneImNotifPolicy object
 * @return Boolean value telling whether imdn displayed notifications are being notified when received.
 */
LINPHONE_PUBLIC bool_t linphone_im_notif_policy_get_recv_imdn_displayed(const LinphoneImNotifPolicy *policy);

/**
 * Enable imdn displayed notifications receiving.
 * @param[in] policy LinphoneImNotifPolicy object
 * @param[in] enable Boolean value telling whether to notify received imdn displayed notifications.
 */
LINPHONE_PUBLIC void linphone_im_notif_policy_set_recv_imdn_displayed(LinphoneImNotifPolicy *policy, bool_t enable);

/**
 * @}
 */


#ifdef __cplusplus
}
#endif

#endif /* LINPHONE_IM_NOTIF_POLICY_H_ */

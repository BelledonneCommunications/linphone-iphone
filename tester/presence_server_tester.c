/*
    liblinphone_tester - liblinphone test suite
    Copyright (C) 2013  Belledonne Communications SARL

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "linphone/core.h"
#include "private.h"
#include "liblinphone_tester.h"

static void enable_publish(LinphoneCoreManager *mgr, bool_t enable) {
	LinphoneProxyConfig *cfg = linphone_core_get_default_proxy_config(mgr->lc);
	linphone_proxy_config_edit(cfg);
	linphone_proxy_config_enable_publish(cfg, enable);
	linphone_proxy_config_set_publish_expires(cfg, 60);
	linphone_proxy_config_done(cfg);
}

const char * get_identity(LinphoneCoreManager *mgr) {
	LinphoneProxyConfig *cfg = linphone_core_get_default_proxy_config(mgr->lc);
	return linphone_proxy_config_get_identity(cfg);
}

static void enable_deflate_content_encoding(LinphoneCoreManager *mgr, bool_t enable) {
	LinphoneCore *lc = mgr->lc;
	if (enable == TRUE)
		lp_config_set_string(lc->config, "sip", "handle_content_encoding", "deflate");
	else
		lp_config_set_string(lc->config, "sip", "handle_content_encoding", "none");
}

static void simple(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphonePresenceModel *pauline_presence = linphone_presence_model_new_with_activity(LinphonePresenceActivityDinner, NULL);
	LinphoneFriend* f = linphone_core_create_friend_with_address(marie->lc, get_identity(pauline));
	LinphonePresenceActivity *activity = NULL;
	LinphoneCoreCbs *callbacks = linphone_factory_create_core_cbs(linphone_factory_get());
	
	linphone_core_cbs_set_publish_state_changed(callbacks, linphone_publish_state_changed);
	_linphone_core_add_callbacks(pauline->lc, callbacks, TRUE);
	linphone_core_cbs_unref(callbacks);

	lp_config_set_int(marie->lc->config, "sip", "subscribe_expires", 40);
	linphone_core_set_user_agent(pauline->lc, "full-presence-support", NULL);
	linphone_core_set_user_agent(marie->lc, "full-presence-support", NULL);
	enable_publish(pauline, TRUE);
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphonePublishOk,1));

	linphone_friend_enable_subscribes(f, TRUE);
	linphone_friend_set_inc_subscribe_policy(f,LinphoneSPAccept); /* Accept incoming subscription request for this friend*/
	linphone_core_add_friend(marie->lc, f);

	linphone_core_set_presence_model(pauline->lc, pauline_presence);

	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphonePresenceActivityDinner,1));
	activity = linphone_presence_model_get_activity(linphone_friend_get_presence_model(f));
	if (BC_ASSERT_PTR_NOT_NULL(activity)) {
		BC_ASSERT_EQUAL(linphone_presence_activity_get_type(activity), LinphonePresenceActivityDinner, int, "%d");
	}

	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&pauline->stat.number_of_LinphonePublishOk,2));
				   
	linphone_friend_unref(f);
	linphone_core_manager_destroy(marie);
	
	linphone_core_manager_stop(pauline);
	BC_ASSERT_EQUAL(pauline->stat.number_of_LinphonePublishCleared,1,int,"%i");
	BC_ASSERT_EQUAL(pauline->stat.number_of_LinphonePublishOk,2,int,"%i");
	linphone_core_manager_destroy(pauline);
	}

static void fast_activity_change(void) {
#if FIX_ME
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphonePresenceModel *pauline_presence;
	LinphoneFriend* f = linphone_core_create_friend_with_address(marie->lc, get_identity(pauline));
	LinphonePresenceActivity *activity = NULL;

	lp_config_set_int(marie->lc->config, "sip", "subscribe_expires", 40);
	linphone_core_set_user_agent(pauline->lc, "full-presence-support", NULL);
	linphone_core_set_user_agent(marie->lc, "full-presence-support", NULL);
	enable_publish(pauline, TRUE);

	linphone_friend_enable_subscribes(f, TRUE);
	linphone_friend_set_inc_subscribe_policy(f,LinphoneSPAccept); /* Accept incoming subscription request for this friend*/
	linphone_core_add_friend(marie->lc, f);

	/* pauline_Presence activity without description. */
	pauline_presence = linphone_presence_model_new_with_activity(LinphonePresenceActivityDinner, NULL);
	linphone_core_set_presence_model(pauline->lc, pauline_presence);
	pauline_presence = linphone_presence_model_new_with_activity(LinphonePresenceActivityTV, NULL);
	linphone_core_set_presence_model(pauline->lc, pauline_presence);
	pauline_presence = linphone_presence_model_new_with_activity(LinphonePresenceActivityAway, NULL);
	linphone_core_set_presence_model(pauline->lc, pauline_presence);

	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphonePresenceActivityDinner,1));
	activity = linphone_presence_model_get_activity(linphone_friend_get_presence_model(f));
	if (BC_ASSERT_PTR_NOT_NULL(activity)) {
		BC_ASSERT_EQUAL(linphone_presence_activity_get_type(activity), LinphonePresenceActivityDinner, int, "%d");
	}
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphonePresenceActivityTV,1));
	activity = linphone_presence_model_get_activity(linphone_friend_get_presence_model(f));
	if (BC_ASSERT_PTR_NOT_NULL(activity)) {
		BC_ASSERT_EQUAL(linphone_presence_activity_get_type(activity), LinphonePresenceActivityTV, int, "%d");
	}
	BC_ASSERT_TRUE(wait_for(marie->lc,pauline->lc,&marie->stat.number_of_LinphonePresenceActivityAway,1));
	activity = linphone_presence_model_get_activity(linphone_friend_get_presence_model(f));
	if (BC_ASSERT_PTR_NOT_NULL(activity)) {
		BC_ASSERT_EQUAL(linphone_presence_activity_get_type(activity), LinphonePresenceActivityAway, int, "%d");
	}

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
#endif
}

static void subscriber_no_longer_reachable(void){
	LinphoneCoreManager* marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager* pauline1 = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneFriend *lf;
	bctbx_list_t *lcs = NULL;
	LinphonePresenceModel * presence;
	int previous_number_of_LinphonePresenceActivityOnline=0;
	int previous_number_of_LinphonePresenceActivityOffline=0;

	lcs = bctbx_list_append(lcs, marie->lc);
	lcs = bctbx_list_append(lcs, pauline1->lc);

	lp_config_set_int(marie->lc->config, "sip", "subscribe_expires", 80);
	linphone_core_set_user_agent(marie->lc, "full-presence-support", NULL);
	linphone_core_set_user_agent(pauline1->lc, "full-presence-support", NULL);

	enable_publish(pauline1, TRUE);

	lf = linphone_core_create_friend(marie->lc);
	linphone_friend_set_address(lf, pauline1->identity);
	linphone_friend_enable_subscribes(lf, TRUE);

	linphone_core_add_friend(marie->lc, lf);
	linphone_friend_unref(lf);
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphonePresenceBasicStatusOpen,1, 5000));

	/*make sure marie subscribe is not reset by accident because of code below located in linphone_core_iterate

	 if (lc->sip_network_reachable && lc->netup_time!=0 && (current_real_time-lc->netup_time)>3){

		linphone_core_send_initial_subscribes(lc);
	}
	 */
	wait_for_until(pauline1->lc, marie->lc, 0, 0, 3000);

	presence =linphone_presence_model_new_with_activity(LinphonePresenceActivityBusy,NULL);
	linphone_core_set_presence_model(pauline1->lc,presence);

	previous_number_of_LinphonePresenceActivityOnline=marie->stat.number_of_LinphonePresenceActivityOnline;

	/*don't schedule marie to simulate Notify timeout server side*/
	wait_for_until(pauline1->lc, NULL, 0, 0, 40000);

	//sal_set_send_error(marie->lc->sal,0);

	/*
	 * Because of notify timeout detected by server, subscription is reset.
	 * The server then sends a NOTIFY with subscription-state terminated, which translated in Offline status to be notified.
	 */
	previous_number_of_LinphonePresenceActivityOffline = marie->stat.number_of_LinphonePresenceBasicStatusClosed;
	BC_ASSERT_TRUE(wait_for_list(lcs,&marie->stat.number_of_LinphonePresenceActivityOffline,previous_number_of_LinphonePresenceActivityOffline+1, 4000));

	/*
	 * The client handles this subscription terminated event.
	 * It will not resubmit a new SUBSCRIBE until expiration of the current one.
	 */
	presence =linphone_presence_model_new_with_activity(LinphonePresenceActivityOnline,NULL);
	linphone_core_set_presence_model(pauline1->lc,presence);

	/*because subscription is not restarted, the online status shall not be notified again.*/
	BC_ASSERT_FALSE(wait_for_list(lcs,&marie->stat.number_of_LinphonePresenceActivityOnline,previous_number_of_LinphonePresenceActivityOnline+1, 8000));

	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline1);

	bctbx_list_free(lcs);

}

static void subscribe_with_late_publish(void) {
	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneProxyConfig* proxy;
	LinphonePresenceModel* presence;
	LpConfig *pauline_lp;
	char* lf_identity;
 	LinphoneFriend *lf;

	linphone_core_set_user_agent(marie->lc, "full-presence-support", NULL);
	linphone_core_set_user_agent(pauline->lc, "full-presence-support", NULL);
	pauline_lp = linphone_core_get_config(pauline->lc);
	lf_identity=linphone_address_as_string_uri_only(marie->identity);
	lf = linphone_core_create_friend_with_address(pauline->lc,lf_identity);

	lp_config_set_int(pauline_lp,"sip","subscribe_expires",10);

	linphone_core_add_friend(pauline->lc,lf);

	/*wait for subscribe acknowledgment*/
	BC_ASSERT_TRUE(wait_for_until(pauline->lc,marie->lc,&pauline->stat.number_of_NotifyPresenceReceived,1,2000));
	/*BC_ASSERT_EQUAL(LinphoneStatusOffline,linphone_friend_get_status(lf), int, "%d");*/


	/*enable publish*/
	presence =linphone_presence_model_new_with_activity(LinphonePresenceActivityPresentation,NULL);
	linphone_core_set_presence_model(marie->lc,presence);
	proxy = linphone_core_get_default_proxy_config(marie->lc);
	linphone_proxy_config_edit(proxy);

	linphone_proxy_config_enable_publish(proxy,TRUE);
	linphone_proxy_config_set_publish_expires(proxy,3);
	linphone_proxy_config_done(proxy);

	/*wait for marie status*/
	BC_ASSERT_TRUE(wait_for_until(pauline->lc,marie->lc,&pauline->stat.number_of_LinphonePresenceActivityPresentation,1,2000));

	presence =linphone_presence_model_new_with_activity(LinphonePresenceActivityBusy,NULL);
	linphone_core_set_presence_model(marie->lc,presence);

	/*wait for new status*/
	BC_ASSERT_TRUE(wait_for_until(pauline->lc,marie->lc,&pauline->stat.number_of_LinphonePresenceActivityBusy,1,2000));

	/*wait for refresh*/
	BC_ASSERT_TRUE(wait_for_until(pauline->lc,marie->lc,&pauline->stat.number_of_LinphonePresenceActivityBusy,2,4000));

	/*linphone_core_remove_friend(pauline->lc,lf);*/
	/*wait for final notify*/
	/*wait_for_until(pauline->lc,marie->lc,&pauline->stat.number_of_NotifyPresenceReceived,4,5000);
	BC_ASSERT_EQUAL(LinphonePresenceActivityOffline,linphone_friend_get_status(lf), int, "%d");
	 */

	/*Expect a notify at publication expiration because marie is no longuer scheduled*/
	BC_ASSERT_FALSE(wait_for_until(pauline->lc,pauline->lc,&pauline->stat.number_of_LinphonePresenceActivityBusy,3,6000));
	/*thanks to long term presence we are still online*/
	BC_ASSERT_EQUAL(LinphoneStatusOnline,linphone_friend_get_status(lf), int, "%d");

	BC_ASSERT_TRUE(wait_for_until(pauline->lc,marie->lc,&pauline->stat.number_of_LinphonePresenceActivityBusy,3,5000));/*re- schedule marie to clean up things*/

	/*simulate a rapid presence change to make sure only first and last are transmited*/
	linphone_core_set_presence_model(marie->lc,linphone_presence_model_new_with_activity(LinphonePresenceActivityAway,NULL));
	linphone_core_set_presence_model(marie->lc,linphone_presence_model_new_with_activity(LinphonePresenceActivityBreakfast,NULL));
	linphone_core_set_presence_model(marie->lc,linphone_presence_model_new_with_activity(LinphonePresenceActivityAppointment,NULL));

	BC_ASSERT_TRUE(wait_for_until(pauline->lc,marie->lc,&pauline->stat.number_of_LinphonePresenceActivityAppointment,1,5000));

	BC_ASSERT_EQUAL(pauline->stat.number_of_LinphonePresenceActivityAway, 1, int,"%i");
	BC_ASSERT_EQUAL(pauline->stat.number_of_LinphonePresenceActivityBreakfast, 0, int,"%i");
	BC_ASSERT_EQUAL(pauline->stat.number_of_LinphonePresenceActivityAppointment, 1, int,"%i");

	linphone_friend_unref(lf);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void test_forked_subscribe_notify_publish(void) {

	LinphoneCoreManager* marie = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* marie2 = linphone_core_manager_new( "marie_rc");
	LinphoneCoreManager* pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	LinphoneProxyConfig* proxy;
	LinphonePresenceModel* presence;
	LpConfig *pauline_lp;
	char* lf_identity;
	LinphoneFriend *lf;
	bctbx_list_t* lcs=bctbx_list_append(NULL,pauline->lc);
	lcs=bctbx_list_append(lcs,marie->lc);
	lcs=bctbx_list_append(lcs,marie->lc);
	lcs=bctbx_list_append(lcs,marie2->lc);
	linphone_core_set_user_agent(marie->lc, "full-presence-support", NULL);
	linphone_core_set_user_agent(marie2->lc, "full-presence-support", NULL);
	linphone_core_set_user_agent(pauline->lc, "full-presence-support", NULL);


	pauline_lp = linphone_core_get_config(pauline->lc);
	lf_identity=linphone_address_as_string_uri_only(marie->identity);
	lf = linphone_core_create_friend_with_address(pauline->lc,lf_identity);

	lp_config_set_int(pauline_lp,"sip","subscribe_expires",5);

	linphone_core_add_friend(pauline->lc,lf);

	/*wait for subscribe acknowledgment*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_NotifyPresenceReceived,1,3000));

	/*enable publish*/

	proxy = linphone_core_get_default_proxy_config(marie->lc);
	linphone_proxy_config_edit(proxy);
	linphone_proxy_config_enable_publish(proxy,TRUE);
	linphone_proxy_config_set_publish_expires(proxy,3);
	linphone_proxy_config_done(proxy);

	proxy = linphone_core_get_default_proxy_config(marie2->lc);
	linphone_proxy_config_edit(proxy);
	linphone_proxy_config_enable_publish(proxy,TRUE);
	linphone_proxy_config_set_publish_expires(proxy,3);
	linphone_proxy_config_done(proxy);


	/*wait for marie status*/
	wait_for_list(lcs,&pauline->stat.number_of_LinphonePresenceActivityOnline,3,2000); /*initial + 2 from publish*/
	BC_ASSERT_EQUAL(LinphoneStatusOnline,linphone_friend_get_status(lf), int, "%d");

	presence =linphone_presence_model_new_with_activity(LinphonePresenceActivityBusy,NULL);
	linphone_core_set_presence_model(marie->lc,presence);

	/*wait for new status*/
	wait_for_list(lcs,&pauline->stat.number_of_LinphonePresenceActivityBusy,1,3000);
	BC_ASSERT_EQUAL(LinphoneStatusBusy,linphone_friend_get_status(lf), int, "%d");


	presence =linphone_presence_model_new_with_activity(  LinphonePresenceActivityMeeting,NULL);
	linphone_core_set_presence_model(marie2->lc,presence);
	/*wait for new status*/
	BC_ASSERT_TRUE(wait_for_list(lcs,&pauline->stat.number_of_LinphonePresenceActivityMeeting,1,3000));

	linphone_friend_unref(lf);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(marie2);
	linphone_core_manager_destroy(pauline);
}


static void test_presence_list_base(bool_t enable_compression) {
	LinphoneCoreManager *laure = linphone_core_manager_new("laure_tcp_rc");
	LinphoneCoreManager *marie = linphone_core_manager_new("marie_rc");
	LinphoneCoreManager *pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	const char *rls_uri = "sip:rls@sip.example.org";
	LinphoneFriendList *lfl;
	LinphoneFriend *lf;
	const char *laure_identity;
	const char *marie_identity;
	const char *pauline_identity;
	bctbx_list_t* lcs = NULL;

	laure_identity = get_identity(laure);
	marie_identity = get_identity(marie);
	pauline_identity = get_identity(pauline);
	enable_publish(marie, TRUE);
	enable_publish(pauline, TRUE);
	enable_publish(laure, TRUE);
	enable_deflate_content_encoding(marie, enable_compression);
	enable_deflate_content_encoding(pauline, enable_compression);
	enable_deflate_content_encoding(laure, enable_compression);

	linphone_core_set_presence_model(marie->lc, linphone_core_create_presence_model_with_activity(marie->lc, LinphonePresenceActivityBusy, NULL));
	linphone_core_set_presence_model(pauline->lc, linphone_core_create_presence_model_with_activity(pauline->lc, LinphonePresenceActivityVacation, NULL));

	lfl = linphone_core_create_friend_list(laure->lc);
	linphone_friend_list_set_rls_uri(lfl, rls_uri);
	lf = linphone_core_create_friend_with_address(laure->lc, marie_identity);
	linphone_friend_list_add_friend(lfl, lf);
	linphone_friend_unref(lf);
	lf = linphone_core_create_friend_with_address(laure->lc, pauline_identity);
	linphone_friend_list_add_friend(lfl, lf);
	linphone_friend_unref(lf);
	lf = linphone_core_create_friend_with_address(laure->lc, "sip:michelle@sip.inexistentdomain.com");
	linphone_friend_list_add_friend(lfl, lf);
	linphone_friend_unref(lf);
	linphone_core_remove_friend_list(laure->lc, linphone_core_get_default_friend_list(laure->lc));
	linphone_core_add_friend_list(laure->lc, lfl);
	linphone_friend_list_unref(lfl);
	linphone_core_set_presence_model(laure->lc, linphone_core_create_presence_model_with_activity(laure->lc, LinphonePresenceActivityOnline, NULL));

	lcs = bctbx_list_append(lcs, laure->lc);
	lcs = bctbx_list_append(lcs, marie->lc);
	lcs = bctbx_list_append(lcs, pauline->lc);

	wait_for_list(lcs, &laure->stat.number_of_NotifyPresenceReceived, 2, 4000);
	BC_ASSERT_EQUAL(laure->stat.number_of_NotifyPresenceReceived, 2, int, "%d");
	BC_ASSERT_EQUAL(linphone_core_get_default_friend_list(laure->lc)->expected_notification_version, 1, int, "%d");
	lf = linphone_friend_list_find_friend_by_uri(linphone_core_get_default_friend_list(laure->lc), marie_identity);
	BC_ASSERT_EQUAL(linphone_friend_get_status(lf), LinphoneStatusBusy, int, "%d");
	if (!BC_ASSERT_TRUE(lf->presence_received)) goto end;
	lf = linphone_friend_list_find_friend_by_uri(linphone_core_get_default_friend_list(laure->lc), pauline_identity);
	BC_ASSERT_EQUAL(linphone_friend_get_status(lf), LinphoneStatusVacation, int, "%d");
	if (!BC_ASSERT_TRUE(lf->presence_received)) goto end;
	lf = linphone_friend_list_find_friend_by_uri(linphone_core_get_default_friend_list(laure->lc), "sip:michelle@sip.inexistentdomain.com");
	BC_ASSERT_EQUAL(linphone_friend_get_status(lf), LinphoneStatusOffline, int, "%d");
	BC_ASSERT_FALSE(lf->presence_received);

	lfl = linphone_core_create_friend_list(marie->lc);
	linphone_friend_list_set_rls_uri(lfl, rls_uri);
	lf = linphone_core_create_friend_with_address(marie->lc, laure_identity);
	linphone_friend_list_add_friend(lfl, lf);
	linphone_friend_unref(lf);
	linphone_core_remove_friend_list(marie->lc, linphone_core_get_default_friend_list(marie->lc));
	linphone_core_add_friend_list(marie->lc, lfl);
	linphone_friend_list_unref(lfl);
	linphone_friend_list_update_subscriptions(linphone_core_get_default_friend_list(marie->lc));

	wait_for_list(lcs, &marie->stat.number_of_NotifyPresenceReceived, 1, 4000);
	BC_ASSERT_EQUAL(marie->stat.number_of_NotifyPresenceReceived, 1, int, "%d");
	BC_ASSERT_EQUAL(linphone_core_get_default_friend_list(marie->lc)->expected_notification_version, 1, int, "%d");
	lf = linphone_friend_list_find_friend_by_uri(linphone_core_get_default_friend_list(marie->lc), laure_identity);
	BC_ASSERT_EQUAL(linphone_friend_get_status(lf), LinphoneStatusOnline, int, "%d");
	if (!BC_ASSERT_TRUE(lf->presence_received)) goto end;

	lfl = linphone_core_create_friend_list(pauline->lc);
	linphone_friend_list_set_rls_uri(lfl, rls_uri);
	lf = linphone_core_create_friend_with_address(pauline->lc, marie_identity);
	linphone_friend_list_add_friend(lfl, lf);
	linphone_friend_unref(lf);
	linphone_core_remove_friend_list(pauline->lc, linphone_core_get_default_friend_list(pauline->lc));
	linphone_core_add_friend_list(pauline->lc, lfl);
	linphone_friend_list_unref(lfl);
	linphone_friend_list_update_subscriptions(linphone_core_get_default_friend_list(pauline->lc));

	wait_for_list(lcs, &pauline->stat.number_of_NotifyPresenceReceived, 1, 4000);
	BC_ASSERT_EQUAL(pauline->stat.number_of_NotifyPresenceReceived, 1, int, "%d");
	BC_ASSERT_EQUAL(linphone_core_get_default_friend_list(pauline->lc)->expected_notification_version, 1, int, "%d");
	lf = linphone_friend_list_find_friend_by_uri(linphone_core_get_default_friend_list(pauline->lc), marie_identity);
	BC_ASSERT_EQUAL(linphone_friend_get_status(lf), LinphoneStatusBusy, int, "%d");
	if (!BC_ASSERT_TRUE(lf->presence_received)) goto end;

	linphone_core_set_presence_model(marie->lc, linphone_core_create_presence_model_with_activity(marie->lc, LinphonePresenceActivityOnThePhone, NULL));

	wait_for_list(lcs, &laure->stat.number_of_NotifyPresenceReceived, 4, 4000);
	/* The number of PresenceReceived events can be 3 or 4 here. TODO: ideally it should always be 3. */
	BC_ASSERT_GREATER(laure->stat.number_of_NotifyPresenceReceived, 3, int, "%d");
	BC_ASSERT_LOWER(laure->stat.number_of_NotifyPresenceReceived, 4, int, "%d");
	BC_ASSERT_EQUAL(linphone_core_get_default_friend_list(laure->lc)->expected_notification_version, 2, int, "%d");
	lf = linphone_friend_list_find_friend_by_uri(linphone_core_get_default_friend_list(laure->lc), marie_identity);
	BC_ASSERT_EQUAL(linphone_friend_get_status(lf), LinphoneStatusOnThePhone, int, "%d");

	wait_for_list(lcs, &pauline->stat.number_of_NotifyPresenceReceived, 2, 4000);
	BC_ASSERT_EQUAL(pauline->stat.number_of_NotifyPresenceReceived, 2, int, "%d");
	BC_ASSERT_EQUAL(linphone_core_get_default_friend_list(pauline->lc)->expected_notification_version, 2, int, "%d");
	lf = linphone_friend_list_find_friend_by_uri(linphone_core_get_default_friend_list(pauline->lc), marie_identity);
	BC_ASSERT_EQUAL(linphone_friend_get_status(lf), LinphoneStatusOnThePhone, int, "%d");

	ms_message("Disabling publish");
	enable_publish(laure, FALSE);
	enable_publish(marie, FALSE);
	enable_publish(pauline, FALSE);


	reset_counters(&pauline->stat);
	reset_counters(&laure->stat);
	reset_counters(&marie->stat);

	/*keep in mind long terme presence*/

	if (!BC_ASSERT_TRUE(wait_for_list(lcs, &pauline->stat.number_of_LinphonePresenceActivityOnline, 1, 4000)))
		goto end;
	lf = linphone_friend_list_find_friend_by_uri(linphone_core_get_default_friend_list(pauline->lc), marie_identity);
	/*BC_ASSERT_EQUAL(linphone_presence_activity_get_type(linphone_presence_model_get_activity(linphone_friend_get_presence_model(lf)))
					, LinphonePresenceActivityOnline, int, "%d"); fixme, should be LinphonePresenceActivityUnknown*/
	BC_ASSERT_EQUAL(linphone_friend_get_status(lf), LinphoneStatusOnline, int, "%d");


	if (!BC_ASSERT_TRUE(wait_for_list(lcs, &laure->stat.number_of_LinphonePresenceActivityOnline, 2, 4000))) goto end;
	lf = linphone_friend_list_find_friend_by_uri(linphone_core_get_default_friend_list(laure->lc), pauline_identity);
	BC_ASSERT_EQUAL(linphone_friend_get_status(lf), LinphoneStatusOnline, int, "%d");
	/*BC_ASSERT_EQUAL(linphone_presence_activity_get_type(linphone_presence_model_get_activity(linphone_friend_get_presence_model(lf)))
					, LinphonePresenceActivityOnline, int, "%d"); fixme, should be LinphonePresenceActivityUnknown*/

	lf = linphone_friend_list_find_friend_by_uri(linphone_core_get_default_friend_list(laure->lc), marie_identity);
	BC_ASSERT_EQUAL(linphone_friend_get_status(lf), LinphoneStatusOnline, int, "%d");
	/*BC_ASSERT_EQUAL(linphone_presence_activity_get_type(linphone_presence_model_get_activity(linphone_friend_get_presence_model(lf)))
					, LinphonePresenceActivityOnline, int, "%d"); fixme, should be LinphonePresenceActivityUnknown*/


	if (!BC_ASSERT_TRUE(wait_for_list(lcs, &marie->stat.number_of_LinphonePresenceActivityOnline, 1, 4000))) goto end;
	lf = linphone_friend_list_find_friend_by_uri(linphone_core_get_default_friend_list(marie->lc), laure_identity);
	BC_ASSERT_EQUAL(linphone_friend_get_status(lf), LinphoneStatusOnline, int, "%d");
	/*BC_ASSERT_EQUAL(linphone_presence_activity_get_type(linphone_presence_model_get_activity(linphone_friend_get_presence_model(lf)))
					, LinphonePresenceActivityOnline, int, "%d"); fixme, should be LinphonePresenceActivityUnknown*/

end:
	bctbx_list_free(lcs);
	linphone_core_manager_destroy(laure);
	linphone_core_manager_destroy(marie);
	linphone_core_manager_destroy(pauline);
}

static void test_presence_list(void) {
	test_presence_list_base(TRUE);
}

static void test_presence_list_without_compression(void) {
	test_presence_list_base(FALSE);
}

#if 0
static void test_presence_list_subscribe_before_publish(void) {
	LinphoneCoreManager *laure = linphone_core_manager_new("laure_tcp_rc");
	LinphoneCoreManager *pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	const char *rls_uri = "sip:rls@sip.example.org";
	LinphoneFriendList *lfl;
	LinphoneFriend *lf;
	const char *pauline_identity;
	bctbx_list_t* lcs = NULL;
	int dummy = 0;

	pauline_identity = get_identity(pauline);

	linphone_core_set_presence_model(pauline->lc, linphone_core_create_presence_model_with_activity(pauline->lc, LinphonePresenceActivityVacation, NULL));

	lfl = linphone_core_create_friend_list(laure->lc);
	linphone_friend_list_set_rls_uri(lfl, rls_uri);
	lf = linphone_core_create_friend_with_address(laure->lc, pauline_identity);
	linphone_friend_list_add_friend(lfl, lf);
	lf = linphone_core_create_friend_with_address(laure->lc, "sip:michelle@sip.inexistentdomain.com");
	linphone_friend_list_add_friend(lfl, lf);
	linphone_core_remove_friend_list(laure->lc, linphone_core_get_default_friend_list(laure->lc));
	linphone_core_add_friend_list(laure->lc, lfl);
	linphone_friend_list_unref(lfl);
	linphone_core_set_presence_model(laure->lc, linphone_core_create_presence_model_with_activity(laure->lc, LinphonePresenceActivityOnline, NULL));
	linphone_friend_list_update_subscriptions(linphone_core_get_default_friend_list(laure->lc), NULL, FALSE);

	lcs = bctbx_list_append(lcs, laure->lc);
	lcs = bctbx_list_append(lcs, pauline->lc);

	wait_for_list(lcs, &dummy, 1, 2000); /* Wait a little bit for the subscribe to happen */

	enable_publish(pauline, TRUE);
	wait_for_list(lcs, &pauline->stat.number_of_NotifyPresenceReceived, 1, 4000);
	BC_ASSERT_GREATER(laure->stat.number_of_NotifyPresenceReceived, 1, int, "%d");
	BC_ASSERT_GREATER(linphone_core_get_default_friend_list(laure->lc)->expected_notification_version, 1, int, "%d");
	lf = linphone_friend_list_find_friend_by_uri(linphone_core_get_default_friend_list(laure->lc), pauline_identity);
	BC_ASSERT_EQUAL(linphone_friend_get_status(lf), LinphoneStatusVacation, int, "%d");
	BC_ASSERT_TRUE(lf->presence_received);
	lf = linphone_friend_list_find_friend_by_uri(linphone_core_get_default_friend_list(laure->lc), "sip:michelle@sip.inexistentdomain.com");
	BC_ASSERT_EQUAL(linphone_friend_get_status(lf), LinphoneStatusOffline, int, "%d");
	BC_ASSERT_FALSE(lf->presence_received);

	enable_publish(laure, FALSE);
	enable_publish(pauline, FALSE);

	linphone_core_manager_destroy(laure);
	linphone_core_manager_destroy(pauline);
}
#endif

static void test_presence_list_subscription_expire_for_unknown(void) {
	LinphoneCoreManager *laure = linphone_core_manager_new("laure_tcp_rc");
	const char *rls_uri = "sip:rls@sip.example.org";
	LinphoneFriendList *lfl;
	LinphoneFriend *lf;
	lp_config_set_int(laure->lc->config, "sip", "rls_presence_expires", 3);

	lfl = linphone_core_create_friend_list(laure->lc);
	linphone_friend_list_set_rls_uri(lfl, rls_uri);
	lf = linphone_core_create_friend_with_address(laure->lc, "sip:michelle@sip.inexistentdomain.com");
	linphone_friend_list_add_friend(lfl, lf);
	linphone_core_remove_friend_list(laure->lc, linphone_core_get_default_friend_list(laure->lc));
	linphone_core_add_friend_list(laure->lc, lfl);
	linphone_friend_list_update_subscriptions(lfl);
	linphone_friend_list_unref(lfl);

	/* wait for refresh*/
	BC_ASSERT_FALSE(wait_for_until(laure->lc, NULL, &laure->stat.number_of_NotifyPresenceReceived, 1, 4000));

	linphone_friend_unref(lf);
	linphone_core_manager_destroy(laure);
}

static void test_presence_list_subscribe_with_error(bool_t io_error) {
	LinphoneCoreManager *laure = linphone_core_manager_new("laure_tcp_rc");
	LinphoneCoreManager *pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	const char *rls_uri = "sip:rls@sip.example.org";
	LinphoneFriendList *lfl;
	LinphoneFriend *lf;
	const char *pauline_identity;
	bctbx_list_t* lcs = NULL;
	int dummy = 0;
	lp_config_set_int(laure->lc->config, "sip", "rls_presence_expires", 5);


	pauline_identity = get_identity(pauline);

	linphone_core_set_presence_model(pauline->lc, linphone_core_create_presence_model_with_activity(pauline->lc, LinphonePresenceActivityVacation, NULL));

	lfl = linphone_core_create_friend_list(laure->lc);
	linphone_friend_list_set_rls_uri(lfl, rls_uri);
	lf = linphone_core_create_friend_with_address(laure->lc, pauline_identity);
	linphone_friend_list_add_friend(lfl, lf);
	linphone_friend_unref(lf);
	lf = linphone_core_create_friend_with_address(laure->lc, "sip:michelle@sip.inexistentdomain.com");
	linphone_friend_list_add_friend(lfl, lf);
	linphone_friend_unref(lf);
	linphone_core_remove_friend_list(laure->lc, linphone_core_get_default_friend_list(laure->lc));
	linphone_core_add_friend_list(laure->lc, lfl);
	linphone_friend_list_unref(lfl);
	linphone_core_set_presence_model(laure->lc, linphone_core_create_presence_model_with_activity(laure->lc, LinphonePresenceActivityOnline, NULL));
	linphone_friend_list_update_subscriptions(linphone_core_get_default_friend_list(laure->lc));
	lcs = bctbx_list_append(lcs, laure->lc);
	lcs = bctbx_list_append(lcs, pauline->lc);

	wait_for_list(lcs, &dummy, 1, 2000); /* Wait a little bit for the subscribe to happen */

	enable_publish(pauline, TRUE);
	BC_ASSERT_TRUE(wait_for_until(laure->lc, pauline->lc, &laure->stat.number_of_LinphonePresenceActivityVacation, 1, 6000));
	BC_ASSERT_GREATER(laure->stat.number_of_NotifyPresenceReceived, 1, int, "%d");
	BC_ASSERT_GREATER(linphone_core_get_default_friend_list(laure->lc)->expected_notification_version, 1, int, "%d");
	lf = linphone_friend_list_find_friend_by_uri(linphone_core_get_default_friend_list(laure->lc), pauline_identity);
	BC_ASSERT_EQUAL(linphone_friend_get_status(lf), LinphoneStatusVacation, int, "%d");
	BC_ASSERT_TRUE(lf->presence_received);
	lf = linphone_friend_list_find_friend_by_uri(linphone_core_get_default_friend_list(laure->lc), "sip:michelle@sip.inexistentdomain.com");
	BC_ASSERT_EQUAL(linphone_friend_get_status(lf), LinphoneStatusOffline, int, "%d");
	BC_ASSERT_FALSE(lf->presence_received);

	BC_ASSERT_TRUE(wait_for_until(laure->lc, pauline->lc, &laure->stat.number_of_LinphonePresenceActivityVacation, 2, 6000));
	if (io_error) {
		ms_message("Simulating socket error");
		sal_set_recv_error(laure->lc->sal, -1);
		wait_for_list(lcs, &dummy, 1, 500); /* just time for socket to be closed */
	} else {
		ms_message("Simulating in/out packets losses");
		sal_set_send_error(laure->lc->sal,1500); /*make sure no refresh is sent, trash the message without generating error*/
		sal_set_recv_error(laure->lc->sal, 1500); /*make sure server notify to close the dialog is also ignored*/
		wait_for_list(lcs, &dummy, 1, 32000); /* Wait a little bit for the subscribe transaction to timeout */
	}
	/*restart normal behavior*/
	sal_set_send_error(laure->lc->sal,0);
	sal_set_recv_error(laure->lc->sal, 1);
	/*a new subscribe should be sent */

	BC_ASSERT_TRUE(wait_for_until(laure->lc, pauline->lc, &laure->stat.number_of_LinphonePresenceActivityVacation, 3, 9000)); /* give time for subscription to recover to avoid to receive 491 Request pending*/

	linphone_core_set_presence_model(pauline->lc, linphone_core_create_presence_model_with_activity(pauline->lc, LinphonePresenceActivityAway, NULL));

	BC_ASSERT_TRUE(wait_for_until(laure->lc, pauline->lc, &laure->stat.number_of_LinphonePresenceActivityAway, 1, 6000));
	lf = linphone_friend_list_find_friend_by_uri(linphone_core_get_default_friend_list(laure->lc), pauline_identity);
	BC_ASSERT_EQUAL(linphone_friend_get_status(lf), LinphoneStatusAway, int, "%d");

	linphone_core_manager_destroy(laure);
	linphone_core_manager_destroy(pauline);
}

static void presence_list_subscribe_dialog_expire(void) {
	test_presence_list_subscribe_with_error(FALSE);
}

static void presence_list_subscribe_io_error(void) {
	test_presence_list_subscribe_with_error(TRUE);
}

static void long_term_presence_base(const char* addr, bool_t exist, const char* contact) {
	LinphoneFriend* friend2;
	const LinphonePresenceModel* model;
	char *presence_contact;
	int *presence;
	LinphoneCoreManager *pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
	linphone_core_set_user_agent(pauline->lc, "full-presence-support", NULL);

	friend2=linphone_core_create_friend_with_address(pauline->lc,addr);
	linphone_friend_edit(friend2);
	linphone_friend_enable_subscribes(friend2,TRUE);
	linphone_friend_done(friend2);
	linphone_core_add_friend(pauline->lc,friend2);

	presence = exist ? &pauline->stat.number_of_LinphonePresenceActivityOnline : &pauline->stat.number_of_LinphonePresenceActivityOffline;
	BC_ASSERT_TRUE(wait_for(pauline->lc,NULL,presence,1));
	BC_ASSERT_EQUAL(*presence, 1, int, "%d");
	model = linphone_friend_get_presence_model(friend2);
	if (BC_ASSERT_PTR_NOT_NULL(model)) {
		BC_ASSERT_EQUAL(linphone_presence_model_get_basic_status(model),
			exist ? LinphonePresenceBasicStatusOpen : LinphonePresenceBasicStatusClosed, int, "%d");
		presence_contact = linphone_presence_model_get_contact(model);
		if (exist && BC_ASSERT_PTR_NOT_NULL(presence_contact)) {
			BC_ASSERT_STRING_EQUAL(presence_contact, contact);
			ms_free(presence_contact);
		} else if (!exist) {
			BC_ASSERT_PTR_NULL(presence_contact);
		}
	}

	linphone_friend_unref(friend2);
	linphone_core_manager_destroy(pauline);
}
static void long_term_presence_existing_friend(void) {
	// this friend is not online, but is known from flexisip to be registered (see flexisip/userdb.conf),
	// so we expect to get a report that he is currently not online
	long_term_presence_base("sip:liblinphone_tester@sip.example.org", TRUE, "sip:liblinphone_tester@sip.example.org");
}
static void long_term_presence_inexistent_friend(void) {
	long_term_presence_base("sip:random_unknown@sip.example.org", FALSE, NULL);
}

static void long_term_presence_phone_alias(void) {
	long_term_presence_base("sip:+33123456789@sip.example.org", TRUE, "sip:liblinphone_tester@sip.example.org");
}

static const char* random_phone_number(void) {
	static char phone[10];
	int i;
	phone[0] = '+';
	for (i = 1; i < 10; i++) {
		phone[i] = '0' + rand() % 10;
	}
	return phone;
}

static void long_term_presence_phone_alias2(void) {
	LinphoneCoreManager *marie = linphone_core_manager_new3("marie_rc", TRUE, random_phone_number());
	char * identity = linphone_address_as_string_uri_only(marie->identity);
	LinphoneAddress * phone_addr = linphone_core_interpret_url(marie->lc, marie->phone_alias);
	char *phone_addr_uri = linphone_address_as_string(phone_addr);
	long_term_presence_base(phone_addr_uri, TRUE, identity);
	ms_free(identity);
	ms_free(phone_addr_uri);
	linphone_address_unref(phone_addr);
	linphone_core_manager_destroy(marie);
}

static void long_term_presence_list(void) {
	if (linphone_core_vcard_supported()){
	
		LinphoneFriend *f1, *f2;
		LinphoneFriendList* friends;
		const LinphonePresenceModel *presence;
		const char *e164_phone_number = "+33" "123456789";
		const char *nationnal_phone_number = "0123456789";
		LinphoneProxyConfig * pauline_proxy_config;
		
		LinphoneCoreManager *pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
		enable_publish(pauline, FALSE);
		enable_deflate_content_encoding(pauline, FALSE);

		friends = linphone_core_create_friend_list(pauline->lc);
		linphone_friend_list_set_rls_uri(friends, "sip:rls@sip.example.org");
		f1 = linphone_core_create_friend_with_address(pauline->lc, "sip:liblinphone_tester@sip.example.org");
		linphone_friend_add_phone_number(f1, e164_phone_number);
		linphone_friend_list_add_friend(friends, f1);
		linphone_friend_unref(f1);
		f2 = linphone_core_create_friend_with_address(pauline->lc, "sip:random_unknown@sip.example.org");
		linphone_friend_list_add_friend(friends, f2);
		linphone_friend_unref(f2);
		linphone_core_remove_friend_list(pauline->lc, linphone_core_get_default_friend_list(pauline->lc));
		linphone_core_add_friend_list(pauline->lc, friends);
		linphone_friend_list_unref(friends);

		BC_ASSERT_TRUE(wait_for(pauline->lc,NULL,&pauline->stat.number_of_NotifyPresenceReceived,1));
		BC_ASSERT_TRUE(wait_for(pauline->lc, NULL, &pauline->stat.number_of_NotifyPresenceReceivedForUriOrTel, 2));

		f1 = linphone_friend_list_find_friend_by_uri(linphone_core_get_default_friend_list(pauline->lc), "sip:liblinphone_tester@sip.example.org");
		BC_ASSERT_EQUAL(linphone_presence_model_get_basic_status(linphone_friend_get_presence_model(f1)), LinphonePresenceBasicStatusOpen, int, "%d");
		
		presence = linphone_friend_get_presence_model_for_uri_or_tel(f1, e164_phone_number);
		
		if (BC_ASSERT_PTR_NOT_NULL(presence)) {
			BC_ASSERT_STRING_EQUAL(linphone_presence_model_get_contact(presence), "sip:liblinphone_tester@sip.example.org");
		}
		BC_ASSERT_TRUE(f1->presence_received);

		/*now try with nationnal version of phone numer*/
		pauline_proxy_config = linphone_core_get_default_proxy_config(pauline->lc);
		linphone_proxy_config_edit(pauline_proxy_config);
		linphone_proxy_config_set_dial_prefix(pauline_proxy_config, "33");
		linphone_proxy_config_done(pauline_proxy_config);
		presence = linphone_friend_get_presence_model_for_uri_or_tel(f1, nationnal_phone_number);
		
		BC_ASSERT_PTR_NOT_NULL(presence);
			
		f2 = linphone_friend_list_find_friend_by_uri(linphone_core_get_default_friend_list(pauline->lc), "sip:random_unknown@sip.example.org");
		BC_ASSERT_EQUAL(linphone_presence_model_get_basic_status(linphone_friend_get_presence_model(f2)), LinphonePresenceBasicStatusClosed, int, "%d");
		BC_ASSERT_FALSE(f2->presence_received);

		linphone_core_manager_destroy(pauline);
	}else ms_warning("Test skipped, no vcard support");
}

static void long_term_presence_with_e164_phone_without_sip(void) {
	if (linphone_core_vcard_supported()){
		LinphoneCoreManager *marie = linphone_core_manager_new3("marie_rc", TRUE, random_phone_number());
		char * identity = linphone_address_as_string_uri_only(marie->identity);
		LinphoneAddress * phone_addr = linphone_core_interpret_url(marie->lc, marie->phone_alias);
		char *phone_addr_uri = linphone_address_as_string(phone_addr);

		LinphoneFriend* friend2;
		char *presence_contact;
		LinphoneCoreManager *pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
		linphone_core_set_user_agent(pauline->lc, "full-presence-support", NULL);

		friend2=linphone_core_create_friend(pauline->lc);
		linphone_friend_add_phone_number(friend2, marie->phone_alias);
		linphone_core_add_friend(pauline->lc,friend2);

		linphone_friend_list_set_rls_uri(linphone_core_get_default_friend_list(pauline->lc), "sip:rls@sip.example.org");
		linphone_friend_list_enable_subscriptions(linphone_core_get_default_friend_list(pauline->lc), TRUE);
		linphone_core_refresh_registers(pauline->lc);

		BC_ASSERT_TRUE(wait_for(pauline->lc,NULL,&pauline->stat.number_of_LinphonePresenceActivityOnline,1));
		BC_ASSERT_EQUAL(pauline->stat.number_of_LinphonePresenceActivityOnline, 1, int, "%d");
		BC_ASSERT_EQUAL(linphone_presence_model_get_basic_status(linphone_friend_get_presence_model(friend2)), LinphonePresenceBasicStatusOpen, int, "%d");
		if(BC_ASSERT_PTR_NOT_NULL(linphone_friend_get_presence_model(friend2))) {
			presence_contact = linphone_presence_model_get_contact(linphone_friend_get_presence_model(friend2));
			if (BC_ASSERT_PTR_NOT_NULL(presence_contact)) {
				BC_ASSERT_STRING_EQUAL(presence_contact, identity);
				ms_free(presence_contact);
			}
		}
		linphone_friend_unref(friend2);
		linphone_core_manager_destroy(pauline);

		ms_free(identity);
		ms_free(phone_addr_uri);
		linphone_address_unref(phone_addr);
		linphone_core_manager_destroy(marie);
	}else ms_warning("Test skipped, no vcard support");
}

static void long_term_presence_with_phone_without_sip(void) {
	if (linphone_core_vcard_supported()){
		const LinphoneDialPlan *dialPlan;
		char phone[20];
		char* e164;
		size_t i;
		LinphoneProxyConfig * proxy_config;
		LinphoneFriend* friend2;
		char *presence_contact;
		LinphoneCoreManager *marie = NULL;
		char * identity=NULL;
		
		while ((dialPlan = linphone_dial_plan_by_ccc_as_int(bctbx_random()%900)) == linphone_dial_plan_by_ccc(NULL));
		/*now with have a dialplan*/
		for (i = 0; i < MIN((size_t)dialPlan->nnl,sizeof(phone)-1); i++) {
			phone[i] = '0' + rand() % 10;
		}
		phone[i]='\0';
		
		e164=ms_strdup_printf("+%s%s",dialPlan->ccc,phone);
		
		ms_message("Phone number is %s, e164 is %s", phone, e164);
		
		marie = linphone_core_manager_new3("marie_rc", TRUE, e164);
		identity = linphone_address_as_string_uri_only(marie->identity);
		
		LinphoneCoreManager *pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");
		linphone_core_set_user_agent(pauline->lc, "full-presence-support", NULL);
		
		friend2=linphone_core_create_friend(pauline->lc);
		linphone_friend_add_phone_number(friend2, phone);
		linphone_core_add_friend(pauline->lc,friend2);
		
		linphone_friend_list_set_rls_uri(linphone_core_get_default_friend_list(pauline->lc), "sip:rls@sip.example.org");
		linphone_friend_list_enable_subscriptions(linphone_core_get_default_friend_list(pauline->lc), TRUE);
		linphone_core_refresh_registers(pauline->lc);
		
		/*because phone is not normalized*/
		BC_ASSERT_FALSE(wait_for_until(pauline->lc,NULL,&pauline->stat.number_of_LinphonePresenceActivityOnline,1,2000));
		
		/*know adding ccc to proxy config*/
		proxy_config = linphone_core_get_default_proxy_config(pauline->lc);
		linphone_proxy_config_edit(proxy_config);
		linphone_proxy_config_set_dial_prefix(proxy_config, dialPlan->ccc);
		linphone_proxy_config_done(proxy_config);
		/*re-create sub list*/
		linphone_friend_list_enable_subscriptions(linphone_core_get_default_friend_list(pauline->lc), FALSE);
		
		wait_for_until(pauline->lc, NULL, NULL, 0,2000); /*wait for unsubscribe*/

		linphone_friend_list_enable_subscriptions(linphone_core_get_default_friend_list(pauline->lc), TRUE);
		
		BC_ASSERT_TRUE(wait_for(pauline->lc,NULL,&pauline->stat.number_of_LinphonePresenceActivityOnline,1));
		BC_ASSERT_EQUAL(pauline->stat.number_of_LinphonePresenceActivityOnline, 1, int, "%d");
		BC_ASSERT_EQUAL(linphone_presence_model_get_basic_status(linphone_friend_get_presence_model(friend2)), LinphonePresenceBasicStatusOpen, int, "%d");
		if(BC_ASSERT_PTR_NOT_NULL(linphone_friend_get_presence_model(friend2))) {
			presence_contact = linphone_presence_model_get_contact(linphone_friend_get_presence_model(friend2));
			if (BC_ASSERT_PTR_NOT_NULL(presence_contact)) {
				BC_ASSERT_STRING_EQUAL(presence_contact, identity);
				ms_free(presence_contact);
			}
		}
		linphone_friend_unref(friend2);
		linphone_core_manager_destroy(pauline);
		ms_free(e164);
		ms_free(identity);
		linphone_core_manager_destroy(marie);
	}else ms_warning("Test skipped, no vcard support");
}

static char * generate_random_e164_phone_from_dial_plan(const LinphoneDialPlan *dialPlan) {
	char phone[64];
	size_t i;
	/*now with have a dialplan*/
	for (i = 0; i < MIN((size_t)dialPlan->nnl,sizeof(phone)-1); i++) {
		phone[i] = '0' + rand() % 10;
	}
	phone[i]='\0';
	
	return ms_strdup_printf("+%s%s",dialPlan->ccc,phone);
}
/* use case:
  I have a friend, I invite him to use Linphone for the first time.
  This friend is already in my addressbook, with his phone number.
  My friend installs Linphone, creates an account, validate number etc...
  He can immediately see me in his Linphone users list.
  However, it takes hours before I can see him in my Linphone users list.
  */
static void long_term_presence_with_crossed_references(void) {
	if (linphone_core_vcard_supported()){
		const LinphoneDialPlan *dialPlan;
		char *e164_marie, *e164_pauline, *e164_laure;
		LinphoneFriend* friend2;
		
		
		while ((dialPlan = linphone_dial_plan_by_ccc_as_int(bctbx_random()%900)) == linphone_dial_plan_by_ccc(NULL));
		
		ms_message("Marie's phone number is %s", e164_marie=generate_random_e164_phone_from_dial_plan(dialPlan));
		ms_message("Pauline's phone number is %s", e164_pauline=generate_random_e164_phone_from_dial_plan(dialPlan));
		ms_message("Laure's phone number is %s", e164_laure=generate_random_e164_phone_from_dial_plan(dialPlan));
		
		
		/*pauline has marie as friend*/
		LinphoneCoreManager *pauline = linphone_core_manager_new3("pauline_rc",TRUE,e164_pauline);
		linphone_core_set_user_agent(pauline->lc, "full-presence-support", NULL);
		friend2=linphone_core_create_friend(pauline->lc);
		linphone_friend_add_phone_number(friend2, e164_marie);
		linphone_core_add_friend(pauline->lc,friend2);
		linphone_friend_unref(friend2);
		linphone_friend_list_set_rls_uri(linphone_core_get_default_friend_list(pauline->lc), "sip:rls@sip.example.org");
		linphone_friend_list_enable_subscriptions(linphone_core_get_default_friend_list(pauline->lc), TRUE);
		linphone_core_refresh_registers(pauline->lc);
		
		
		//Laure has marie as friend
		LinphoneCoreManager *laure = linphone_core_manager_new3("laure_tcp_rc",TRUE,e164_laure);
		linphone_core_set_user_agent(laure->lc, "full-presence-support", NULL);
		friend2=linphone_core_create_friend(laure->lc);
		linphone_friend_add_phone_number(friend2, e164_marie);
		linphone_core_add_friend(laure->lc,friend2);
		linphone_friend_unref(friend2);
		linphone_friend_list_set_rls_uri(linphone_core_get_default_friend_list(laure->lc), "sip:rls@sip.example.org");
		linphone_friend_list_enable_subscriptions(linphone_core_get_default_friend_list(laure->lc), TRUE);
		linphone_core_refresh_registers(laure->lc);
		
		
		/*because marie is not registered yet*/
		BC_ASSERT_FALSE(wait_for_until(pauline->lc,laure->lc,&pauline->stat.number_of_LinphonePresenceActivityOnline,1,2000));
		BC_ASSERT_FALSE(wait_for_until(pauline->lc,laure->lc,&laure->stat.number_of_LinphonePresenceActivityOnline,1,2000));
		
		//Now, marie register to the service
		LinphoneCoreManager *marie = linphone_core_manager_new3("marie_rc", TRUE, e164_marie);
		friend2=linphone_core_create_friend(marie->lc);
		linphone_friend_add_phone_number(friend2, e164_pauline);
		linphone_core_add_friend(marie->lc,friend2);
		linphone_friend_unref(friend2);
		linphone_friend_list_set_rls_uri(linphone_core_get_default_friend_list(marie->lc), "sip:rls@sip.example.org");
		linphone_friend_list_enable_subscriptions(linphone_core_get_default_friend_list(marie->lc), TRUE);
		linphone_core_refresh_registers(marie->lc);
		
		//Pauline is already registered so I must be notified very rapidely
		BC_ASSERT_TRUE(wait_for_until(marie->lc,marie->lc,&marie->stat.number_of_LinphonePresenceActivityOnline,1,4000));
		
		
		//For Pauline and Laure long term presence check was already performed so they will not be notified until new subscription
		BC_ASSERT_FALSE(wait_for_until(pauline->lc,laure->lc,&laure->stat.number_of_LinphonePresenceActivityOnline,1,4000));
		BC_ASSERT_FALSE(wait_for_until(pauline->lc,laure->lc,&pauline->stat.number_of_LinphonePresenceActivityOnline,1,4000));
		
		//re-subscribe to get notification.
		linphone_friend_list_enable_subscriptions(linphone_core_get_default_friend_list(pauline->lc), FALSE);
		wait_for_until(pauline->lc, NULL, NULL, 0,2000); /*wait for unsubscribe*/
		linphone_friend_list_enable_subscriptions(linphone_core_get_default_friend_list(pauline->lc), TRUE);
		
		BC_ASSERT_TRUE(wait_for_until(pauline->lc,pauline->lc,&pauline->stat.number_of_LinphonePresenceActivityOnline,1,4000));
		
		linphone_core_manager_destroy(pauline);
		linphone_core_manager_destroy(marie);
		linphone_core_manager_destroy(laure);
		ms_free(e164_marie);
		ms_free(e164_pauline);
		ms_free(e164_laure);
	}else ms_warning("Test skipped, no vcard support");
}

static void long_term_presence_list_for_many_friends(void) {
	if (linphone_core_vcard_supported()) {
		LinphoneFriendList *friends;
		char *import_filepath = bc_tester_res("vcards/presence.vcf");
		LinphoneCoreManager *pauline;
		
		pauline = linphone_core_manager_new(transport_supported(LinphoneTransportTls) ? "pauline_rc" : "pauline_tcp_rc");

		friends = linphone_core_create_friend_list(pauline->lc);
		linphone_friend_list_set_rls_uri(friends, "sip:rls@sip.example.org");
		linphone_friend_list_import_friends_from_vcard4_file(friends, import_filepath);
		linphone_core_remove_friend_list(pauline->lc, linphone_core_get_default_friend_list(pauline->lc));
		linphone_core_add_friend_list(pauline->lc, friends);
		linphone_core_refresh_registers(pauline->lc);
		linphone_friend_list_unref(friends);

		BC_ASSERT_TRUE(wait_for(pauline->lc,NULL,&pauline->stat.number_of_NotifyPresenceReceived,1));

		linphone_core_manager_destroy(pauline);
	} else ms_warning("Test skipped, no vcard support");
}

test_t presence_server_tests[] = {
	TEST_NO_TAG("Simple", simple),
	TEST_NO_TAG("Fast activity change", fast_activity_change),
	TEST_NO_TAG("Forked subscribe with late publish", test_forked_subscribe_notify_publish),
	TEST_NO_TAG("Presence list", test_presence_list),
	TEST_NO_TAG("Presence list without compression", test_presence_list_without_compression),
	TEST_NO_TAG("Presence list, subscription expiration for unknown contact",test_presence_list_subscription_expire_for_unknown),
	TEST_NO_TAG("Presence list, silent subscription expiration", presence_list_subscribe_dialog_expire),
	TEST_NO_TAG("Presence list, io error",presence_list_subscribe_io_error),
	TEST_ONE_TAG("Long term presence existing friend",long_term_presence_existing_friend, "longterm"),
	TEST_ONE_TAG("Long term presence inexistent friend",long_term_presence_inexistent_friend, "longterm"),
	TEST_ONE_TAG("Long term presence phone alias",long_term_presence_phone_alias, "longterm"),
	TEST_ONE_TAG("Long term presence phone alias 2",long_term_presence_phone_alias2, "longterm"),
	TEST_ONE_TAG("Long term presence list",long_term_presence_list, "longterm"),
	TEST_ONE_TAG("Long term presence list for many friends",long_term_presence_list_for_many_friends, "longterm"),
	TEST_ONE_TAG("Long term presence with +164 phone, without sip",long_term_presence_with_e164_phone_without_sip, "longterm"),
	TEST_ONE_TAG("Long term presence with phone, without sip",long_term_presence_with_phone_without_sip, "longterm"),
	TEST_ONE_TAG("Long term presence with cross references", long_term_presence_with_crossed_references,"longtern"),
	TEST_NO_TAG("Subscriber no longer reachable using server",subscriber_no_longer_reachable),
	TEST_NO_TAG("Subscribe with late publish", subscribe_with_late_publish),
};

test_suite_t presence_server_test_suite = {"Presence using server", NULL, NULL, liblinphone_tester_before_each, liblinphone_tester_after_each,
									sizeof(presence_server_tests) / sizeof(presence_server_tests[0]), presence_server_tests};

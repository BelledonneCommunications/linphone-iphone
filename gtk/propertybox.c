/*
linphone, gtk-glade interface.
Copyright (C) 2008-2009  Simon MORLAT (simon.morlat@linphone.org)

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

#include "linphone.h"
#include "linphone/tunnel.h"
#include "linphone/lpconfig.h"
#include "config.h"

void linphone_gtk_fill_combo_box(GtkWidget *combo, const char **devices, const char *selected, DeviceCap cap){
	const char **p=devices;
	int i=0,active=-1;
	GtkTreeModel *model;


	if ((model=gtk_combo_box_get_model(GTK_COMBO_BOX(combo)))==NULL){
		/*case where combo box is created with no model*/
		GtkCellRenderer *renderer=gtk_cell_renderer_text_new();
		model=GTK_TREE_MODEL(gtk_list_store_new(1,G_TYPE_STRING));
		gtk_combo_box_set_model(GTK_COMBO_BOX(combo),model);
		gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo),renderer,TRUE);
		gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo),renderer,"text",0,NULL);
	}else{
		gtk_list_store_clear(GTK_LIST_STORE(model));
		/* glade creates a combo box without list model and text renderer,
		unless we fill it with a dummy text.
		This dummy text needs to be removed first*/
	}

	for(;*p!=NULL;++p){
		if ( cap==CAP_IGNORE
			|| (cap==CAP_CAPTURE && linphone_core_sound_device_can_capture(linphone_gtk_get_core(),*p))
			|| (cap==CAP_PLAYBACK && linphone_core_sound_device_can_playback(linphone_gtk_get_core(),*p)) ){
			gtk_combo_box_append_text(GTK_COMBO_BOX(combo),*p);
			if (selected && strcmp(selected,*p)==0) active=i;
			i++;
		}
	}
	if (active!=-1)
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo),active);
}

static void linphone_gtk_ldap_display( GtkWidget* param )
{
	LpConfig* config = linphone_core_get_config(linphone_gtk_get_core());
	LinphoneDictionary* ldap_conf = lp_config_section_to_dict(config,"ldap");
	GtkLabel* label;

	ms_message("linphone_gtk_ldap_display");
	label= GTK_LABEL(linphone_gtk_get_widget(param,"ldap_server"));
	gtk_label_set_text(label, linphone_dictionary_get_string(ldap_conf,"server", "ldap://localhost") );

	label = GTK_LABEL(linphone_gtk_get_widget(param,"ldap_auth_method"));
	gtk_label_set_text(label, linphone_dictionary_get_string(ldap_conf,"auth_method", "ANONYMOUS") );

	label = GTK_LABEL(linphone_gtk_get_widget(param,"ldap_username"));
	gtk_label_set_text(label, linphone_dictionary_get_string(ldap_conf,"username", "") );
}

static void linphone_gtk_ldap_set_authcombo( GtkComboBox* box, const char* authmethod )
{
	GtkTreeModel* model = GTK_TREE_MODEL(gtk_combo_box_get_model(box));
	GtkTreeIter iter;
	g_return_if_fail(gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter) );

	do{
		const char* value;

		gtk_tree_model_get(model,&iter,0,&value,-1);
		if( value && strcmp(value, authmethod) == 0){
			gtk_combo_box_set_active_iter(box, &iter);
			break;
		}

	}while(gtk_tree_model_iter_next(model,&iter));
}

static void linphone_gtk_ldap_load_settings(GtkWidget* param)
{
	LpConfig* config = linphone_core_get_config(linphone_gtk_get_core());
	LinphoneDictionary* ldap_conf = lp_config_section_to_dict(config,"ldap");
	GtkEntry* entry;
	GtkToggleButton* toggle;
	GtkSpinButton* spin;
	GtkComboBox* cbox;


	toggle = GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(param,"ldap_use_tls"));
	gtk_toggle_button_set_active(toggle, linphone_dictionary_get_int(ldap_conf,"use_tls", 0) );

	entry = GTK_ENTRY(linphone_gtk_get_widget(param,"ldap_server"));
	gtk_entry_set_text(entry, linphone_dictionary_get_string(ldap_conf,"server", "ldap://localhost") );

	entry = GTK_ENTRY(linphone_gtk_get_widget(param,"ldap_username"));
	gtk_entry_set_text(entry, linphone_dictionary_get_string(ldap_conf,"username", "") );

	entry = GTK_ENTRY(linphone_gtk_get_widget(param,"ldap_password"));
	gtk_entry_set_text(entry, linphone_dictionary_get_string(ldap_conf,"password", "") );

	// SASL
	entry = GTK_ENTRY(linphone_gtk_get_widget(param,"ldap_bind_dn"));
	gtk_entry_set_text(entry, linphone_dictionary_get_string(ldap_conf,"bind_dn", "") );
	entry = GTK_ENTRY(linphone_gtk_get_widget(param,"ldap_sasl_authname"));
	gtk_entry_set_text(entry, linphone_dictionary_get_string(ldap_conf,"sasl_authname", "") );
	entry = GTK_ENTRY(linphone_gtk_get_widget(param,"ldap_sasl_realm"));
	gtk_entry_set_text(entry, linphone_dictionary_get_string(ldap_conf,"sasl_realm", "") );

	cbox  = GTK_COMBO_BOX(linphone_gtk_get_widget(param,"ldap_auth_method"));
	linphone_gtk_ldap_set_authcombo(cbox, linphone_dictionary_get_string(ldap_conf,"auth_method", "ANONYMOUS"));

	entry = GTK_ENTRY(linphone_gtk_get_widget(param,"ldap_base_object"));
	gtk_entry_set_text(entry, linphone_dictionary_get_string(ldap_conf,"base_object", "dc=example,dc=com") );

	entry = GTK_ENTRY(linphone_gtk_get_widget(param,"ldap_filter"));
	gtk_entry_set_text(entry, linphone_dictionary_get_string(ldap_conf,"filter", "uid=*%s*") );

	entry = GTK_ENTRY(linphone_gtk_get_widget(param,"ldap_name_attribute"));
	gtk_entry_set_text(entry, linphone_dictionary_get_string(ldap_conf,"name_attribute", "cn") );

	entry = GTK_ENTRY(linphone_gtk_get_widget(param,"ldap_sip_attribute"));
	gtk_entry_set_text(entry, linphone_dictionary_get_string(ldap_conf,"sip_attribute", "mobile") );

	entry = GTK_ENTRY(linphone_gtk_get_widget(param,"ldap_attributes"));
	gtk_entry_set_text(entry, linphone_dictionary_get_string(ldap_conf,"attributes", "cn,givenName,sn,mobile,homePhone") );


	toggle = GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(param,"ldap_deref_aliases"));
	gtk_toggle_button_set_active(toggle, linphone_dictionary_get_int(ldap_conf,"deref_aliases", 0) );

	spin = GTK_SPIN_BUTTON(linphone_gtk_get_widget(param,"ldap_max_results"));
	gtk_spin_button_set_value(spin, linphone_dictionary_get_int(ldap_conf,"max_results", 50) );

	spin = GTK_SPIN_BUTTON(linphone_gtk_get_widget(param,"ldap_timeout"));
	gtk_spin_button_set_value(spin, linphone_dictionary_get_int(ldap_conf,"timeout", 10) );

}


void linphone_gtk_show_ldap_config(GtkWidget* button)
{
	GtkWidget* param = gtk_widget_get_toplevel(button);
	GtkWidget* ldap_config = linphone_gtk_create_window("ldap", param);
	linphone_gtk_ldap_load_settings(ldap_config);

	// to refresh parameters when the ldap config is destroyed
	g_object_weak_ref(G_OBJECT(ldap_config), (GWeakNotify)linphone_gtk_ldap_display, (gpointer)param);

	gtk_widget_show(ldap_config);
}

void linphone_gtk_ldap_reset(GtkWidget *button)
{
	GtkWidget *w=gtk_widget_get_toplevel(GTK_WIDGET(button));
	ms_message("RESET LDAP");
	gtk_widget_destroy(w);
}

void linphone_gtk_ldap_save(GtkWidget *button)
{
	LinphoneCore *lc = linphone_gtk_get_core();
	LpConfig* conf = linphone_core_get_config(lc);
	LinphoneDictionary* dict = linphone_dictionary_new();

	GtkWidget *ldap_widget = gtk_widget_get_toplevel(button);
	GtkEntry* entry;
	GtkToggleButton* toggle;
	GtkSpinButton* spin;
	GtkComboBox* cbox;

	ms_message("SAVE LDAP");

	toggle = GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(ldap_widget,"ldap_use_tls"));
	linphone_dictionary_set_int(dict, "use_tls", gtk_toggle_button_get_active(toggle));


	entry = GTK_ENTRY(linphone_gtk_get_widget(ldap_widget,"ldap_server"));
	linphone_dictionary_set_string(dict, "server", gtk_entry_get_text(entry));

	entry = GTK_ENTRY(linphone_gtk_get_widget(ldap_widget,"ldap_username"));
	linphone_dictionary_set_string(dict, "username", gtk_entry_get_text(entry));

	entry = GTK_ENTRY(linphone_gtk_get_widget(ldap_widget,"ldap_password"));
	linphone_dictionary_set_string(dict, "password", gtk_entry_get_text(entry));

	// SASL
	entry = GTK_ENTRY(linphone_gtk_get_widget(ldap_widget,"ldap_bind_dn"));
	linphone_dictionary_set_string(dict, "bind_dn", gtk_entry_get_text(entry));
	entry = GTK_ENTRY(linphone_gtk_get_widget(ldap_widget,"ldap_sasl_authname"));
	linphone_dictionary_set_string(dict, "sasl_authname", gtk_entry_get_text(entry));
	entry = GTK_ENTRY(linphone_gtk_get_widget(ldap_widget,"ldap_sasl_realm"));
	linphone_dictionary_set_string(dict, "sasl_realm", gtk_entry_get_text(entry));


	cbox = GTK_COMBO_BOX(linphone_gtk_get_widget(ldap_widget,"ldap_auth_method"));
	linphone_dictionary_set_string(dict, "auth_method", gtk_combo_box_get_active_text(cbox));

	entry = GTK_ENTRY(linphone_gtk_get_widget(ldap_widget,"ldap_base_object"));
	linphone_dictionary_set_string(dict, "base_object", gtk_entry_get_text(entry));

	entry = GTK_ENTRY(linphone_gtk_get_widget(ldap_widget,"ldap_filter"));
	linphone_dictionary_set_string(dict, "filter", gtk_entry_get_text(entry));

	entry = GTK_ENTRY(linphone_gtk_get_widget(ldap_widget,"ldap_name_attribute"));
	linphone_dictionary_set_string(dict, "name_attribute", gtk_entry_get_text(entry));

	entry = GTK_ENTRY(linphone_gtk_get_widget(ldap_widget,"ldap_sip_attribute"));
	linphone_dictionary_set_string(dict, "sip_attribute", gtk_entry_get_text(entry));

	entry = GTK_ENTRY(linphone_gtk_get_widget(ldap_widget,"ldap_attributes"));
	linphone_dictionary_set_string(dict, "attributes", gtk_entry_get_text(entry));

	toggle = GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(ldap_widget,"ldap_deref_aliases"));
	linphone_dictionary_set_int(dict, "deref_aliases", gtk_toggle_button_get_active(toggle));

	spin = GTK_SPIN_BUTTON(linphone_gtk_get_widget(ldap_widget,"ldap_max_results"));
	linphone_dictionary_set_int(dict, "max_results", (int)gtk_spin_button_get_value(spin) );

	spin = GTK_SPIN_BUTTON(linphone_gtk_get_widget(ldap_widget,"ldap_timeout"));
	linphone_dictionary_set_int(dict, "timeout", (int)gtk_spin_button_get_value(spin) );

	ms_message("Create LDAP from config");
	// create new LDAP according to the validated config
	linphone_gtk_set_ldap( linphone_ldap_contact_provider_create(lc, dict) );
	// save the config to linphonerc:
	lp_config_load_dict_to_section(conf, "ldap", dict);

	linphone_dictionary_unref(dict);

	// close widget
	gtk_widget_destroy(ldap_widget);

}

void linphone_gtk_fill_video_sizes(GtkWidget *combo){
	int i;
	int active=0;
	char vsize_def[256];
	MSVideoSize cur=linphone_core_get_preferred_video_size(linphone_gtk_get_core());
	const MSVideoSizeDef *def=linphone_core_get_supported_video_sizes(linphone_gtk_get_core());;
	/* glade creates a combo box without list model and text renderer,
	unless we fill it with a dummy text.
	This dummy text needs to be removed first*/
	gtk_combo_box_remove_text(GTK_COMBO_BOX(combo),0);
	for(i=0;def->name!=NULL;++def,++i){
		snprintf(vsize_def,sizeof(vsize_def),"%s (%ix%i)",def->name,def->vsize.width,def->vsize.height);
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo),vsize_def);
		if (cur.width==def->vsize.width && cur.height==def->vsize.height) active=i;
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(combo),active);
}

void linphone_gtk_parameters_closed(GtkWidget *button){
	GtkWidget *pb=gtk_widget_get_toplevel(button);
	gtk_widget_destroy(pb);
	linphone_gtk_update_status_bar_icons();
}

void linphone_gtk_update_my_contact(GtkWidget *w){
	GtkWidget *pb=gtk_widget_get_toplevel(w);
	const char *username=gtk_entry_get_text(GTK_ENTRY(linphone_gtk_get_widget(pb,"username")));
	const char *displayname=gtk_entry_get_text(GTK_ENTRY(linphone_gtk_get_widget(pb,"displayname")));
	int port=linphone_core_get_sip_port(linphone_gtk_get_core());
	LinphoneAddress *parsed=linphone_core_get_primary_contact_parsed(linphone_gtk_get_core());
	char *contact;
	g_return_if_fail(parsed!=NULL);
	if (username[0]=='\0') return;

	linphone_address_set_display_name(parsed,displayname);
	linphone_address_set_username(parsed,username);
	linphone_address_set_port(parsed,port);
	contact=linphone_address_as_string(parsed);
	gtk_entry_set_text(GTK_ENTRY(linphone_gtk_get_widget(pb,"sip_address")),contact);
	linphone_core_set_primary_contact(linphone_gtk_get_core(),contact);
	ms_free(contact);
	linphone_address_unref(parsed);
	linphone_gtk_load_identities();
}

void linphone_gtk_set_propety_entry(GtkWidget *w, gboolean stunServer, gboolean ip){
	GtkWidget *stun_entry=linphone_gtk_get_widget(gtk_widget_get_toplevel(w),"stun_server");
	GtkWidget *ip_entry=linphone_gtk_get_widget(gtk_widget_get_toplevel(w),"nat_address");
	gtk_widget_set_sensitive(stun_entry,stunServer);
	gtk_widget_set_sensitive(ip_entry,ip);
}

void linphone_gtk_stun_server_changed(GtkWidget *w){
	const gchar *addr=gtk_entry_get_text(GTK_ENTRY(w));
	linphone_core_set_stun_server(linphone_gtk_get_core(),addr);
}

void linphone_gtk_nat_address_changed(GtkWidget *w){
	const gchar *addr=gtk_entry_get_text(GTK_ENTRY(w));
	linphone_core_set_nat_address(linphone_gtk_get_core(),addr);
}

void linphone_gtk_ipv6_toggled(GtkWidget *w){
	linphone_core_enable_ipv6(linphone_gtk_get_core(),
				gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)));
}

void linphone_gtk_min_audio_port_changed(GtkWidget *w){
	GtkWidget *mw = linphone_gtk_get_main_window();
	GtkWidget *pb = (GtkWidget *) g_object_get_data(G_OBJECT(mw), "parameters");
	GtkSpinButton *min_button = GTK_SPIN_BUTTON(w);
	GtkSpinButton *max_button = GTK_SPIN_BUTTON(linphone_gtk_get_widget(pb, "audio_max_rtp_port"));
	gboolean fixed = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(pb, "fixed_audio_port")));

	if (fixed) {
		linphone_core_set_audio_port(linphone_gtk_get_core(), (gint) gtk_spin_button_get_value(min_button));
		gtk_spin_button_set_value(max_button, gtk_spin_button_get_value(min_button));
	} else {
		gint min_port = (gint)gtk_spin_button_get_value(min_button);
		gint max_port = (gint)gtk_spin_button_get_value(max_button);
		if (min_port > max_port) {
			gtk_spin_button_set_value(max_button, min_port);
			max_port = min_port;
		}
		linphone_core_set_audio_port_range(linphone_gtk_get_core(), min_port, max_port);
	}
}

void linphone_gtk_max_audio_port_changed(GtkWidget *w){
	GtkWidget *mw = linphone_gtk_get_main_window();
	GtkWidget *pb = (GtkWidget *) g_object_get_data(G_OBJECT(mw), "parameters");
	GtkSpinButton *min_button = GTK_SPIN_BUTTON(linphone_gtk_get_widget(pb, "audio_min_rtp_port"));
	GtkSpinButton *max_button = GTK_SPIN_BUTTON(w);
	gint min_port = (gint)gtk_spin_button_get_value(min_button);
	gint max_port = (gint)gtk_spin_button_get_value(max_button);
	if (max_port < min_port) {
		gtk_spin_button_set_value(min_button, max_port);
		min_port = max_port;
	}
	linphone_core_set_audio_port_range(linphone_gtk_get_core(), min_port, max_port);
}

void linphone_gtk_min_video_port_changed(GtkWidget *w){
	GtkWidget *mw = linphone_gtk_get_main_window();
	GtkWidget *pb = (GtkWidget *) g_object_get_data(G_OBJECT(mw), "parameters");
	GtkSpinButton *min_button = GTK_SPIN_BUTTON(w);
	GtkSpinButton *max_button = GTK_SPIN_BUTTON(linphone_gtk_get_widget(pb, "video_max_rtp_port"));
	gboolean fixed = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(pb, "fixed_video_port")));

	if (fixed) {
		linphone_core_set_video_port(linphone_gtk_get_core(), (gint) gtk_spin_button_get_value(min_button));
		gtk_spin_button_set_value(max_button, gtk_spin_button_get_value(min_button));
	} else {
		gint min_port = (gint)gtk_spin_button_get_value(min_button);
		gint max_port = (gint)gtk_spin_button_get_value(max_button);
		if (min_port > max_port) {
			gtk_spin_button_set_value(max_button, min_port);
			max_port = min_port;
		}
		linphone_core_set_video_port_range(linphone_gtk_get_core(), min_port, max_port);
	}
}

void linphone_gtk_max_video_port_changed(GtkWidget *w){
	GtkWidget *mw = linphone_gtk_get_main_window();
	GtkWidget *pb = (GtkWidget *) g_object_get_data(G_OBJECT(mw), "parameters");
	GtkSpinButton *min_button = GTK_SPIN_BUTTON(linphone_gtk_get_widget(pb, "video_min_rtp_port"));
	GtkSpinButton *max_button = GTK_SPIN_BUTTON(w);
	gint min_port = (gint)gtk_spin_button_get_value(min_button);
	gint max_port = (gint)gtk_spin_button_get_value(max_button);
	if (max_port < min_port) {
		gtk_spin_button_set_value(min_button, max_port);
		min_port = max_port;
	}
	linphone_core_set_video_port_range(linphone_gtk_get_core(), min_port, max_port);
}

void linphone_gtk_no_firewall_toggled(GtkWidget *w){
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))){
		linphone_gtk_set_propety_entry(w,FALSE,FALSE);
		linphone_core_set_firewall_policy(linphone_gtk_get_core(),LinphonePolicyNoFirewall);
	}
}

void linphone_gtk_use_nat_address_toggled(GtkWidget *w){
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))){
		linphone_gtk_set_propety_entry(w,FALSE,TRUE);
		linphone_core_set_firewall_policy(linphone_gtk_get_core(),LinphonePolicyUseNatAddress);
	}
}

void linphone_gtk_use_stun_toggled(GtkWidget *w){
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))){
		linphone_gtk_set_propety_entry(w,TRUE,FALSE);
		linphone_core_set_firewall_policy(linphone_gtk_get_core(),LinphonePolicyUseStun);
	}
}

void linphone_gtk_use_ice_toggled(GtkWidget *w){
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))){
		linphone_gtk_set_propety_entry(w,TRUE,FALSE);
		linphone_core_set_firewall_policy(linphone_gtk_get_core(),LinphonePolicyUseIce);
	}
}

void linphone_gtk_use_upnp_toggled(GtkWidget *w){
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))){
		linphone_gtk_set_propety_entry(w,FALSE,FALSE);
		linphone_core_set_firewall_policy(linphone_gtk_get_core(),LinphonePolicyUseUpnp);
	}
}

void linphone_gtk_mtu_changed(GtkWidget *w){
	if (GTK_WIDGET_SENSITIVE(w))
		linphone_core_set_mtu(linphone_gtk_get_core(),(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(w)));
}

void linphone_gtk_use_sip_info_dtmf_toggled(GtkWidget *w){
	linphone_core_set_use_info_for_dtmf(linphone_gtk_get_core(),
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)));
}

void linphone_gtk_mtu_set(GtkWidget *w){
	GtkWidget *mtu=linphone_gtk_get_widget(gtk_widget_get_toplevel(w),"mtu");
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))){
		gtk_widget_set_sensitive(mtu,TRUE);
		linphone_gtk_mtu_changed(mtu);
	}else{
		gtk_widget_set_sensitive(mtu,FALSE);
		linphone_core_set_mtu(linphone_gtk_get_core(),0);
	}
}

void linphone_gtk_playback_device_changed(GtkWidget *w){
	gchar *sel=gtk_combo_box_get_active_text(GTK_COMBO_BOX(w));
	linphone_core_set_playback_device(linphone_gtk_get_core(),sel);
	g_free(sel);
}

void linphone_gtk_capture_device_changed(GtkWidget *w){
	gchar *sel=gtk_combo_box_get_active_text(GTK_COMBO_BOX(w));
	linphone_core_set_capture_device(linphone_gtk_get_core(),sel);
	g_free(sel);
}

void linphone_gtk_ring_device_changed(GtkWidget *w){
	gchar *sel=gtk_combo_box_get_active_text(GTK_COMBO_BOX(w));
	linphone_core_set_ringer_device(linphone_gtk_get_core(),sel);
	g_free(sel);
}

void linphone_gtk_alsa_special_device_changed(GtkWidget *w){
	/*
	const gchar *dev=gtk_entry_get_text(GTK_ENTRY(w));
	...*/
}

void linphone_gtk_cam_changed(GtkWidget *w){
	LinphoneCall *call;
	LinphoneCore *lc = linphone_gtk_get_core();
	gchar *sel=gtk_combo_box_get_active_text(GTK_COMBO_BOX(w));
	linphone_core_set_video_device(linphone_gtk_get_core(),sel);
	if ((call = linphone_core_get_current_call(lc)) != NULL) {
		linphone_core_update_call(lc, call, NULL);
	}
	g_free(sel);
}

void linphone_gtk_video_size_changed(GtkWidget *w){
	int sel=gtk_combo_box_get_active(GTK_COMBO_BOX(w));
	const MSVideoSizeDef *defs=linphone_core_get_supported_video_sizes(linphone_gtk_get_core());
	if (sel<0) return;
	linphone_core_set_preferred_video_size(linphone_gtk_get_core(),
					defs[sel].vsize);
}

void linphone_gtk_video_renderer_changed(GtkWidget *w){
	GtkTreeIter iter;
	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(w),&iter)){
		GtkTreeModel *model=gtk_combo_box_get_model(GTK_COMBO_BOX(w));
		gchar *name;
		gtk_tree_model_get(model,&iter,0,&name,-1);
		linphone_core_set_video_display_filter(linphone_gtk_get_core(),name);
	}
}

void linphone_gtk_video_preset_changed(GtkWidget *w) {
	gchar *sel = gtk_combo_box_get_active_text(GTK_COMBO_BOX(w));
	GtkSpinButton *framerate = GTK_SPIN_BUTTON(linphone_gtk_get_widget(gtk_widget_get_toplevel(w), "video_framerate"));
	LinphoneCore *lc = linphone_gtk_get_core();
	if (g_strcmp0(sel, "default") == 0) {
		linphone_core_set_video_preset(lc, NULL);
		gtk_spin_button_set_value(framerate, 0);
		gtk_widget_set_sensitive(GTK_WIDGET(framerate), FALSE);
	} else if (g_strcmp0(sel, "high-fps") == 0) {
		linphone_core_set_video_preset(lc, "high-fps");
		gtk_spin_button_set_value(framerate, 30);
		gtk_widget_set_sensitive(GTK_WIDGET(framerate), FALSE);
	} else if (g_strcmp0(sel, "custom") == 0) {
		linphone_core_set_video_preset(lc, "custom");
		gtk_spin_button_set_value(framerate, 30);
		gtk_widget_set_sensitive(GTK_WIDGET(framerate), TRUE);
	}
	g_free(sel);
}

void linphone_gtk_ring_file_set(GtkWidget *w){
	gchar *file=gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(w));
	linphone_core_set_ring(linphone_gtk_get_core(),file);
	g_free(file);
}

static void linphone_gtk_end_of_ring(LinphoneCore *lc, void *user_data){
	gtk_widget_set_sensitive((GtkWidget*)user_data,TRUE);
}

void linphone_gtk_play_ring_file(GtkWidget *w){
	if (linphone_core_preview_ring(linphone_gtk_get_core(),
				linphone_core_get_ring(linphone_gtk_get_core()),
				linphone_gtk_end_of_ring,
				w)==0){
		gtk_widget_set_sensitive(w,FALSE);
	}
}

void linphone_gtk_echo_cancelation_toggled(GtkWidget *w){
	linphone_core_enable_echo_cancellation(linphone_gtk_get_core(),
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)));
}

enum {
	CODEC_NAME,
	CODEC_RATE,
	CODEC_BITRATE,
	CODEC_STATUS,
	CODEC_PARAMS,
	CODEC_PRIVDATA,
	CODEC_COLOR,
	CODEC_INFO,
	CODEC_NCOLUMNS
};

static void fmtp_edited(GtkCellRendererText *renderer, gchar *path, gchar *new_text, gpointer userdata){
	GtkListStore *store=(GtkListStore*)userdata;
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store),&iter,path)){
		PayloadType *pt;
		gtk_list_store_set(store,&iter,CODEC_PARAMS,new_text,-1);
		gtk_tree_model_get(GTK_TREE_MODEL(store),&iter,CODEC_PRIVDATA,&pt,-1);
		payload_type_set_recv_fmtp(pt,new_text);
	}
}

static void bitrate_edited(GtkCellRendererText *renderer, gchar *path, gchar *new_text, gpointer userdata){
	GtkListStore *store=(GtkListStore*)userdata;
	GtkTreeIter iter;
	float newbitrate=0;

	if (!new_text) return;

	if (sscanf(new_text, "%f", &newbitrate)!=1) return;

	if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(store),&iter,path)){
		PayloadType *pt;
		gtk_list_store_set(store,&iter,CODEC_BITRATE,newbitrate,-1);
		gtk_tree_model_get(GTK_TREE_MODEL(store),&iter,CODEC_PRIVDATA,&pt,-1);
		linphone_core_set_payload_type_bitrate(linphone_gtk_get_core(), pt, (int)newbitrate);
	}
}

static void linphone_gtk_init_codec_list(GtkTreeView *listview){
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *select;
	GValue editable = {0};

	GtkListStore *store = gtk_list_store_new (CODEC_NCOLUMNS, G_TYPE_STRING,G_TYPE_INT,
							G_TYPE_FLOAT,
							G_TYPE_STRING,
							G_TYPE_STRING,
							G_TYPE_POINTER,
							G_TYPE_STRING,
							G_TYPE_STRING);

	gtk_tree_view_set_model(listview,GTK_TREE_MODEL(store));
	g_object_unref(G_OBJECT(store));

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Name"),
                                                   renderer,
                                                   "text", CODEC_NAME,
						"foreground",CODEC_COLOR,
                                                   NULL);
	gtk_tree_view_append_column (listview, column);
	column = gtk_tree_view_column_new_with_attributes (_("Rate (Hz)"),
                                                   renderer,
                                                   "text", CODEC_RATE,
						"foreground",CODEC_COLOR,
                                                   NULL);
	gtk_tree_view_append_column (listview, column);
	column = gtk_tree_view_column_new_with_attributes (_("Status"),
                                                   renderer,
                                                   "text", CODEC_STATUS,
						"foreground",CODEC_COLOR,
                                                   NULL);
	gtk_tree_view_append_column (listview, column);

	g_value_init(&editable, G_TYPE_BOOLEAN);
	g_value_set_boolean(&editable, TRUE);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set_property(G_OBJECT(renderer), "editable", &editable);
	column = gtk_tree_view_column_new_with_attributes (
		_("IP Bitrate (kbit/s)"),
		renderer,
		"text", CODEC_BITRATE,
		"foreground",CODEC_COLOR,
		NULL);
	g_signal_connect(G_OBJECT(renderer),"edited",G_CALLBACK(bitrate_edited),store);
	gtk_tree_view_append_column (listview, column);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set_property(G_OBJECT(renderer), "editable", &editable);
	column = gtk_tree_view_column_new_with_attributes (
		_("Parameters"),
		renderer,
		"text", CODEC_PARAMS,
		"foreground",CODEC_COLOR,
		NULL);
	g_signal_connect(G_OBJECT(renderer),"edited",G_CALLBACK(fmtp_edited),store);
	gtk_tree_view_append_column (listview, column);
	/* Setup the selection handler */
	select = gtk_tree_view_get_selection (listview);
	gtk_tree_selection_set_mode (select, GTK_SELECTION_MULTIPLE);
}


const char *get_codec_color(LinphoneCore *lc, PayloadType *pt){
	const gchar *color;
	if (linphone_core_check_payload_type_usability(lc,pt)) color="blue";
		else color="red";
	if (!linphone_core_payload_type_enabled(lc,pt)) {
		color="grey";
	}
	return color;
}

static void linphone_gtk_show_codecs(GtkTreeView *listview, const bctbx_list_t *codeclist)
{
	const bctbx_list_t *elem;
	GtkTreeIter iter;
	GtkListStore *store=GTK_LIST_STORE(gtk_tree_view_get_model(listview));
// 	GtkTreeSelection *selection;

	gtk_list_store_clear(store);

	for(elem=codeclist; elem!=NULL; elem=elem->next){
		gchar *status;
		gint rate;
		gfloat bitrate;
		const gchar *color;
		const char *params="";

		struct _PayloadType *pt=(struct _PayloadType *)elem->data;

		color=get_codec_color(linphone_gtk_get_core(),pt);
		if (linphone_core_payload_type_enabled(linphone_gtk_get_core(),pt)) status=_("Enabled");
		else {
			status=_("Disabled");
		}
		/* get an iterator */
		gtk_list_store_append(store,&iter);
		bitrate=(gfloat)linphone_core_get_payload_type_bitrate(linphone_gtk_get_core(),pt);
		rate=payload_type_get_rate(pt);
		if (pt->recv_fmtp!=NULL) params=pt->recv_fmtp;
		gtk_list_store_set(store,&iter,	CODEC_NAME,payload_type_get_mime(pt),
					CODEC_RATE,rate,
					CODEC_BITRATE,bitrate,
					CODEC_STATUS,status,
					CODEC_PARAMS,params,
					CODEC_PRIVDATA,(gpointer)pt,
					CODEC_COLOR,(gpointer)color,
					CODEC_INFO,(gpointer)linphone_core_get_payload_type_description(linphone_gtk_get_core(),pt),
					-1);
	}



	/* Setup the selection handler */
// 	selection = gtk_tree_view_get_selection (listview);
// 	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	//gtk_tree_view_columns_autosize(GTK_TREE_VIEW (sec->interfaces));
#if GTK_CHECK_VERSION(2,12,0)
	gtk_tree_view_set_tooltip_column(listview,CODEC_INFO);
#endif
}

static void linphone_gtk_check_codec_bandwidth(GtkTreeView *v){
	GtkTreeIter iter;
	GtkTreeModel *model;
	model=gtk_tree_view_get_model(v);
	g_return_if_fail(gtk_tree_model_get_iter_first(model,&iter));
	do{
		PayloadType *pt=NULL;

		gfloat bitrate;
		gtk_tree_model_get(model,&iter,CODEC_PRIVDATA,&pt,-1);

		bitrate=(gfloat)linphone_core_get_payload_type_bitrate(linphone_gtk_get_core(),pt);
		gtk_list_store_set(GTK_LIST_STORE(model),&iter,CODEC_COLOR, (gpointer)get_codec_color(linphone_gtk_get_core(),pt),
					CODEC_BITRATE, bitrate,-1);
	}while(gtk_tree_model_iter_next(model,&iter));
}

static void linphone_gtk_select_codec(GtkTreeView *v, PayloadType *ref){
	GtkTreeIter iter;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	selection=gtk_tree_view_get_selection(v);
	model=gtk_tree_view_get_model(v);
	g_return_if_fail(gtk_tree_model_get_iter_first(model,&iter));
	do{
		PayloadType *pt=NULL;
		gtk_tree_model_get(model,&iter,CODEC_PRIVDATA,&pt,-1);
		if (pt==ref){
			gtk_tree_selection_select_iter(selection,&iter);
		}

	}while(gtk_tree_model_iter_next(model,&iter));
}

static void linphone_gtk_draw_codec_list(GtkTreeView *v, int type){ /* 0=audio, 1=video*/
	const bctbx_list_t *list;
	if (type==0) list=linphone_core_get_audio_codecs(linphone_gtk_get_core());
	else list=linphone_core_get_video_codecs(linphone_gtk_get_core());
	linphone_gtk_show_codecs(v,list);
}

void linphone_gtk_download_bw_changed(GtkWidget *w){
	GtkTreeView *audiov=GTK_TREE_VIEW(linphone_gtk_get_widget(gtk_widget_get_toplevel(w),"audio_codec_list"));
	GtkTreeView *videov=GTK_TREE_VIEW(linphone_gtk_get_widget(gtk_widget_get_toplevel(w),"video_codec_list"));
	linphone_core_set_download_bandwidth(linphone_gtk_get_core(),
				(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(w)));
	linphone_gtk_check_codec_bandwidth(audiov);
	linphone_gtk_check_codec_bandwidth(videov);
}

void linphone_gtk_upload_bw_changed(GtkWidget *w){
	GtkTreeView *audiov=GTK_TREE_VIEW(linphone_gtk_get_widget(gtk_widget_get_toplevel(w),"audio_codec_list"));
	GtkTreeView *videov=GTK_TREE_VIEW(linphone_gtk_get_widget(gtk_widget_get_toplevel(w),"video_codec_list"));
	linphone_core_set_upload_bandwidth(linphone_gtk_get_core(),
				(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(w)));
	linphone_gtk_check_codec_bandwidth(audiov);
	linphone_gtk_check_codec_bandwidth(videov);
}

void linphone_gtk_video_framerate_changed(GtkWidget *w) {
	linphone_core_set_preferred_framerate(linphone_gtk_get_core(),
		(float)(int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(w)));
}

void linphone_gtk_adaptive_rate_control_toggled(GtkToggleButton *button){
	gboolean active=gtk_toggle_button_get_active(button);
	linphone_core_enable_adaptive_rate_control(linphone_gtk_get_core(),active);
}

static void _g_list_func_destroy_tree_path(gpointer data, gpointer user_data) {
	GtkTreePath *tree_path = (GtkTreePath *)data;
	gtk_tree_path_free(tree_path);
}

static void linphone_gtk_codec_move(GtkWidget *button, int dir, int type){ /* 0=audio, 1=video*/
	GtkTreeView *v;
	GtkTreeSelection *sel;
	GtkTreeModel *mod;
	GtkTreeIter iter;
	PayloadType *pt=NULL;
	LinphoneCore *lc=linphone_gtk_get_core();

	if (type == 0) v=GTK_TREE_VIEW(linphone_gtk_get_widget(gtk_widget_get_toplevel(button),"audio_codec_list"));
	else v=GTK_TREE_VIEW(linphone_gtk_get_widget(gtk_widget_get_toplevel(button),"video_codec_list"));
	sel=gtk_tree_view_get_selection(v);
	if (gtk_tree_selection_count_selected_rows(sel) == 1){
		bctbx_list_t *sel_elem,*before;
		bctbx_list_t *codec_list;

		GList *selected_rows = gtk_tree_selection_get_selected_rows(sel, &mod);
		gtk_tree_model_get_iter(mod, &iter, (GtkTreePath *)g_list_nth_data(selected_rows, 0));
		gtk_tree_model_get(mod,&iter,CODEC_PRIVDATA,&pt,-1);
		g_list_foreach(selected_rows, _g_list_func_destroy_tree_path, NULL);
		g_list_free(selected_rows);

		if (pt->type==PAYLOAD_VIDEO)
			codec_list=bctbx_list_copy(linphone_core_get_video_codecs(lc));
		else codec_list=bctbx_list_copy(linphone_core_get_audio_codecs(lc));
		sel_elem=bctbx_list_find(codec_list,pt);
		if (dir>0) {
			if (sel_elem->prev) before=sel_elem->prev;
			else before=sel_elem;
			codec_list=bctbx_list_insert(codec_list,before,pt);
		}
		else{
			if (sel_elem->next) before=sel_elem->next->next;
			else before=sel_elem;
			codec_list=bctbx_list_insert(codec_list,before,pt);
		}
		codec_list=bctbx_list_erase_link(codec_list,sel_elem);
		if (pt->type==PAYLOAD_VIDEO)
			linphone_core_set_video_codecs(lc,codec_list);
		else linphone_core_set_audio_codecs(lc,codec_list);
		linphone_gtk_show_codecs(v,codec_list);
		linphone_gtk_select_codec(v,pt);
	}
}

static void linphone_gtk_codec_set_enable(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data) {
	PayloadType *pt=NULL;
	gboolean *enabled = (gboolean *)data;
	gtk_tree_model_get(model, iter, CODEC_PRIVDATA, &pt, -1);
	linphone_core_enable_payload_type(linphone_gtk_get_core(), pt, *enabled);
	gtk_list_store_set(GTK_LIST_STORE(model), iter, CODEC_STATUS, *enabled ? _("Enabled") : _("Disabled"),
		CODEC_COLOR,(gpointer)get_codec_color(linphone_gtk_get_core(),pt), -1);
}

static void linphone_gtk_codec_set_enable_all_selected(GtkWidget *button, gboolean enabled, int type){ /* 0=audio, 1=video*/
	GtkTreeView *v;
	GtkTreeSelection *sel;
	if (type == 0) v=GTK_TREE_VIEW(linphone_gtk_get_widget(gtk_widget_get_toplevel(button),"audio_codec_list"));
	else v=GTK_TREE_VIEW(linphone_gtk_get_widget(gtk_widget_get_toplevel(button),"video_codec_list"));
	sel=gtk_tree_view_get_selection(v);
	gtk_tree_selection_selected_foreach(sel, linphone_gtk_codec_set_enable, &enabled);
	if (type == 0){
		/*activating audio and video codecs has consequences on video bandwidth*/
		GtkTreeView *videov=GTK_TREE_VIEW(linphone_gtk_get_widget(gtk_widget_get_toplevel(button),"video_codec_list"));
		linphone_gtk_check_codec_bandwidth(videov);
	}
}

void linphone_gtk_audio_codec_up(GtkWidget *button){
	linphone_gtk_codec_move(button,+1,0);
}

void linphone_gtk_audio_codec_down(GtkWidget *button){
	linphone_gtk_codec_move(button,-1,0);
}

void linphone_gtk_audio_codec_enable(GtkWidget *button){
	linphone_gtk_codec_set_enable_all_selected(button,TRUE,0);
}

void linphone_gtk_audio_codec_disable(GtkWidget *button){
	linphone_gtk_codec_set_enable_all_selected(button,FALSE,0);
}

void linphone_gtk_video_codec_up(GtkWidget *button){
	linphone_gtk_codec_move(button,+1,1);
}

void linphone_gtk_video_codec_down(GtkWidget *button){
	linphone_gtk_codec_move(button,-1,1);
}

void linphone_gtk_video_codec_enable(GtkWidget *button){
	linphone_gtk_codec_set_enable_all_selected(button,TRUE,1);
}

void linphone_gtk_video_codec_disable(GtkWidget *button){
	linphone_gtk_codec_set_enable_all_selected(button,FALSE,1);
}

void linphone_gtk_clear_passwords(GtkWidget *button){
	linphone_core_clear_all_auth_info(linphone_gtk_get_core());
}

enum{
	PROXY_CONFIG_IDENTITY,
	PROXY_CONFIG_REF,
	PROXY_CONFIG_NCOL
};

void linphone_gtk_show_sip_accounts(GtkWidget *w){
	GtkTreeView *v=GTK_TREE_VIEW(linphone_gtk_get_widget(w,"proxy_list"));
	GtkTreeModel *model=gtk_tree_view_get_model(v);
	GtkListStore *store;
	GtkTreeSelection *select;
	const LinphoneProxyConfig *default_pc = linphone_core_get_default_proxy_config(linphone_gtk_get_core());
	GtkTreePath *default_pc_path = NULL;

	const bctbx_list_t *elem;
	if (!model){
		GtkCellRenderer *renderer;
		GtkTreeViewColumn *column;
		/* create the proxy list */
		store = gtk_list_store_new (PROXY_CONFIG_NCOL, G_TYPE_STRING, G_TYPE_POINTER);

		gtk_tree_view_set_model(v,GTK_TREE_MODEL(store));
		g_object_unref(G_OBJECT(store));
		renderer = gtk_cell_renderer_text_new ();
		column = gtk_tree_view_column_new_with_attributes (_("Account"),
							renderer,
							"text", PROXY_CONFIG_IDENTITY,
							NULL);
		gtk_tree_view_append_column (v, column);

		select = gtk_tree_view_get_selection (v);
		gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);
		model=GTK_TREE_MODEL(store);
	}else {
		store=GTK_LIST_STORE(model);
	}
	gtk_list_store_clear(store);
	for(elem=linphone_core_get_proxy_config_list(linphone_gtk_get_core());elem!=NULL;elem=bctbx_list_next(elem)){
		LinphoneProxyConfig *cfg=(LinphoneProxyConfig*)elem->data;
		GtkTreeIter iter;
		gtk_list_store_append(store,&iter);
		gtk_list_store_set(store,&iter,PROXY_CONFIG_IDENTITY,linphone_proxy_config_get_identity(cfg),
					PROXY_CONFIG_REF,cfg,-1);
		if(cfg == default_pc) default_pc_path = gtk_tree_model_get_path(GTK_TREE_MODEL(store), &iter);
	}
	if(default_pc_path) {
		gtk_tree_selection_select_path(gtk_tree_view_get_selection(v), default_pc_path);
		gtk_tree_path_free(default_pc_path);
	}
}

static void linphone_gtk_proxy_closed(GtkWidget *w){
	LinphoneProxyConfig *cfg=(LinphoneProxyConfig*)g_object_get_data(G_OBJECT(w),"config");
	gboolean was_editing=! GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w),"is_new"));
	if (cfg){
		if (was_editing){
			linphone_proxy_config_done(cfg);
		}else linphone_proxy_config_destroy(cfg);
	}
}

static void fill_transport_combo_box(GtkWidget *combo, LinphoneTransportType choice, gboolean is_sensitive){
	GtkTreeModel *model;
	GtkTreeIter iter;

	if (GPOINTER_TO_INT(g_object_get_data(G_OBJECT(combo),"combo-updating"))) return;

	if ((model=gtk_combo_box_get_model(GTK_COMBO_BOX(combo)))==NULL){
		/*case where combo box is created with no model*/
		GtkCellRenderer *renderer=gtk_cell_renderer_text_new();
		model=GTK_TREE_MODEL(gtk_list_store_new(1,G_TYPE_STRING));
		gtk_combo_box_set_model(GTK_COMBO_BOX(combo),model);
		gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo),renderer,TRUE);
		gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo),renderer,"markup",0,NULL);
	}
	if (!gtk_tree_model_get_iter_first(model,&iter)){
		gtk_list_store_append(GTK_LIST_STORE(model),&iter);
		gtk_list_store_set(GTK_LIST_STORE(model),&iter,0,"UDP",-1);
		gtk_list_store_append(GTK_LIST_STORE(model),&iter);
		gtk_list_store_set(GTK_LIST_STORE(model),&iter,0,"TCP",-1);
		if (linphone_core_sip_transport_supported(linphone_gtk_get_core(),LinphoneTransportTls)){
			gtk_list_store_append(GTK_LIST_STORE(model),&iter);
			gtk_list_store_set(GTK_LIST_STORE(model),&iter,0,"TLS",-1);
		}
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(combo),(int)choice);
	gtk_widget_set_sensitive(combo,is_sensitive);
}

static void update_proxy_transport(GtkWidget *w){
	const char *addr=gtk_entry_get_text(GTK_ENTRY(linphone_gtk_get_widget(w,"proxy")));
	LinphoneAddress *laddr=linphone_address_new(addr);
	if (laddr){
		GtkWidget *combo=linphone_gtk_get_widget(w,"transport");
		if (linphone_address_is_secure(laddr)){
			fill_transport_combo_box(combo,LinphoneTransportTls,FALSE);
		}else{
			fill_transport_combo_box(combo,linphone_address_get_transport(laddr),TRUE);
		}
		linphone_address_unref(laddr);
	}
}

void linphone_gtk_proxy_transport_changed(GtkWidget *combo){
	GtkWidget *w=gtk_widget_get_toplevel(combo);
	int index=gtk_combo_box_get_active(GTK_COMBO_BOX(combo));
	GtkWidget *proxy=linphone_gtk_get_widget(w,"proxy");
	const char *addr=gtk_entry_get_text(GTK_ENTRY(proxy));
	LinphoneAddress *laddr;
	LinphoneTransportType new_transport=(LinphoneTransportType)index;

	if (index==-1) return;

	g_object_set_data(G_OBJECT(w),"combo-updating",GINT_TO_POINTER(1));
	laddr=linphone_address_new(addr);
	if (laddr){
		if (linphone_address_get_transport(laddr)!=new_transport){
			char *newaddr;
			linphone_address_set_transport(laddr,new_transport);
			newaddr=linphone_address_as_string(laddr);
			gtk_entry_set_text(GTK_ENTRY(proxy),newaddr);
			ms_free(newaddr);
		}
		linphone_address_unref(laddr);
	}
	g_object_set_data(G_OBJECT(w),"combo-updating",GINT_TO_POINTER(0));
}

void linphone_gtk_proxy_address_changed(GtkEditable *editable){
	update_proxy_transport(gtk_widget_get_toplevel(GTK_WIDGET(editable)));
}

void linphone_gtk_show_proxy_config(GtkWidget *pb, LinphoneProxyConfig *cfg){
	GtkWidget *w=linphone_gtk_create_window("sip_account", gtk_widget_get_toplevel(pb));
	const char *tmp;
	gboolean is_new=FALSE;

	if (!cfg) {
		cfg=linphone_core_create_proxy_config(linphone_gtk_get_core());
		is_new=TRUE;
		g_object_set_data(G_OBJECT(w),"is_new",GINT_TO_POINTER(TRUE));
	}

	if (!is_new){
		linphone_proxy_config_edit(cfg);
		gtk_entry_set_text(GTK_ENTRY(linphone_gtk_get_widget(w,"identity")),
			linphone_proxy_config_get_identity(cfg));
		gtk_entry_set_text(GTK_ENTRY(linphone_gtk_get_widget(w,"proxy")),linphone_proxy_config_get_addr(cfg));
		tmp=linphone_proxy_config_get_route(cfg);
		if (tmp) gtk_entry_set_text(GTK_ENTRY(linphone_gtk_get_widget(w,"route")),tmp);
		tmp=linphone_proxy_config_get_contact_parameters(cfg);
		if (tmp) gtk_entry_set_text(GTK_ENTRY(linphone_gtk_get_widget(w,"params")),tmp);
	}

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(linphone_gtk_get_widget(w,"regperiod")),
		linphone_proxy_config_get_expires(cfg));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(w,"register")),
		linphone_proxy_config_register_enabled(cfg));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(w,"publish")),
		linphone_proxy_config_publish_enabled(cfg));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(w,"avpf")),
		linphone_proxy_config_avpf_enabled(cfg));
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(linphone_gtk_get_widget(w,"avpf_rr_interval")),
		linphone_proxy_config_get_avpf_rr_interval(cfg));

	g_object_set_data(G_OBJECT(w),"config",(gpointer)cfg);
	g_object_set_data(G_OBJECT(w),"parameters",(gpointer)pb);
	g_object_weak_ref(G_OBJECT(w),(GWeakNotify)linphone_gtk_proxy_closed,w);
	gtk_widget_show(w);
}

void linphone_gtk_proxy_cancel(GtkButton *button){
	GtkWidget *w=gtk_widget_get_toplevel(GTK_WIDGET(button));
	gtk_widget_destroy(w);
}

void linphone_gtk_proxy_ok(GtkButton *button){
	LinphoneCore *lc=linphone_gtk_get_core();
	GtkWidget *w=gtk_widget_get_toplevel(GTK_WIDGET(button));
	LinphoneProxyConfig *cfg=(LinphoneProxyConfig*)g_object_get_data(G_OBJECT(w),"config");
	int index=gtk_combo_box_get_active(GTK_COMBO_BOX(linphone_gtk_get_widget(w,"transport")));
	LinphoneTransportType tport=(LinphoneTransportType)index;
	gboolean was_editing=! GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w),"is_new"));

	linphone_proxy_config_set_identity(cfg,
		gtk_entry_get_text(GTK_ENTRY(linphone_gtk_get_widget(w,"identity"))));
	if (linphone_proxy_config_set_server_addr(cfg,
		gtk_entry_get_text(GTK_ENTRY(linphone_gtk_get_widget(w,"proxy"))))==0){
		if (index!=-1){
			/*make sure transport was added to proxy address*/
			LinphoneAddress *laddr=linphone_address_new(linphone_proxy_config_get_addr(cfg));
			if (laddr){
				if (linphone_address_get_transport(laddr)!=tport){
					char *tmp;
					linphone_address_set_transport(laddr,tport);
					tmp=linphone_address_as_string(laddr);
					linphone_proxy_config_set_server_addr(cfg,tmp);
					ms_free(tmp);
				}
				linphone_address_unref(laddr);
			}
		}
	}
	linphone_proxy_config_set_route(cfg,
		gtk_entry_get_text(GTK_ENTRY(linphone_gtk_get_widget(w,"route"))));
	linphone_proxy_config_set_contact_parameters(cfg,
		gtk_entry_get_text(GTK_ENTRY(linphone_gtk_get_widget(w,"params"))));
	linphone_proxy_config_set_expires(cfg,
		(int)gtk_spin_button_get_value(
			GTK_SPIN_BUTTON(linphone_gtk_get_widget(w,"regperiod"))));
	linphone_proxy_config_enable_publish(cfg,
		gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(w,"publish"))));
	linphone_proxy_config_enable_register(cfg,
		gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(w,"register"))));
	linphone_proxy_config_enable_avpf(cfg,
		gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(w,"avpf"))));
	linphone_proxy_config_set_avpf_rr_interval(cfg,
		(int)gtk_spin_button_get_value(
			GTK_SPIN_BUTTON(linphone_gtk_get_widget(w,"avpf_rr_interval"))));

	/* check if tls was asked but is not enabled in transport configuration*/
	if (tport==LinphoneTransportTls){
		LCSipTransports tports;
		linphone_core_get_sip_transports(lc,&tports);
		if (tports.tls_port==LC_SIP_TRANSPORT_DISABLED){
			tports.tls_port=LC_SIP_TRANSPORT_RANDOM;
		}
		linphone_core_set_sip_transports(lc,&tports);
	}

	if (was_editing){
		if (linphone_proxy_config_done(cfg)==-1)
			return;
	}
	else {
		if (linphone_core_add_proxy_config(linphone_gtk_get_core(),cfg)==-1) return;
		linphone_core_set_default_proxy(linphone_gtk_get_core(),cfg);
	}
	g_object_set_data(G_OBJECT(w),"config",NULL);
	linphone_gtk_show_sip_accounts(GTK_WIDGET(g_object_get_data(G_OBJECT(w),"parameters")));
	gtk_widget_destroy(w);
	/* also update the main window's list of identities*/
	linphone_gtk_load_identities();
}

static LinphoneProxyConfig *linphone_gtk_get_selected_proxy_config(GtkWidget* pb){
	GtkTreeView *v=GTK_TREE_VIEW(linphone_gtk_get_widget(pb,"proxy_list"));
	GtkTreeSelection *selection=gtk_tree_view_get_selection(v);
	GtkTreeIter iter;
	GtkTreeModel *model;
	if (gtk_tree_selection_get_selected(selection,&model,&iter)){
		LinphoneProxyConfig *cfg=NULL;
		gtk_tree_model_get(model,&iter,PROXY_CONFIG_REF,&cfg,-1);
		return cfg;
	}
	return NULL;
}

void linphone_gtk_add_proxy(GtkButton *button){
	linphone_gtk_show_proxy_config(gtk_widget_get_toplevel(GTK_WIDGET(button)),NULL);
}

void linphone_gtk_remove_proxy(GtkButton *button){
	LinphoneProxyConfig *cfg=linphone_gtk_get_selected_proxy_config(
			gtk_widget_get_toplevel(GTK_WIDGET(button)));
	if (cfg){
		linphone_core_remove_proxy_config(linphone_gtk_get_core(),cfg);
		linphone_gtk_show_sip_accounts(gtk_widget_get_toplevel(GTK_WIDGET(button)));
		/* also update the main window's list of identities*/
		linphone_gtk_load_identities();
	}
}

void linphone_gtk_edit_proxy(GtkButton *button){
	GtkWidget *pb=gtk_widget_get_toplevel(GTK_WIDGET(button));
	LinphoneProxyConfig *cfg=linphone_gtk_get_selected_proxy_config(pb);
	if (cfg){
		linphone_gtk_show_proxy_config(pb,cfg);
		/* also update the main window's list of identities*/
		linphone_gtk_load_identities();
	}
}

typedef struct _LangCodes{
	const char *code;
	const char *name;
}LangCodes;

static LangCodes supported_langs[]={
	{	"C"	,	N_("English")	},
	{	"fr"	,	N_("French")	},
	{	"sv"	,	N_("Swedish")	},
	{	"it"	,	N_("Italian")	},
	{	"es"	,	N_("Spanish")	},
	{	"pt_BR"	,	N_("Brazilian Portugese")	},
	{	"pl"	,	N_("Polish")	},
	{	"de"	,	N_("German")	},
	{	"ru"	,	N_("Russian")	},
	{	"ja"	,	N_("Japanese")	},
	{	"nl"	,	N_("Dutch")	},
	{	"hu"	,	N_("Hungarian")	},
	{	"cs"	,	N_("Czech")	},
	{	"zh_CN" ,	N_("Chinese")	},
	{	"zh_TW"	,	N_("Traditional Chinese")	},
	{	"nb_NO"	,	N_("Norwegian")	},
	{	"he"	,	N_("Hebrew")	},
	{	"sr"	,	N_("Serbian")	},
	{	"ar"	,	N_("Arabic")	},
	{	"tr"	,	N_("Turkish")	},
	{	"fi"	,	N_("Finnish")	},
	{	"lt"	,	N_("Lithuanian")	},
	{	NULL	,	NULL		}
};

static const char *lang_get_name(const char *code){
	LangCodes *p=supported_langs;
	while(p->code!=NULL){
		if (strcmp(p->code,code)==0) return p->name;
		p++;
	}
	return NULL;
}

static gboolean lang_equals(const char *l1, const char *l2){
	return ((strncmp(l1,l2,5)==0 || strncmp(l1,l2,2)==0));
}

static void linphone_gtk_fill_langs(GtkWidget *pb){
	GtkWidget *combo=linphone_gtk_get_widget(pb,"lang_combo");
	char code[10];
	const char *all_langs="C " LINPHONE_ALL_LANGS;
	const char *name;
	int i=0,index=0;
	int cur_lang_index=-1;
	char text[256]={0};
	const char *cur_lang = g_getenv("LANGUAGE");
	if (cur_lang==NULL) cur_lang="C";
	/* glade creates a combo box without list model and text renderer,
	unless we fill it with a dummy text.
	This dummy text needs to be removed first*/
	gtk_combo_box_remove_text(GTK_COMBO_BOX(combo),0);
	while(sscanf(all_langs+i,"%s",code)==1){
		i+=strlen(code);
		while(all_langs[i]==' ') ++i;
		name=lang_get_name(code);
		snprintf(text,sizeof(text)-1,"%s : %s",code,name!=NULL ? _(name) : code);
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo),text);
		if (cur_lang_index==-1 && lang_equals(cur_lang,code))
			cur_lang_index=index;
		index++;
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(combo),cur_lang_index);
}

void linphone_gtk_lang_changed(GtkComboBox *combo){
	const char *selected=gtk_combo_box_get_active_text(combo);
	char code[10];
	const char *cur_lang=g_getenv("LANGUAGE");
	if (selected!=NULL){
		sscanf(selected,"%s",code);
		if (cur_lang==NULL) cur_lang="C";
		if (!lang_equals(cur_lang,code)){
			GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(combo))),
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_INFO,
				GTK_BUTTONS_CLOSE,
				"%s",
				(const gchar*)_("You need to restart linphone for the new language selection to take effect."));
				/* Destroy the dialog when the user responds to it (e.g. clicks a button) */
			g_signal_connect_swapped (G_OBJECT (dialog), "response",
					G_CALLBACK (gtk_widget_destroy),
					G_OBJECT (dialog));
			gtk_widget_show(dialog);
			linphone_gtk_set_lang(code);
		}
	}
}

static void linphone_gtk_ui_level_adapt(GtkWidget *top) {
	gboolean ui_advanced;
	const char *simple_ui = linphone_gtk_get_ui_config("simple_ui", "parameters.codec_tab parameters.transport_frame parameters.ports_frame parameters.bandwidth_frame");

	ui_advanced = linphone_gtk_get_ui_config_int("advanced_ui", FALSE);
	if (ui_advanced) {
		linphone_gtk_visibility_set(simple_ui, "parameters", top, TRUE);
	} else {
		linphone_gtk_visibility_set(simple_ui, "parameters", top, FALSE);
	}
}

void linphone_gtk_ui_level_toggled(GtkWidget *w) {
	gint ui_advanced;
	GtkWidget *top;

	top = gtk_widget_get_toplevel(w);
	ui_advanced = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
	linphone_gtk_set_ui_config_int("advanced_ui", ui_advanced);
	linphone_gtk_ui_level_adapt(top);
}

static void linphone_gtk_set_media_encryption_mandatory_sensitive(GtkWidget *propbox, gboolean val){
	GtkWidget *w=linphone_gtk_get_widget(propbox,"media_encryption_mandatory_checkbox");
	gtk_widget_set_sensitive(w,val);
}

static void linphone_gtk_media_encryption_changed(GtkWidget *combo){
	char *selected=gtk_combo_box_get_active_text(GTK_COMBO_BOX(combo));
	LinphoneCore *lc=linphone_gtk_get_core();
	GtkWidget *toplevel=gtk_widget_get_toplevel(combo);
	GtkWidget *mandatory_box = linphone_gtk_get_widget(toplevel,"media_encryption_mandatory_checkbox");
	if (selected!=NULL){
		if (strcasecmp(selected,"SRTP")==0){
			linphone_core_set_media_encryption(lc,LinphoneMediaEncryptionSRTP);
			gtk_widget_set_sensitive(mandatory_box,TRUE);
		}else if (strcasecmp(selected,"DTLS")==0){
			linphone_core_set_media_encryption(lc,LinphoneMediaEncryptionDTLS);
			gtk_widget_set_sensitive(mandatory_box,TRUE);
		}else if (strcasecmp(selected,"ZRTP")==0){
			linphone_core_set_media_encryption(lc,LinphoneMediaEncryptionZRTP);
			gtk_widget_set_sensitive(mandatory_box,TRUE);
		} else {
			linphone_core_set_media_encryption(lc,LinphoneMediaEncryptionNone);
			gtk_widget_set_sensitive(mandatory_box,FALSE);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mandatory_box), FALSE);
		}
		g_free(selected);
	}else g_warning("gtk_combo_box_get_active_text() returned NULL");
}

void linphone_gtk_set_media_encryption_mandatory(GtkWidget *button){
	linphone_core_set_media_encryption_mandatory(linphone_gtk_get_core(),gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)));
}

void linphone_gtk_lime_changed(GtkComboBoxText *combotext) {
	linphone_core_enable_lime(linphone_gtk_get_core(), gtk_combo_box_get_active(GTK_COMBO_BOX(combotext)));
}

static void linphone_gtk_show_media_encryption(GtkWidget *pb){
	LinphoneCore *lc=linphone_gtk_get_core();
	GtkWidget *combo=linphone_gtk_get_widget(pb,"media_encryption_combo");
	bool_t no_enc=TRUE;
	int srtp_id=-1,zrtp_id=-1,dtls_id=-1;
	GtkTreeModel *model;
	GtkListStore *store;
	GtkTreeIter iter;
	GtkCellRenderer *renderer=gtk_cell_renderer_text_new();

	model=GTK_TREE_MODEL((store=gtk_list_store_new(1,G_TYPE_STRING)));
	gtk_combo_box_set_model(GTK_COMBO_BOX(combo),model);
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo),renderer,TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo),renderer,"text",0,NULL);

	gtk_list_store_append(store,&iter);
	gtk_list_store_set(store,&iter,0,_("None"),-1);

	if (linphone_core_media_encryption_supported(lc,LinphoneMediaEncryptionSRTP)){
		gtk_list_store_append(store,&iter);
		gtk_list_store_set(store,&iter,0,_("SRTP"),-1);
		srtp_id=1;
		no_enc=FALSE;
	}
	if (linphone_core_media_encryption_supported(lc,LinphoneMediaEncryptionDTLS)){
		gtk_list_store_append(store,&iter);
		gtk_list_store_set(store,&iter,0,_("DTLS"),-1);
		if (srtp_id!=-1) dtls_id=2;
		else dtls_id=1;
		no_enc=FALSE;
	}
	if (linphone_core_media_encryption_supported(lc,LinphoneMediaEncryptionZRTP)){
		gtk_list_store_append(store,&iter);
		gtk_list_store_set(store,&iter,0,_("ZRTP"),-1);
		no_enc=FALSE;
		if (srtp_id!=-1) {
			if (dtls_id!=-1)
				zrtp_id=3;
			else zrtp_id=2;
		} else {
			if (dtls_id!=-1)
				zrtp_id=2;
			else zrtp_id=1;
		}
	}
	if (no_enc){
		/*hide this setting*/
		gtk_widget_hide(linphone_gtk_get_widget(pb,"encryption_label"));
		gtk_widget_hide(linphone_gtk_get_widget(pb,"encryption_table"));
	}else{
		LinphoneMediaEncryption menc=linphone_core_get_media_encryption(lc);
		gtk_widget_show(linphone_gtk_get_widget(pb,"encryption_label"));
		gtk_widget_show(linphone_gtk_get_widget(pb,"encryption_table"));

		switch(menc){
			case LinphoneMediaEncryptionNone:
				gtk_combo_box_set_active(GTK_COMBO_BOX(combo),0);
				linphone_gtk_set_media_encryption_mandatory_sensitive(pb,FALSE);
			break;
			case LinphoneMediaEncryptionSRTP:
				if (srtp_id!=-1) {
					gtk_combo_box_set_active(GTK_COMBO_BOX(combo),srtp_id);
					linphone_gtk_set_media_encryption_mandatory_sensitive(pb,TRUE);
				}
			break;
			case LinphoneMediaEncryptionDTLS:
				if (dtls_id!=-1) {
					gtk_combo_box_set_active(GTK_COMBO_BOX(combo),dtls_id);
					linphone_gtk_set_media_encryption_mandatory_sensitive(pb,TRUE);
				}
			break;
			case LinphoneMediaEncryptionZRTP:
				if (zrtp_id!=-1) {
					gtk_combo_box_set_active(GTK_COMBO_BOX(combo),zrtp_id);
					linphone_gtk_set_media_encryption_mandatory_sensitive(pb,TRUE);
				}
			break;
		}
		g_signal_connect(G_OBJECT(combo),"changed",(GCallback)linphone_gtk_media_encryption_changed,NULL);
	}
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(pb,"media_encryption_mandatory_checkbox")),
				     linphone_core_is_media_encryption_mandatory(lc));

	gtk_combo_box_set_active(GTK_COMBO_BOX(linphone_gtk_get_widget(pb,"chat_lime_combo")), linphone_core_lime_enabled(lc));
	gtk_widget_set_sensitive(linphone_gtk_get_widget(pb,"chat_lime_combo"), linphone_core_lime_available(lc));

	g_object_unref(G_OBJECT(model));
}

void linphone_gtk_parameters_destroyed(GtkWidget *pb){
	GtkWidget *mw=linphone_gtk_get_main_window();
	g_object_set_data(G_OBJECT(mw),"parameters",NULL);
}

void linphone_gtk_fill_soundcards(GtkWidget *pb){
	LinphoneCore *lc=linphone_gtk_get_core();
	const char **sound_devices=linphone_core_get_sound_devices(lc);
	linphone_gtk_fill_combo_box(linphone_gtk_get_widget(pb,"playback_device"), sound_devices,
					linphone_core_get_playback_device(lc),CAP_PLAYBACK);
	linphone_gtk_fill_combo_box(linphone_gtk_get_widget(pb,"ring_device"), sound_devices,
					linphone_core_get_ringer_device(lc),CAP_PLAYBACK);
	linphone_gtk_fill_combo_box(linphone_gtk_get_widget(pb,"capture_device"), sound_devices,
					linphone_core_get_capture_device(lc), CAP_CAPTURE);
}

void linphone_gtk_fill_webcams(GtkWidget *pb){
	LinphoneCore *lc=linphone_gtk_get_core();
	linphone_gtk_fill_combo_box(linphone_gtk_get_widget(pb,"webcams"),linphone_core_get_video_devices(lc),
					linphone_core_get_video_device(lc),CAP_IGNORE);
}

void linphone_gtk_fill_video_renderers(GtkWidget *pb){
#ifdef VIDEO_ENABLED /* video_stream_get_default_video_renderer requires video enabled */
	LinphoneCore *lc=linphone_gtk_get_core();
	MSFactory *factory = linphone_core_get_ms_factory(lc);
	GtkWidget *combo=linphone_gtk_get_widget(pb,"renderers");
	bctbx_list_t *l=ms_factory_lookup_filter_by_interface(factory, MSFilterVideoDisplayInterface);
	bctbx_list_t *elem;
	int i;
	int active=-1;
	const char *current_renderer=linphone_core_get_video_display_filter(lc);
	GtkListStore *store;
	GtkCellRenderer *renderer=gtk_cell_renderer_text_new();
	GtkTreeModel *model=GTK_TREE_MODEL(store=gtk_list_store_new(2,G_TYPE_STRING,G_TYPE_STRING));

	if (current_renderer==NULL) current_renderer=video_stream_get_default_video_renderer();

	gtk_combo_box_set_model(GTK_COMBO_BOX(combo),model);
	gtk_cell_layout_clear(GTK_CELL_LAYOUT(combo));
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo),renderer,TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo),renderer,"text",1,NULL);

	for(i=0,elem=l;elem!=NULL && i<4 ;elem=elem->next){
		MSFilterDesc *desc=(MSFilterDesc *)elem->data;
		GtkTreeIter iter;

		/* do not offer the user to select combo 'decoding/rendering' filter */
		if (desc->enc_fmt != NULL)
			continue;

		gtk_list_store_append(store,&iter);
		gtk_list_store_set(store,&iter,0,desc->name,1,desc->text,-1);
		if (current_renderer && strcmp(current_renderer,desc->name)==0)
			active=i;

		i++;
	}
	bctbx_list_free(l);
	if (active!=-1) gtk_combo_box_set_active(GTK_COMBO_BOX(combo),active);
#endif
}

void linphone_gtk_fill_video_presets(GtkWidget *pb) {
	GtkComboBox *combo = GTK_COMBO_BOX(linphone_gtk_get_widget(pb, "video_preset"));
	const char *preset = linphone_core_get_video_preset(linphone_gtk_get_core());
	if (preset == NULL) {
		gtk_combo_box_set_active(combo, -1);
		gtk_combo_box_set_active(combo, 0);
	} else {
		gboolean valid;
		GtkTreeIter iter;
		gchar *str_data;
		GtkTreeModel *model = gtk_combo_box_get_model(combo);
		valid = gtk_tree_model_get_iter_first(model, &iter);
		while (valid) {
			gtk_tree_model_get(model, &iter, 0, &str_data, -1);
			if (g_strcmp0(preset, str_data) == 0) {
				gtk_combo_box_set_active_iter(combo, &iter);
				break;
			}
			valid = gtk_tree_model_iter_next(model, &iter);
		}
	}
}

typedef struct {
	guint timeout_id;
	LCSipTransports tp;
}PortConfigCtx;

static void port_config_free(PortConfigCtx *ctx){
	g_free(ctx);
}

static gboolean apply_transports(PortConfigCtx *ctx){
	GtkWidget *mw=linphone_gtk_get_main_window();
	LCSipTransports tp;
	LinphoneCore *lc=linphone_gtk_get_core();
	linphone_core_get_sip_transports(lc,&tp);
	tp.udp_port=ctx->tp.udp_port;
	tp.tcp_port=ctx->tp.tcp_port;
	linphone_core_set_sip_transports(lc,&tp);
	g_object_set_data(G_OBJECT(mw),"port_config",NULL);
	return FALSE;
}

static PortConfigCtx* get_port_config(void) {
	GtkWidget *mw=linphone_gtk_get_main_window();
	PortConfigCtx *cfg=(PortConfigCtx*)g_object_get_data(G_OBJECT(mw),"port_config");
	if (cfg==NULL){
		cfg=g_new0(PortConfigCtx,1);
		g_object_set_data_full(G_OBJECT(mw),"port_config",cfg,(GDestroyNotify)port_config_free);
	}
	if (cfg->timeout_id!=0) {
		g_source_remove(cfg->timeout_id);
	}
	cfg->timeout_id=g_timeout_add_seconds(2,(GSourceFunc)apply_transports,cfg);
	return cfg;
}

static void transport_changed(GtkWidget *parameters){
	PortConfigCtx *cfg = get_port_config();

	GtkWidget *udp_random_port=linphone_gtk_get_widget(parameters,"random_udp_port");
	GtkWidget *tcp_random_port=linphone_gtk_get_widget(parameters,"random_tcp_port");

	GtkWidget *sip_udp_port=linphone_gtk_get_widget(parameters,"sip_udp_port");
	GtkWidget *sip_tcp_port=linphone_gtk_get_widget(parameters,"sip_tcp_port");

	gboolean udp_is_disabled=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(parameters,"disabled_udp_port")));
	gboolean tcp_is_disabled=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(parameters,"disabled_tcp_port")));

	gboolean udp_is_random=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(udp_random_port));
	gboolean tcp_is_random=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tcp_random_port));

	int udp_port = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(sip_udp_port));
	int tcp_port = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(sip_tcp_port));


	gtk_widget_set_sensitive(udp_random_port, !udp_is_disabled);
	gtk_widget_set_sensitive(tcp_random_port, !tcp_is_disabled);
	gtk_widget_set_sensitive(sip_udp_port, !udp_is_disabled && !udp_is_random);
	gtk_widget_set_sensitive(sip_tcp_port, !tcp_is_disabled && !tcp_is_random);

	cfg->tp.udp_port=udp_is_disabled?0:udp_is_random?-1:udp_port;
	cfg->tp.tcp_port=tcp_is_disabled?0:tcp_is_random?-1:tcp_port;
}

void linphone_gtk_disabled_udp_port_toggle(GtkCheckButton *button){
	GtkWidget *parameters = gtk_widget_get_toplevel((GtkWidget*)button);
	transport_changed(parameters);
}

void linphone_gtk_random_udp_port_toggle(GtkCheckButton *button){
	GtkWidget *parameters = gtk_widget_get_toplevel((GtkWidget*)button);
	transport_changed(parameters);
}

void linphone_gtk_udp_port_value_changed(GtkSpinButton *button){
	GtkWidget *parameters = gtk_widget_get_toplevel((GtkWidget*)button);
	transport_changed(parameters);
}

void linphone_gtk_disabled_tcp_port_toggle(GtkCheckButton *button){
	GtkWidget *parameters = gtk_widget_get_toplevel((GtkWidget*)button);
	transport_changed(parameters);
}

void linphone_gtk_random_tcp_port_toggle(GtkCheckButton *button){
	GtkWidget *parameters = gtk_widget_get_toplevel((GtkWidget*)button);
	transport_changed(parameters);
}

void linphone_gtk_tcp_port_value_changed(GtkSpinButton *button){
	GtkWidget *parameters = gtk_widget_get_toplevel((GtkWidget*)button);
	transport_changed(parameters);
}

void linphone_gtk_show_parameters(void){
	GtkWidget *mw=linphone_gtk_get_main_window();
	GtkWidget *pb=(GtkWidget*)g_object_get_data(G_OBJECT(mw),"parameters");
	LinphoneCore *lc=linphone_gtk_get_core();
	const char *tmp;
	LinphoneAddress *contact;
	LinphoneFirewallPolicy pol;
	GtkWidget *audio_codec_list;
	GtkWidget *video_codec_list;
	int mtu;
	int ui_advanced;
	LCSipTransports tr;
	int min_port = 0, max_port = 0;

	if (pb==NULL) {
		pb=linphone_gtk_create_window("parameters", linphone_gtk_get_main_window());
		g_object_set_data(G_OBJECT(mw),"parameters",pb);
	}else {
		gtk_widget_show(pb);
		return;
	}
	audio_codec_list=linphone_gtk_get_widget(pb,"audio_codec_list");
	video_codec_list=linphone_gtk_get_widget(pb,"video_codec_list");

	/* NETWORK CONFIG */
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(pb,"ipv6_enabled")),
				linphone_core_ipv6_enabled(lc));
	linphone_core_get_sip_transports(lc,&tr);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(pb,"disabled_udp_port")), tr.udp_port==0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(pb,"disabled_tcp_port")), tr.tcp_port==0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(pb,"random_udp_port")), tr.udp_port==-1);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(pb,"random_tcp_port")), tr.tcp_port==-1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(linphone_gtk_get_widget(pb,"sip_udp_port")), tr.udp_port);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(linphone_gtk_get_widget(pb,"sip_tcp_port")), tr.tcp_port);

	linphone_core_get_audio_port_range(lc, &min_port, &max_port);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(linphone_gtk_get_widget(pb, "audio_min_rtp_port")), min_port);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(linphone_gtk_get_widget(pb, "audio_max_rtp_port")), max_port);
	if (min_port == max_port) {
		gtk_widget_set_sensitive(GTK_WIDGET(linphone_gtk_get_widget(pb, "audio_max_rtp_port")), FALSE);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(pb, "fixed_audio_port")), TRUE);
	}
	linphone_core_get_video_port_range(lc, &min_port, &max_port);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(linphone_gtk_get_widget(pb, "video_min_rtp_port")), min_port);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(linphone_gtk_get_widget(pb, "video_max_rtp_port")), max_port);
	if (min_port == max_port) {
		gtk_widget_set_sensitive(GTK_WIDGET(linphone_gtk_get_widget(pb, "video_max_rtp_port")), FALSE);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(pb, "fixed_video_port")), TRUE);
	}

	linphone_gtk_show_media_encryption(pb);

	tmp=linphone_core_get_nat_address(lc);
	if (tmp) gtk_entry_set_text(GTK_ENTRY(linphone_gtk_get_widget(pb,"nat_address")),tmp);
	tmp=linphone_core_get_stun_server(lc);
	if (tmp) gtk_entry_set_text(GTK_ENTRY(linphone_gtk_get_widget(pb,"stun_server")),tmp);
	pol=linphone_core_get_firewall_policy(lc);
	switch(pol){
		case LinphonePolicyNoFirewall:
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(pb,"no_nat")),TRUE);
		break;
		case LinphonePolicyUseNatAddress:
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(pb,"use_nat_address")),TRUE);
		break;
		case LinphonePolicyUseStun:
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(pb,"use_stun")),TRUE);
		break;
		case LinphonePolicyUseIce:
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(pb,"use_ice")),TRUE);
		break;
		case LinphonePolicyUseUpnp:
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(pb,"use_upnp")),TRUE);
		break;
	}
	if(!linphone_core_upnp_available()) {
		gtk_widget_hide(linphone_gtk_get_widget(pb,"use_upnp"));
	}

	mtu=linphone_core_get_mtu(lc);
	if (mtu<=0){
		gtk_widget_set_sensitive(linphone_gtk_get_widget(pb,"mtu"),FALSE);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(linphone_gtk_get_widget(pb,"mtu")),1500);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(pb,"mtu_set")),FALSE);
	}else{
		gtk_widget_set_sensitive(linphone_gtk_get_widget(pb,"mtu"),TRUE);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(linphone_gtk_get_widget(pb,"mtu")),mtu);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(pb,"mtu_set")),TRUE);
	}
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(pb,"dtmf_sipinfo")),
					linphone_core_get_use_info_for_dtmf(lc));
	/* MUTIMEDIA CONFIG */
	linphone_gtk_fill_soundcards(pb);
	linphone_gtk_fill_webcams(pb);
	linphone_gtk_fill_video_renderers(pb);
	linphone_gtk_fill_video_presets(pb);
	linphone_gtk_fill_video_sizes(linphone_gtk_get_widget(pb,"video_size"));
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(linphone_gtk_get_widget(pb,"video_framerate")),
				linphone_core_get_preferred_framerate(lc));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(pb,"echo_cancelation")),
					linphone_core_echo_cancellation_enabled(lc));

	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(linphone_gtk_get_widget(pb,"ring_chooser")),
					linphone_core_get_ring(lc));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(pb,"adaptive_rate_control")),
	                         linphone_core_adaptive_rate_control_enabled(lc));
	/* SIP CONFIG */
	contact=linphone_core_get_primary_contact_parsed(lc);
	if (contact){
		if (linphone_address_get_display_name(contact)) {
			const char *dn=linphone_address_get_display_name(contact);
			gtk_entry_set_text(GTK_ENTRY(linphone_gtk_get_widget(pb,"displayname")),dn);
		}
		if (linphone_address_get_username(contact))
			gtk_entry_set_text(GTK_ENTRY(linphone_gtk_get_widget(pb,"username")),linphone_address_get_username(contact));
		linphone_address_unref(contact);
	}
#ifdef BUILD_WIZARD
	gtk_widget_show(linphone_gtk_get_widget(pb,"wizard"));
#else
	gtk_widget_hide(linphone_gtk_get_widget(pb,"wizard"));
#endif
	linphone_gtk_show_sip_accounts(pb);
	/* CODECS CONFIG */
	linphone_gtk_init_codec_list(GTK_TREE_VIEW(audio_codec_list));
	linphone_gtk_init_codec_list(GTK_TREE_VIEW(video_codec_list));
	linphone_gtk_draw_codec_list(GTK_TREE_VIEW(audio_codec_list), 0);
	linphone_gtk_draw_codec_list(GTK_TREE_VIEW(video_codec_list), 1);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(linphone_gtk_get_widget(pb,"download_bw")),
				linphone_core_get_download_bandwidth(lc));
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(linphone_gtk_get_widget(pb,"upload_bw")),
				linphone_core_get_upload_bandwidth(lc));

	/* CALL PARAMS CONFIG */
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(pb, "auto_answer_checkbox")), linphone_gtk_auto_answer_enabled());
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(linphone_gtk_get_widget(pb, "auto_answer_delay_spinbutton")), linphone_gtk_get_ui_config_int("auto_answer_delay", 2000));

	/* UI CONFIG */
	linphone_gtk_fill_langs(pb);
	ui_advanced = linphone_gtk_get_ui_config_int("advanced_ui", 1);
	linphone_gtk_set_ui_config_int("advanced_ui", ui_advanced);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(pb,"ui_level")),
				ui_advanced);
	linphone_gtk_ui_level_adapt(pb);

	if (linphone_core_tunnel_available()){
		gtk_widget_set_visible(GTK_WIDGET(linphone_gtk_get_widget(pb,"tunnel_edit_button")), TRUE);
		gtk_widget_set_visible(GTK_WIDGET(linphone_gtk_get_widget(pb,"tunnel_label")), TRUE);
	}

	/* LDAP CONFIG */
	if( linphone_gtk_is_ldap_supported() ) { // if LDAP provider is available
		linphone_gtk_ldap_display(pb);
	} else {
		// hide the LDAP tab
		GtkNotebook* notebook = GTK_NOTEBOOK(linphone_gtk_get_widget(pb, "notebook1"));
		gtk_notebook_remove_page(notebook,5);
	}

	gtk_widget_show(pb);
}


void linphone_gtk_fixed_audio_port_toggle(void) {
	GtkWidget *mw = linphone_gtk_get_main_window();
	GtkWidget *pb = (GtkWidget *) g_object_get_data(G_OBJECT(mw), "parameters");
	gboolean fixed = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(pb, "fixed_audio_port")));
	gint min_port = (gint)gtk_spin_button_get_value(GTK_SPIN_BUTTON(linphone_gtk_get_widget(pb, "audio_min_rtp_port")));
	gint max_port = (gint)gtk_spin_button_get_value(GTK_SPIN_BUTTON(linphone_gtk_get_widget(pb, "audio_max_rtp_port")));
	gtk_widget_set_sensitive(GTK_WIDGET(linphone_gtk_get_widget(pb, "audio_max_rtp_port")), !fixed);
	if (fixed) {
		linphone_core_set_audio_port(linphone_gtk_get_core(), min_port);
	} else {
		linphone_core_set_audio_port_range(linphone_gtk_get_core(), min_port, max_port);
	}
}


void linphone_gtk_fixed_video_port_toggle(void) {
	GtkWidget *mw = linphone_gtk_get_main_window();
	GtkWidget *pb = (GtkWidget *) g_object_get_data(G_OBJECT(mw), "parameters");
	gboolean fixed = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(pb, "fixed_video_port")));
	gint min_port = (gint)gtk_spin_button_get_value(GTK_SPIN_BUTTON(linphone_gtk_get_widget(pb, "video_min_rtp_port")));
	gint max_port = (gint)gtk_spin_button_get_value(GTK_SPIN_BUTTON(linphone_gtk_get_widget(pb, "video_max_rtp_port")));
	gtk_widget_set_sensitive(GTK_WIDGET(linphone_gtk_get_widget(pb, "video_max_rtp_port")), !fixed);
	if (fixed) {
		linphone_core_set_video_port(linphone_gtk_get_core(), min_port);
	} else {
		linphone_core_set_video_port_range(linphone_gtk_get_core(), min_port, max_port);
	}
}


void linphone_gtk_edit_tunnel_closed(GtkWidget *button){
	GtkWidget *pb=gtk_widget_get_toplevel(button);
	gtk_widget_destroy(pb);
}

void linphone_gtk_edit_tunnel(GtkButton *button){
	GtkWidget *w=linphone_gtk_create_window("tunnel_config", gtk_widget_get_toplevel(GTK_WIDGET(button)));
	LinphoneCore *lc=linphone_gtk_get_core();
	LinphoneTunnel *tunnel=linphone_core_get_tunnel(lc);
	const bctbx_list_t *configs;
	const char *host = NULL;
	int port=0;

	if (!tunnel) return;

	configs = linphone_tunnel_get_servers(tunnel);
	if(configs != NULL) {
		LinphoneTunnelConfig *ltc = (LinphoneTunnelConfig *)configs->data;
		host = linphone_tunnel_config_get_host(ltc);
		port = linphone_tunnel_config_get_port(ltc);
	}

	gtk_entry_set_text(GTK_ENTRY(linphone_gtk_get_widget(w,"host")),host);
	if (port==0) port=443;
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(linphone_gtk_get_widget(w,"port")), port);

	switch(linphone_tunnel_get_mode(tunnel)){
		case LinphoneTunnelModeDisable:
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(w,"radio_disable")),1);
		break;
		case LinphoneTunnelModeEnable:
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(w,"radio_enable")),1);
		break;
		case LinphoneTunnelModeAuto:
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(w,"tunnel_autodetect")),1);
		break;
	}
	{
		const char *proxy=NULL,*username=NULL,*password=NULL;
		port=0;
		linphone_tunnel_get_http_proxy(tunnel,&proxy,&port,&username,&password);
		if (proxy)
			gtk_entry_set_text(GTK_ENTRY(linphone_gtk_get_widget(w,"http_host")),proxy);
		if (port>0)
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(linphone_gtk_get_widget(w,"http_port")), port);
		if (username)
			gtk_entry_set_text(GTK_ENTRY(linphone_gtk_get_widget(w,"username")),username);
		if (password)
			gtk_entry_set_text(GTK_ENTRY(linphone_gtk_get_widget(w,"password")),password);
	}

	g_object_weak_ref(G_OBJECT(w),(GWeakNotify)linphone_gtk_edit_tunnel_closed,w);
	gtk_widget_show(w);
}

void linphone_gtk_tunnel_ok(GtkButton *button){
	GtkWidget *w=gtk_widget_get_toplevel(GTK_WIDGET(button));
	LinphoneCore *lc=linphone_gtk_get_core();
	LinphoneTunnel *tunnel=linphone_core_get_tunnel(lc);
	LinphoneTunnelConfig *config=linphone_tunnel_config_new();

	gint port = (gint)gtk_spin_button_get_value(GTK_SPIN_BUTTON(linphone_gtk_get_widget(w,"port")));
	gboolean enabled=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(w,"radio_enable")));
	gboolean autodetect=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(linphone_gtk_get_widget(w,"tunnel_autodetect")));
	const char *host=gtk_entry_get_text(GTK_ENTRY(linphone_gtk_get_widget(w,"host")));
	const char *http_host=gtk_entry_get_text(GTK_ENTRY(linphone_gtk_get_widget(w,"http_host")));
	gint http_port = (gint)gtk_spin_button_get_value(GTK_SPIN_BUTTON(linphone_gtk_get_widget(w,"http_port")));
	const char *username=gtk_entry_get_text(GTK_ENTRY(linphone_gtk_get_widget(w,"username")));
	const char *password=gtk_entry_get_text(GTK_ENTRY(linphone_gtk_get_widget(w,"password")));
	LinphoneTunnelMode mode = LinphoneTunnelModeDisable;

	if (tunnel==NULL) return;
	if (host && *host=='\0') host=NULL;
	if (http_port==0) http_port=8080;
	linphone_tunnel_clean_servers(tunnel);
	linphone_tunnel_config_set_host(config, host);
	linphone_tunnel_config_set_port(config, port);
	linphone_tunnel_add_server(tunnel, config);

	if (enabled){
		mode = LinphoneTunnelModeEnable;
	}else if (autodetect){
		mode = LinphoneTunnelModeAuto;
	}
	linphone_tunnel_set_mode(tunnel, mode);
	linphone_tunnel_set_http_proxy(tunnel,http_host,http_port,username,password);

	gtk_widget_destroy(w);
}


void linphone_gtk_tunnel_cancel(GtkButton *button){

}

static void show_dscp(GtkWidget *entry, int val){
	char tmp[20];
	snprintf(tmp,sizeof(tmp),"0x%x",val);
	gtk_entry_set_text(GTK_ENTRY(entry),tmp);

}

static int read_dscp(GtkWidget *entry){
	const char *val=gtk_entry_get_text(GTK_ENTRY(entry));
	const char *begin;
	int ret=0;
	if (val==NULL || val[0]=='\0') return 0;
	/*skip potential 0x*/
	begin=strstr(val,"0x");
	if (begin) begin+=2;
	else begin=val;
	if (sscanf(begin,"%x",&ret)==1)
		return ret;
	return -1;
}

void linphone_gtk_dscp_edit(void){
	LinphoneCore *lc=linphone_gtk_get_core();
	GtkWidget *widget=linphone_gtk_create_window("dscp_settings", linphone_gtk_get_main_window());
	show_dscp(linphone_gtk_get_widget(widget,"sip_dscp"),
		  linphone_core_get_sip_dscp(lc));
	show_dscp(linphone_gtk_get_widget(widget,"audio_dscp"),
		  linphone_core_get_audio_dscp(lc));
	show_dscp(linphone_gtk_get_widget(widget,"video_dscp"),
		  linphone_core_get_video_dscp(lc));
	gtk_widget_show(widget);
}

void linphone_gtk_dscp_edit_response(GtkWidget *dialog, guint response_id){
	LinphoneCore *lc=linphone_gtk_get_core();
	switch(response_id){
		case GTK_RESPONSE_OK:
			linphone_core_set_sip_dscp(lc,
				read_dscp(linphone_gtk_get_widget(dialog,"sip_dscp")));
			linphone_core_set_audio_dscp(lc,
				read_dscp(linphone_gtk_get_widget(dialog,"audio_dscp")));
			linphone_core_set_video_dscp(lc,
				read_dscp(linphone_gtk_get_widget(dialog,"video_dscp")));

		break;
		default:
		break;
	}
	gtk_widget_destroy(dialog);
}

void linphone_gtk_enable_auto_answer(GtkToggleButton *checkbox, gpointer user_data) {
	gboolean auto_answer_enabled = gtk_toggle_button_get_active(checkbox);
	linphone_gtk_set_ui_config_int("auto_answer", auto_answer_enabled ? 1 : 0);
}

gboolean linphone_gtk_auto_answer_enabled(void) {
	return (gboolean)linphone_gtk_get_ui_config_int("auto_answer", 0);
}

void linphone_gtk_auto_answer_delay_changed(GtkSpinButton *spinbutton, gpointer user_data) {
	int delay = (int)gtk_spin_button_get_value(spinbutton);
	linphone_gtk_set_ui_config_int("auto_answer_delay", delay);
}

void linphone_gtk_notebook_current_page_changed	(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer user_data) {
#ifndef HAVE_LIBUDEV_H
	if (page_num == 1) {
		// Multimedia settings - we reload audio and video devices to detect
		// hot-plugged devices
		g_message("Opened multimedia page... reloading audio and video devices!");
		linphone_gtk_reload_sound_devices();
		linphone_gtk_reload_video_devices();
	}
#endif
}

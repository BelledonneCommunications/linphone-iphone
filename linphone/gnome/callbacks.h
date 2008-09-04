#include <gnome.h>


void
on_app1_destroy                        (GtkObject       *object,
                                        gpointer         user_data);

void
on_adresse_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_parametres1_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_fermer1_activate                    (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_exit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_about1_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_user_manual1_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_greenbutton_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_redbutton_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_showmore_toggled                    (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

gboolean
on_play_vol_button_release_event       (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_rec_vol_button_release_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

gboolean
on_ring_vol_button_release_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_reachable                           (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_busy                                (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_minutesaway_changed                 (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_away                                (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_do_not_disturb                      (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_moved_tmply                         (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_alt_serv                            (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_contact_field_changed               (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_presence_validate_clicked           (GtkButton       *button,
                                        gpointer         user_data);

void
on_dtmf_entry_changed                  (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_dtmf_3_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

void
on_dmtf_2_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

void
on_dtmf_1_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

void
on_dtmf_4_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

void
on_dtmf_5_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

void
on_dtmf_6_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

void
on_dtmf_7_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

void
on_dtmf_8_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

void
on_dtmf_9_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

void
on_dtmf_star_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_dtmf_0_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

void
on_dtmf_pound_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_propertybox1_apply                  (GnomePropertyBox *propertybox,
                                        gint             page_num,
                                        gpointer         user_data);

gboolean
on_prop1_close                         (GnomeDialog     *gnomedialog,
                                        gpointer         user_data);

void
on_prop1_help                          (GnomePropertyBox *propertybox,
                                        gint             page_num,
                                        gpointer         user_data);

gboolean
on_hscale1_button_release_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_audioport_changed                   (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_sipport_changed                     (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_user_name_changed                   (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_domain_name_changed                 (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_registrar_checked_toggled           (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_rsvp_checked_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data);


void
on_redirect_button_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_proxy_button_toggled                (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_obproxy_button_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_registrar_addr_changed              (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_reg_passwd_changed                  (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_address_of_record_changed           (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_aucodec_up_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_aucodec_down_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_aucodec_enable_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_aucodec_disable_clicked             (GtkButton       *button,
                                        gpointer         user_data);

void
on_sounddriver_changed                 (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_source_changed                      (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_autokill_button1_toggled            (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_address_book_show                   (GtkWidget       *widget,
                                        gpointer         user_data);

void
on_add_address_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

void
on_remove_address_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_select_address_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_modify_address_clicked              (GtkButton       *button,
                                        gpointer         user_data);

void
on_alt_href_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_alt_href_realize                    (GtkWidget       *widget,
                                        gpointer         user_data);


void
on_exit1_activate                      (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
address_book_close                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_card_changed                        (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_audio_jittcomp_value_changed        (GtkRange        *range,
                                        gpointer         user_data);

void
on_enable_nat_toggled                  (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_nat_address_changed                 (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_display_ab_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_ringfileentry_changed               (GtkEditable     *editable,
                                        gpointer         user_data);

void
on_ringpreview_clicked                 (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_property_box_closed                 (GnomeDialog     *gnomedialog,
                                        gpointer         user_data);

void
on_address_book_close                  (GtkObject       *object,
                                        gpointer         user_data);

#ifndef VERSION
# define VERSION LINPHONE_VERSION
#endif

void
on_addfriend_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_removefriend_clicked                (GtkButton       *button,
                                        gpointer         user_data);

void
on_add_adbk_clicked                    (GtkButton       *button,
                                        gpointer         user_data);

void
on_addfriend_dialog_response           (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data);

void
on_friendlist_row_activated            (GtkTreeView     *treeview,
                                        GtkTreePath     *path,
                                        GtkTreeViewColumn *column,
                                        gpointer         user_data);
void
on_useRSVP_toggled                     (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_useRPC_toggled                      (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

#ifdef VINCENT_MAURY_RSVP
void 
dialog_click			       (GtkDialog *dialog,
					gint arg1,
					gpointer user_data);
#endif

void
on_proxy_config_box_response           (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data);

void
on_removeproxy_button_clicked          (GtkButton       *button,
                                        gpointer         user_data);

void
on_addproxy_button_clicked             (GtkButton       *button,
                                        gpointer         user_data);

void
on_editproxy_button_clicked            (GtkButton       *button,
                                        gpointer         user_data);


void
on_contact_box_response                (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data);

void
on_inc_subscr_dialog_response          (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data);

void
on_ob_proxy_changed                    (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_authentication_dialog_response      (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data);

void
on_clear_auth_info_clicked             (GtkButton       *button,
                                        gpointer         user_data);

void
on_use_sipinfo_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_guess_hostname_toggled              (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_call_history_activate               (GtkMenuItem     *menuitem,
                                        gpointer         user_data);

void
on_call_logs_response                  (GtkDialog       *dialog,
                                        gint             response_id,
                                        gpointer         user_data);

void
on_call_logs_destroy                   (GtkObject       *object,
                                        gpointer         user_data);

void
on_enable_ipv6_toggled                 (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_play_card_changed                   (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_capt_card_changed                   (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_ring_card_changed                   (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_callbutton_clicked                  (GtkButton       *button,
                                        gpointer         user_data);

void
on_chatbox_clicked                     (GtkButton       *button,
                                        gpointer         user_data);

void
on_chatentry_activate                  (GtkEntry        *entry,
                                        gpointer         user_data);

void
on_hangup_clicked                      (GtkButton       *button,
                                        gpointer         user_data);

void
on_chat_clicked                        (GtkButton       *button,
                                        gpointer         user_data);

void
on_chatroom_destroy                    (GtkObject       *object,
                                        gpointer         user_data);

void
on_ring_card_changed                   (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_addressentry_editing_done           (GtkCellEditable *celleditable,
                                        gpointer         user_data);

void
on_addressentry_destroy                (GtkObject       *object,
                                        gpointer         user_data);

gboolean
on_addressentry_key_pressed            (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

void
on_addressentry_changed                (GtkComboBox     *combobox,
                                        gpointer         user_data);

void
on_addressentry_activate               (GtkEntry        *entry,
                                        gpointer         user_data);

void
on_addressentry_destroy                (GtkObject       *object,
                                        gpointer         user_data);

void
on_download_bw_value_changed           (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);

void
on_download_bw_editing_done            (GtkCellEditable *celleditable,
                                        gpointer         user_data);

void
on_download_bw_changed                 (GtkEditable     *editable,
                                        gpointer         user_data);

gboolean
on_upload_bw_output                    (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);

void
on_upload_bw_change_value              (GtkSpinButton   *spinbutton,
                                        GtkScrollType    scroll,
                                        gpointer         user_data);

void
on_upload_bw_value_changed             (GtkSpinButton   *spinbutton,
                                        gpointer         user_data);

gboolean
on_upload_bw_leave_notify_event        (GtkWidget       *widget,
                                        GdkEventCrossing *event,
                                        gpointer         user_data);

void
on_video_enabled_toggled               (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_echocancelation_toggled             (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_no_nat_toggled                      (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_use_stun_toggled                    (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_static_nat_toggled                  (GtkToggleButton *togglebutton,
                                        gpointer         user_data);

void
on_stun_server_changed                 (GtkEditable     *editable,
                                        gpointer         user_data);

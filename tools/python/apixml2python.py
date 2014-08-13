#!/usr/bin/python

# Copyright (C) 2014 Belledonne Communications SARL
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

import argparse
import os
import pystache
import sys
import xml.etree.ElementTree as ET

sys.path.append(os.path.realpath(__file__))
from apixml2python.linphone import LinphoneModule


blacklisted_classes = [
	'LinphoneTunnel',
	'LinphoneTunnelConfig'
]
blacklisted_events = [
	'LinphoneCoreInfoReceivedCb',	# missing LinphoneInfoMessage
	'LinphoneCoreNotifyReceivedCb',	# missing LinphoneContent
	'LinphoneCoreFileTransferProgressIndicationCb',	# missing LinphoneContent
	'LinphoneCoreFileTransferRecvCb',	# missing LinphoneContent
	'LinphoneCoreFileTransferSendCb'	# missing LinphoneContent
]
blacklisted_functions = [
	'linphone_call_get_user_pointer',	# rename to linphone_call_get_user_data
	'linphone_call_set_user_pointer',	# rename to linphone_call_set_user_data
	'linphone_call_log_get_local_stats',	# missing rtp_stats_t
	'linphone_call_log_get_remote_stats',	# missing rtp_stats_t
	'linphone_call_log_get_start_date',	# missing time_t
	'linphone_call_log_get_user_pointer',	# rename to linphone_call_log_get_user_data
	'linphone_call_log_set_user_pointer',	# rename to linphone_call_log_set_user_data
	'linphone_call_params_get_privacy',	# missing LinphonePrivacyMask
	'linphone_call_params_get_used_audio_codec',	# missing PayloadType
	'linphone_call_params_get_used_video_codec',	# missing PayloadType
	'linphone_call_params_set_privacy',	# missing LinphonePrivacyMask
	'linphone_chat_message_get_file_transfer_information',	# missing LinphoneContent
	'linphone_chat_message_get_time',	# missing time_t
	'linphone_chat_message_start_file_download',	# to be handwritten because of callback
	'linphone_chat_message_state_to_string',	# There is no use to wrap this function
	'linphone_chat_room_create_file_transfer_message',	# missing LinphoneContent
	'linphone_chat_room_create_message_2',	# missing time_t
	'linphone_core_can_we_add_call',	# private function
	'linphone_core_enable_payload_type',	# missing PayloadType
	'linphone_core_find_payload_type',	# missing PayloadType
	'linphone_core_get_audio_codecs',	# missing PayloadType and MSList
	'linphone_core_get_auth_info_list',	# missing MSList
	'linphone_core_get_call_logs',	# missing MSList
	'linphone_core_get_calls',	# missing MSList
	'linphone_core_get_chat_rooms',	# missing MSList
	'linphone_core_get_default_proxy',	# to be handwritten because of double pointer indirection
	'linphone_core_get_payload_type_bitrate',	# missing PayloadType
	'linphone_core_get_friend_list',	# missing MSList
	'linphone_core_get_proxy_config_list',	# missing MSList
	'linphone_core_get_sip_transports',	# missing LCSipTransports
	'linphone_core_get_sip_transports_used',	# missing LCSipTransports
	'linphone_core_get_supported_video_sizes',	# missing MSVideoSizeDef
	'linphone_core_get_video_codecs',	# missing PayloadType and MSList
	'linphone_core_get_video_devices',	# returns a list of strings
	'linphone_core_get_video_policy',	# missing LinphoneVideoPolicy
	'linphone_core_payload_type_enabled',	# missing PayloadType
	'linphone_core_payload_type_is_vbr',	# missing PayloadType
	'linphone_core_publish',	# missing LinphoneContent
	'linphone_core_serialize_logs',	# There is no use to wrap this function
	'linphone_core_set_log_file',	# There is no use to wrap this function
	'linphone_core_set_log_handler',	# Hand-written but put directly in the linphone module
	'linphone_core_set_log_level',	# There is no use to wrap this function
	'linphone_core_set_payload_type_bitrate',	# missing PayloadType
	'linphone_core_set_video_policy',	# missing LinphoneVideoPolicy
	'linphone_core_set_audio_codecs',	# missing PayloadType and MSList
	'linphone_core_set_sip_transports',	# missing LCSipTransports
	'linphone_core_subscribe',	# missing LinphoneContent
	'linphone_event_notify',	# missing LinphoneContent
	'linphone_event_send_publish',	# missing LinphoneContent
	'linphone_event_send_subscribe',	# missing LinphoneContent
	'linphone_event_update_publish',	# missing LinphoneContent
	'linphone_event_update_subscribe',	# missing LinphoneContent
	'linphone_presence_model_get_timestamp',	# missing time_t
	'linphone_proxy_config_get_privacy',	# missing LinphonePrivacyMask
	'linphone_proxy_config_normalize_number',	# to be handwritten because of result via arguments
	'linphone_proxy_config_set_file_transfer_server',	# defined but not implemented in linphone core
	'linphone_proxy_config_set_privacy',	# missing LinphonePrivacyMask
	'linphone_tunnel_get_http_proxy',	# to be handwritten because of double pointer indirection
	'lp_config_for_each_entry',	# to be handwritten because of callback
	'lp_config_for_each_section',	# to be handwritten because of callback
	'lp_config_get_range',	# to be handwritten because of result via arguments
	'lp_config_load_dict_to_section',	# missing LinphoneDictionary
	'lp_config_section_to_dict'	# missing LinphoneDictionary
]
hand_written_functions = [
	'linphone_chat_room_send_message2',
	'linphone_core_new',
	'linphone_core_new_with_config'
]

def generate(apixmlfile, outputfile):
	tree = ET.parse(apixmlfile)
	renderer = pystache.Renderer()
	m = LinphoneModule(tree, blacklisted_classes, blacklisted_events, blacklisted_functions, hand_written_functions)
	os.chdir('apixml2python')
	tmpfilename = outputfile.name + '.tmp'
	f = open(tmpfilename, 'w')
	f.write(renderer.render(m))
	f.close()
	f = open(tmpfilename, 'rU')
	for line in f:
		outputfile.write(line)
	f.close()
	os.unlink(tmpfilename)


def main(argv = None):
	if argv is None:
		argv = sys.argv
	argparser = argparse.ArgumentParser(description="Generate a Python wrapper of the Linphone API.")
	argparser.add_argument('-o', '--outputfile', metavar='outputfile', type=argparse.FileType('w'), help="Output C file containing the code of the Python wrapper.")
	argparser.add_argument('apixmlfile', help="XML file of the Linphone API generated by genapixml.py.")
	args = argparser.parse_args()
	if args.outputfile == None:
		args.outputfile = open('linphone.c', 'w')
	generate(args.apixmlfile, args.outputfile)

if __name__ == "__main__":
	sys.exit(main())

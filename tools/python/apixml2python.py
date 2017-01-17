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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

import argparse
import os
import pystache
import sys
import xml.etree.ElementTree as ET

sys.path.append(os.path.realpath(__file__))
from apixml2python.linphone import LinphoneModule, HandWrittenClassMethod, HandWrittenInstanceMethod, HandWrittenDeallocMethod, HandWrittenProperty


blacklisted_classes = [
	'LinphoneTunnel',
	'LinphoneTunnelConfig'
]
blacklisted_events = [
	'LinphoneChatMessageStateChangedCb',	# not respecting naming convention
	'LinphoneCoreAuthInfoRequestedCb',
	'LinphoneCoreInfoReceivedCb',	# missing LinphoneInfoMessage
	'LinphoneCoreNotifyReceivedCb',	# missing LinphoneContent
	'LinphoneCoreFileTransferProgressIndicationCb',	# missing LinphoneContent
	'LinphoneCoreFileTransferRecvCb',	# missing LinphoneContent
	'LinphoneCoreFileTransferSendCb',	# missing LinphoneContent
	'LinphoneCoreTextMessageReceivedCb'	# not respecting naming convention
]
blacklisted_functions = [
	'linphone_call_log_get_local_stats',	# missing rtp_stats_t
	'linphone_call_log_get_remote_stats',	# missing rtp_stats_t
	'linphone_call_params_get_privacy',	# missing LinphonePrivacyMask
	'linphone_call_params_set_privacy',	# missing LinphonePrivacyMask
	'linphone_chat_message_start_file_download',	# callback function in parameter
	'linphone_chat_message_state_to_string',	# There is no use to wrap this function
	'linphone_chat_room_send_chat_message',	# Use linphone_chat_room_send_chat_message_2 instead
	'linphone_chat_room_send_message2',	# callback function in parameter
	'linphone_config_for_each_entry',	# to be handwritten because of callback
	'linphone_config_for_each_section',	# to be handwritten because of callback
	'linphone_config_get_range',	# to be handwritten because of result via arguments
	'linphone_config_load_dict_to_section',	# missing LinphoneDictionary
	'linphone_config_section_to_dict',	# missing LinphoneDictionary
	'linphone_core_add_listener',
	'linphone_core_can_we_add_call',	# private function
	'linphone_core_enable_log_collection',	# need to handle class properties
	'linphone_core_enable_logs',	# unhandled argument type FILE
	'linphone_core_enable_logs_with_cb',	# callback function in parameter
	'linphone_core_get_audio_port_range',	# to be handwritten because of result via arguments
	'linphone_core_get_default_proxy',
	'linphone_core_get_network_simulator_params',	# missing OrtpNetworkSimulatorParams
	'linphone_core_get_supported_video_sizes',	# missing MSVideoSizeDef
	'linphone_core_get_video_policy',	# missing LinphoneVideoPolicy
	'linphone_core_get_video_port_range',	# to be handwritten because of result via arguments
	'linphone_core_new', # replaced by linphone_factory_create_core
	'linphone_core_new_with_config', # replaced by linphone_factory_create_core_with_config
	'linphone_core_remove_listener',
	'linphone_core_serialize_logs',	# There is no use to wrap this function
	'linphone_core_set_dns_servers',
	'linphone_core_set_log_collection_max_file_size',	# need to handle class properties
	'linphone_core_set_log_collection_path',	# need to handle class properties
	'linphone_core_set_log_collection_prefix',	# need to handle class properties
	'linphone_core_set_log_file',	# There is no use to wrap this function
	'linphone_core_set_log_handler',	# Hand-written but put directly in the linphone module
	'linphone_core_set_log_level',	# There is no use to wrap this function
	'linphone_core_set_log_level_mask',	# There is no use to wrap this function
	'linphone_core_set_network_simulator_params',	# missing OrtpNetworkSimulatorParams
	'linphone_core_set_video_policy',	# missing LinphoneVideoPolicy
	'linphone_nat_policy_get_stun_server_addrinfo',
	'linphone_proxy_config_get_privacy',	# missing LinphonePrivacyMask
	'linphone_proxy_config_normalize_number',	# to be handwritten because of result via arguments
	'linphone_proxy_config_set_file_transfer_server',	# defined but not implemented in linphone core
	'linphone_proxy_config_set_privacy',	# missing LinphonePrivacyMask
	'linphone_tunnel_get_http_proxy',	# to be handwritten because of double pointer indirection
	'linphone_vcard_get_belcard', # specific to C++
	'linphone_xml_rpc_request_new_with_args',	# to be handwritten because of va_list
]
hand_written_functions = [
	HandWrittenClassMethod('Buffer', 'new_from_data', 'linphone_buffer_new_from_data', "Create a new LinphoneBuffer object from existing data.\n\n:param data: The initial data to store in the LinphoneBuffer.\n:type data: ByteArray\n:returns: A new LinphoneBuffer object.\n:rtype: linphone.Buffer"),
	HandWrittenProperty('Buffer', 'content', 'linphone_buffer_get_content', 'linphone_buffer_set_content', "[ByteArray] Set the content of the data buffer."),
	HandWrittenProperty('Content', 'buffer', 'linphone_content_get_buffer', 'linphone_content_set_buffer', "[ByteArray] Set the content data buffer."),
	HandWrittenProperty('Call', 'native_video_window_id', 'linphone_call_get_native_video_window_id', 'linphone_call_set_native_video_window_id', "[int] Set the native video window id where the video is to be displayed."),
	HandWrittenProperty('Core', 'native_preview_window_id', 'linphone_core_get_native_preview_window_id', 'linphone_core_set_native_preview_window_id', "[int] Set the native window id where the preview video (local camera) is to be displayed. This has to be used in conjonction with :py:meth:`linphone.Core.use_preview_window` . MacOS, Linux, Windows: if not set or zero the core will create its own window, unless the special id -1 is given."),
	HandWrittenProperty('Core', 'native_video_window_id', 'linphone_core_get_native_video_window_id', 'linphone_core_set_native_video_window_id', "[int] Set the native video window id where the video is to be displayed. For MacOS, Linux, Windows: if not set or LINPHONE_VIDEO_DISPLAY_AUTO the core will create its own window, unless the special id LINPHONE_VIDEO_DISPLAY_NONE is given."),
	HandWrittenProperty('Core', 'sip_transports', 'linphone_core_get_sip_transports', 'linphone_core_set_sip_transports', "[:py:class:`linphone.SipTransports`] Sets the ports to be used for each transport. A zero value port for a given transport means the transport is not used. A value of LC_SIP_TRANSPORT_RANDOM (-1) means the port is to be chosen randomly by the system."),
	HandWrittenProperty('Core', 'sip_transports_used', 'linphone_core_get_sip_transports_used', None, "[:py:class:`linphone.SipTransports`] Retrieves the real port number assigned for each sip transport (udp, tcp, tls). A zero value means that the transport is not activated. If LC_SIP_TRANSPORT_RANDOM was passed to :py:attr:`linphone.Core.sip_transports`, the random port choosed by the system is returned."),
	HandWrittenProperty('Core', 'sound_devices', 'linphone_core_get_sound_devices', None, "[list of string] Get the available sound devices."),
	HandWrittenProperty('Core', 'video_devices', 'linphone_core_get_video_devices', None, "[list of string] Get the available video capture devices."),
	HandWrittenProperty('Config', 'sections_names', 'linphone_config_get_sections_names', None, "[list of string] Get the sections' names in the lp config."),
	HandWrittenInstanceMethod('Factory', 'create_core', 'linphone_factory_create_core', "Instanciate a LinphoneCore object.\n\nThe LinphoneCore object is the primary handle for doing all phone actions. It should be unique within your application.\n\n:param cbs: a LinphoneCoreCbs object holding your application callbacks. A reference will be taken on it until the destruciton of the core or the unregistration with linphone_core_remove_cbs().\n:type cbs: linphone.CoreCbs\n:param config_path: a path to a config file. If it does not exists it will be created. The config file is used to store all settings, call logs, friends, proxies... so that all these settings become persistent over the life of the LinphoneCore object. It is allowed to set a None config file. In that case LinphoneCore will not store any settings.\n:type config_path: string\n:param factory_config_path: a path to a read-only config file that can be used to to store hard-coded preference such as proxy settings or internal preferences. The settings in this factory file always override the one in the normal config file. It is OPTIONAL, use None if unneeded.\n:type factory_config_path: string\n:returns: \n:rtype: linphone.Core"),
	HandWrittenInstanceMethod('Factory', 'create_core_with_config', 'linphone_factory_create_core_with_config', "Instantiates a LinphoneCore object with a given LpConfig.\n\n:param cbs: a LinphoneCoreCbs object holding your application callbacks. A reference will be taken on it until the destruciton of the core or the unregistration with linphone_core_remove_cbs().\n:type cbs: linphone.CoreCbs\n:param config: a pointer to an LpConfig object holding the configuration of the LinphoneCore to be instantiated.\n:type config: linphone.Config\n:returns: \n:rtype: linphone.Core"),
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

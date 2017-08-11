/*
 * imdn.cpp
 * Copyright (C) 2017  Belledonne Communications SARL
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "imdn.h"

#include "logger/logger.h"

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// =============================================================================

const string Imdn::imdnPrefix = "/imdn:imdn";

void Imdn::parse (ChatRoom &cr, const string &text) {
	xmlparsing_context_t *xmlCtx = linphone_xmlparsing_context_new();
	xmlSetGenericErrorFunc(xmlCtx, linphone_xmlparsing_genericxml_error);
	xmlCtx->doc = xmlReadDoc((const unsigned char *)text.c_str(), 0, nullptr, 0);
	if (xmlCtx->doc)
		parse(cr, xmlCtx);
	else
		lWarning() << "Wrongly formatted IMDN XML: " << xmlCtx->errorBuffer;
	linphone_xmlparsing_context_destroy(xmlCtx);
}

void Imdn::parse (ChatRoom &cr, xmlparsing_context_t *xmlCtx) {
	char xpathStr[MAX_XPATH_LENGTH];
	char *messageIdStr = nullptr;
	char *datetimeStr = nullptr;
	if (linphone_create_xml_xpath_context(xmlCtx) < 0)
		return;

	xmlXPathRegisterNs(xmlCtx->xpath_ctx, (const xmlChar *)"imdn", (const xmlChar *)"urn:ietf:params:xml:ns:imdn");
	xmlXPathObjectPtr imdnObject = linphone_get_xml_xpath_object_for_node_list(xmlCtx, imdnPrefix.c_str());
	if (imdnObject) {
		if (imdnObject->nodesetval && (imdnObject->nodesetval->nodeNr >= 1)) {
			snprintf(xpathStr, sizeof(xpathStr), "%s[1]/imdn:message-id", imdnPrefix.c_str());
			messageIdStr = linphone_get_xml_text_content(xmlCtx, xpathStr);
			snprintf(xpathStr, sizeof(xpathStr), "%s[1]/imdn:datetime", imdnPrefix.c_str());
			datetimeStr = linphone_get_xml_text_content(xmlCtx, xpathStr);
		}
		xmlXPathFreeObject(imdnObject);
	}

	if (messageIdStr && datetimeStr) {
		LinphoneChatMessage *cm = cr.findMessageWithDirection(messageIdStr, LinphoneChatMessageOutgoing);
		if (!cm) {
			lWarning() << "Received IMDN for unknown message " << messageIdStr;
		} else {
			LinphoneImNotifPolicy *policy = linphone_core_get_im_notif_policy(cr.getCore());
			snprintf(xpathStr, sizeof(xpathStr), "%s[1]/imdn:delivery-notification/imdn:status", imdnPrefix.c_str());
			xmlXPathObjectPtr deliveryStatusObject = linphone_get_xml_xpath_object_for_node_list(xmlCtx, xpathStr);
			snprintf(xpathStr, sizeof(xpathStr), "%s[1]/imdn:display-notification/imdn:status", imdnPrefix.c_str());
			xmlXPathObjectPtr displayStatusObject = linphone_get_xml_xpath_object_for_node_list(xmlCtx, xpathStr);
			if (deliveryStatusObject && linphone_im_notif_policy_get_recv_imdn_delivered(policy)) {
				if (deliveryStatusObject->nodesetval && (deliveryStatusObject->nodesetval->nodeNr >= 1)) {
					xmlNodePtr node = deliveryStatusObject->nodesetval->nodeTab[0];
					if (node->children && node->children->name) {
						if (strcmp((const char *)node->children->name, "delivered") == 0) {
							linphone_chat_message_update_state(cm, LinphoneChatMessageStateDeliveredToUser);
						} else if (strcmp((const char *)node->children->name, "error") == 0) {
							linphone_chat_message_update_state(cm, LinphoneChatMessageStateNotDelivered);
						}
					}
				}
				xmlXPathFreeObject(deliveryStatusObject);
			}
			if (displayStatusObject && linphone_im_notif_policy_get_recv_imdn_displayed(policy)) {
				if (displayStatusObject->nodesetval && (displayStatusObject->nodesetval->nodeNr >= 1)) {
					xmlNodePtr node = displayStatusObject->nodesetval->nodeTab[0];
					if (node->children && node->children->name) {
						if (strcmp((const char *)node->children->name, "displayed") == 0) {
							linphone_chat_message_update_state(cm, LinphoneChatMessageStateDisplayed);
						}
					}
				}
				xmlXPathFreeObject(displayStatusObject);
			}
			linphone_chat_message_unref(cm);
		}
	}
	if (messageIdStr)
		linphone_free_xml_text_content(messageIdStr);
	if (datetimeStr)
		linphone_free_xml_text_content(datetimeStr);
}

LINPHONE_END_NAMESPACE

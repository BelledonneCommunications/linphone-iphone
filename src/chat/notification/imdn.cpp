/*
 * imdn.cpp
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

#include "chat/chat-message/chat-message.h"
#include "chat/chat-room/chat-room.h"
#include "core/core.h"
#include "logger/logger.h"

#include "imdn.h"

// =============================================================================

using namespace std;

LINPHONE_BEGIN_NAMESPACE

const string Imdn::imdnPrefix = "/imdn:imdn";

string Imdn::createXml (const string &id, time_t time, Imdn::Type imdnType, LinphoneReason reason) {
	xmlBufferPtr buf;
	xmlTextWriterPtr writer;
	int err;
	string content;
	char *datetime = nullptr;

	// Check that the chat message has a message id.
	if (id.empty()) return nullptr;

	buf = xmlBufferCreate();
	if (buf == nullptr) {
		lError() << "Error creating the XML buffer";
		return content;
	}
	writer = xmlNewTextWriterMemory(buf, 0);
	if (writer == nullptr) {
		lError() << "Error creating the XML writer";
		return content;
	}

	datetime = linphone_timestamp_to_rfc3339_string(time);
	err = xmlTextWriterStartDocument(writer, "1.0", "UTF-8", nullptr);
	if (err >= 0) {
		err = xmlTextWriterStartElementNS(writer, nullptr, (const xmlChar *)"imdn",
				(const xmlChar *)"urn:ietf:params:xml:ns:imdn");
	}
	if ((err >= 0) && (reason != LinphoneReasonNone)) {
		err = xmlTextWriterWriteAttributeNS(writer, (const xmlChar *)"xmlns", (const xmlChar *)"linphoneimdn", nullptr, (const xmlChar *)"http://www.linphone.org/xsds/imdn.xsd");
	}
	if (err >= 0) {
		err = xmlTextWriterWriteElement(writer, (const xmlChar *)"message-id", (const xmlChar *)id.c_str());
	}
	if (err >= 0) {
		err = xmlTextWriterWriteElement(writer, (const xmlChar *)"datetime", (const xmlChar *)datetime);
	}
	if (err >= 0) {
		if (imdnType == Imdn::Type::Delivery) {
			err = xmlTextWriterStartElement(writer, (const xmlChar *)"delivery-notification");
		} else {
			err = xmlTextWriterStartElement(writer, (const xmlChar *)"display-notification");
		}
	}
	if (err >= 0) {
		err = xmlTextWriterStartElement(writer, (const xmlChar *)"status");
	}
	if (err >= 0) {
		if (reason == LinphoneReasonNone) {
			if (imdnType == Imdn::Type::Delivery) {
				err = xmlTextWriterStartElement(writer, (const xmlChar *)"delivered");
			} else {
				err = xmlTextWriterStartElement(writer, (const xmlChar *)"displayed");
			}
		} else {
			err = xmlTextWriterStartElement(writer, (const xmlChar *)"error");
		}
	}
	if (err >= 0) {
		// Close the "delivered", "displayed" or "error" element.
		err = xmlTextWriterEndElement(writer);
	}
	if ((err >= 0) && (reason != LinphoneReasonNone)) {
		err = xmlTextWriterStartElementNS(writer, (const xmlChar *)"linphoneimdn", (const xmlChar *)"reason", nullptr);
		if (err >= 0) {
			char codestr[16];
			snprintf(codestr, 16, "%d", linphone_reason_to_error_code(reason));
			err = xmlTextWriterWriteAttribute(writer, (const xmlChar *)"code", (const xmlChar *)codestr);
		}
		if (err >= 0) {
			err = xmlTextWriterWriteString(writer, (const xmlChar *)linphone_reason_to_string(reason));
		}
		if (err >= 0) {
			err = xmlTextWriterEndElement(writer);
		}
	}
	if (err >= 0) {
		// Close the "status" element.
		err = xmlTextWriterEndElement(writer);
	}
	if (err >= 0) {
		// Close the "delivery-notification" or "display-notification" element.
		err = xmlTextWriterEndElement(writer);
	}
	if (err >= 0) {
		// Close the "imdn" element.
		err = xmlTextWriterEndElement(writer);
	}
	if (err >= 0) {
		err = xmlTextWriterEndDocument(writer);
	}
	if (err > 0) {
		// xmlTextWriterEndDocument returns the size of the content.
		content = string((char *)buf->content);
	}
	xmlFreeTextWriter(writer);
	xmlBufferFree(buf);
	ms_free(datetime);
	return content;
}

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
		shared_ptr<ChatMessage> cm = cr.findChatMessage(messageIdStr, ChatMessage::Direction::Outgoing);
		if (!cm) {
			lWarning() << "Received IMDN for unknown message " << messageIdStr;
		} else {
			LinphoneImNotifPolicy *policy = linphone_core_get_im_notif_policy(cr.getCore()->getCCore());
			snprintf(xpathStr, sizeof(xpathStr), "%s[1]/imdn:delivery-notification/imdn:status", imdnPrefix.c_str());
			xmlXPathObjectPtr deliveryStatusObject = linphone_get_xml_xpath_object_for_node_list(xmlCtx, xpathStr);
			snprintf(xpathStr, sizeof(xpathStr), "%s[1]/imdn:display-notification/imdn:status", imdnPrefix.c_str());
			xmlXPathObjectPtr displayStatusObject = linphone_get_xml_xpath_object_for_node_list(xmlCtx, xpathStr);
			if (deliveryStatusObject && linphone_im_notif_policy_get_recv_imdn_delivered(policy)) {
				if (deliveryStatusObject->nodesetval && (deliveryStatusObject->nodesetval->nodeNr >= 1)) {
					xmlNodePtr node = deliveryStatusObject->nodesetval->nodeTab[0];
					if (node->children && node->children->name) {
						if (strcmp((const char *)node->children->name, "delivered") == 0) {
							cm->updateState(ChatMessage::State::DeliveredToUser);
						} else if (strcmp((const char *)node->children->name, "error") == 0) {
							cm->updateState(ChatMessage::State::NotDelivered);
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
							cm->updateState(ChatMessage::State::Displayed);
						}
					}
				}
				xmlXPathFreeObject(displayStatusObject);
			}
		}
	}
	if (messageIdStr)
		linphone_free_xml_text_content(messageIdStr);
	if (datetimeStr)
		linphone_free_xml_text_content(datetimeStr);
}

LINPHONE_END_NAMESPACE

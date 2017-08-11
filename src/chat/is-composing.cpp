/*
 * is-composing.cpp
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

#include "is-composing.h"

#include "chat-room-p.h"

#include "logger/logger.h"

using namespace std;

LINPHONE_BEGIN_NAMESPACE

// =============================================================================

const string IsComposing::isComposingPrefix = "/xsi:isComposing";

// -----------------------------------------------------------------------------

IsComposing::IsComposing (LinphoneCore *core, IsComposingListener *listener)
	: core(core), listener(listener) {}

IsComposing::~IsComposing () {
	stopTimers();
}

// -----------------------------------------------------------------------------

std::string IsComposing::marshal (bool isComposing) {
	string content;

	xmlBufferPtr buf = xmlBufferCreate();
	if (!buf) {
		lError() << "Error creating the XML buffer";
		return content;
	}
	xmlTextWriterPtr writer = xmlNewTextWriterMemory(buf, 0);
	if (!writer) {
		lError() << "Error creating the XML writer";
		return content;
	}

	int err = xmlTextWriterStartDocument(writer, "1.0", "UTF-8", nullptr);
	if (err >= 0) {
		err = xmlTextWriterStartElementNS(writer, nullptr, (const xmlChar *)"isComposing",
			(const xmlChar *)"urn:ietf:params:xml:ns:im-iscomposing");
	}
	if (err >= 0) {
		err = xmlTextWriterWriteAttributeNS(writer, (const xmlChar *)"xmlns", (const xmlChar *)"xsi", nullptr,
			(const xmlChar *)"http://www.w3.org/2001/XMLSchema-instance");
	}
	if (err >= 0) {
		err = xmlTextWriterWriteAttributeNS(writer, (const xmlChar *)"xsi", (const xmlChar *)"schemaLocation", nullptr,
			(const xmlChar *)"urn:ietf:params:xml:ns:im-composing iscomposing.xsd");
	}
	if (err >= 0) {
		err = xmlTextWriterWriteElement(writer, (const xmlChar *)"state",
			isComposing ? (const xmlChar *)"active" : (const xmlChar *)"idle");
	}
	if ((err >= 0) && isComposing) {
		int refreshTimeout = lp_config_get_int(core->config, "sip", "composing_refresh_timeout", defaultRefreshTimeout);
		err = xmlTextWriterWriteElement(writer, (const xmlChar *)"refresh", (const xmlChar *)to_string(refreshTimeout).c_str());
	}
	if (err >= 0) {
		/* Close the "isComposing" element. */
		err = xmlTextWriterEndElement(writer);
	}
	if (err >= 0) {
		err = xmlTextWriterEndDocument(writer);
	}
	if (err > 0) {
		/* xmlTextWriterEndDocument returns the size of the content. */
		content = (char *)buf->content;
	}
	xmlFreeTextWriter(writer);
	xmlBufferFree(buf);
	return content;
}

void IsComposing::parse (const string &text) {
	xmlparsing_context_t *xmlCtx = linphone_xmlparsing_context_new();
	xmlSetGenericErrorFunc(xmlCtx, linphone_xmlparsing_genericxml_error);
	xmlCtx->doc = xmlReadDoc((const unsigned char *)text.c_str(), 0, nullptr, 0);
	if (xmlCtx->doc)
		parse(xmlCtx);
	else
		lWarning() << "Wrongly formatted presence XML: " << xmlCtx->errorBuffer;
	linphone_xmlparsing_context_destroy(xmlCtx);
}

void IsComposing::startIdleTimer () {
	int duration = getIdleTimerDuration();
	if (!idleTimer) {
		idleTimer = sal_create_timer(core->sal, idleTimerExpired, this,
			duration * 1000, "composing idle timeout");
	} else {
		belle_sip_source_set_timeout(idleTimer, duration * 1000);
	}
}

void IsComposing::startRefreshTimer () {
	int duration = getRefreshTimerDuration();
	if (!refreshTimer) {
		refreshTimer = sal_create_timer(core->sal, refreshTimerExpired, this,
			duration * 1000, "composing refresh timeout");
	} else {
		belle_sip_source_set_timeout(refreshTimer, duration * 1000);
	}
}

void IsComposing::startRemoteRefreshTimer () {
	int duration = getRemoteRefreshTimerDuration();
	if (!remoteRefreshTimer) {
		remoteRefreshTimer = sal_create_timer(core->sal, remoteRefreshTimerExpired, this,
			duration * 1000, "composing remote refresh timeout");
	} else {
		belle_sip_source_set_timeout(remoteRefreshTimer, duration * 1000);
	}
}

#if 0
void IsComposing::idleTimerExpired () {
	stopRefreshTimer();
	stopIdleTimer();
}
#endif

void IsComposing::stopTimers () {
	stopIdleTimer();
	stopRefreshTimer();
	stopRemoteRefreshTimer();
}

// -----------------------------------------------------------------------------

void IsComposing::stopIdleTimer () {
	if (idleTimer) {
		if (core && core->sal)
			sal_cancel_timer(core->sal, idleTimer);
		belle_sip_object_unref(idleTimer);
		idleTimer = nullptr;
	}
}

void IsComposing::stopRefreshTimer () {
	if (refreshTimer) {
		if (core && core->sal)
			sal_cancel_timer(core->sal, refreshTimer);
		belle_sip_object_unref(refreshTimer);
		refreshTimer = nullptr;
	}
}

void IsComposing::stopRemoteRefreshTimer () {
	if (remoteRefreshTimer) {
		if (core && core->sal)
			sal_cancel_timer(core->sal, remoteRefreshTimer);
		belle_sip_object_unref(remoteRefreshTimer);
		remoteRefreshTimer = nullptr;
	}
}

// -----------------------------------------------------------------------------

int IsComposing::getIdleTimerDuration () {
	return lp_config_get_int(core->config, "sip", "composing_idle_timeout", defaultIdleTimeout);
}

int IsComposing::getRefreshTimerDuration () {
	return lp_config_get_int(core->config, "sip", "composing_refresh_timeout", defaultRefreshTimeout);
}

int IsComposing::getRemoteRefreshTimerDuration () {
	return lp_config_get_int(core->config, "sip", "composing_remote_refresh_timeout", defaultRemoteRefreshTimeout);
}

void IsComposing::parse (xmlparsing_context_t *xmlCtx) {
	char xpathStr[MAX_XPATH_LENGTH];
	char *stateStr = nullptr;
	char *refreshStr = nullptr;
	int i;
	bool state = false;

	if (linphone_create_xml_xpath_context(xmlCtx) < 0)
		return;

	xmlXPathRegisterNs(xmlCtx->xpath_ctx, (const xmlChar *)"xsi", (const xmlChar *)"urn:ietf:params:xml:ns:im-iscomposing");
	xmlXPathObjectPtr isComposingObject = linphone_get_xml_xpath_object_for_node_list(xmlCtx, isComposingPrefix.c_str());
	if (isComposingObject) {
		if (isComposingObject->nodesetval) {
			for (i = 1; i <= isComposingObject->nodesetval->nodeNr; i++) {
				snprintf(xpathStr, sizeof(xpathStr), "%s[%i]/xsi:state", isComposingPrefix.c_str(), i);
				stateStr = linphone_get_xml_text_content(xmlCtx, xpathStr);
				if (!stateStr)
					continue;
				snprintf(xpathStr, sizeof(xpathStr), "%s[%i]/xsi:refresh", isComposingPrefix.c_str(), i);
				refreshStr = linphone_get_xml_text_content(xmlCtx, xpathStr);
			}
		}
		xmlXPathFreeObject(isComposingObject);
	}

	if (stateStr) {
		if (strcmp(stateStr, "active") == 0) {
			int refreshDuration = getRefreshTimerDuration();
			state = true;
			if (refreshStr)
				refreshDuration = atoi(refreshStr);
			startRemoteRefreshTimer();
		} else {
			stopRemoteRefreshTimer();
		}

		listener->isRemoteComposingStateChanged(state);
		linphone_free_xml_text_content(stateStr);
	}
	if (refreshStr)
		linphone_free_xml_text_content(refreshStr);
}

int IsComposing::idleTimerExpired (unsigned int revents) {
	listener->isComposingStateChanged(false);
	return BELLE_SIP_STOP;
}

int IsComposing::refreshTimerExpired (unsigned int revents) {
	listener->isComposingRefreshNeeded();
	return BELLE_SIP_CONTINUE;
}

int IsComposing::remoteRefreshTimerExpired (unsigned int revents) {
	stopRemoteRefreshTimer();
	listener->isRemoteComposingStateChanged(false);
	return BELLE_SIP_STOP;
}

int IsComposing::idleTimerExpired (void *data, unsigned int revents) {
	IsComposing *d = reinterpret_cast<IsComposing *>(data);
	return d->idleTimerExpired(revents);
}

int IsComposing::refreshTimerExpired (void *data, unsigned int revents) {
	IsComposing *d = reinterpret_cast<IsComposing *>(data);
	return d->refreshTimerExpired(revents);
}

int IsComposing::remoteRefreshTimerExpired (void *data, unsigned int revents) {
	IsComposing *d = reinterpret_cast<IsComposing *>(data);
	return d->remoteRefreshTimerExpired(revents);
}

LINPHONE_END_NAMESPACE

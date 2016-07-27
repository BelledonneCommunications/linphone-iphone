/*
linphone
Copyright (C) 2010-2013  Belledonne Communications SARL

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


#include "private.h"

#include <libxml/xmlreader.h>
#include <libxml/xmlwriter.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>


xmlparsing_context_t * linphone_xmlparsing_context_new(void) {
	xmlparsing_context_t *xmlCtx = (xmlparsing_context_t *)malloc(sizeof(xmlparsing_context_t));
	if (xmlCtx != NULL) {
		xmlCtx->doc = NULL;
		xmlCtx->xpath_ctx = NULL;
		xmlCtx->errorBuffer[0] = '\0';
		xmlCtx->warningBuffer[0] = '\0';
	}
	return xmlCtx;
}

void linphone_xmlparsing_context_destroy(xmlparsing_context_t *ctx) {
	if (ctx->doc != NULL) {
		xmlFreeDoc(ctx->doc);
		ctx->doc = NULL;
	}
	if (ctx->xpath_ctx != NULL) {
		xmlXPathFreeContext(ctx->xpath_ctx);
		ctx->xpath_ctx = NULL;
	}
	free(ctx);
}

void linphone_xmlparsing_genericxml_error(void *ctx, const char *fmt, ...) {
	xmlparsing_context_t *xmlCtx = (xmlparsing_context_t *)ctx;
	size_t sl = strlen(xmlCtx->errorBuffer);
	va_list args;
	va_start(args, fmt);
	vsnprintf(xmlCtx->errorBuffer + sl, XMLPARSING_BUFFER_LEN - sl, fmt, args);
	va_end(args);
}

int linphone_create_xml_xpath_context(xmlparsing_context_t *xml_ctx) {
	if (xml_ctx->xpath_ctx != NULL) {
		xmlXPathFreeContext(xml_ctx->xpath_ctx);
	}
	xml_ctx->xpath_ctx = xmlXPathNewContext(xml_ctx->doc);
	if (xml_ctx->xpath_ctx == NULL) return -1;
	return 0;
}

char * linphone_get_xml_text_content(xmlparsing_context_t *xml_ctx, const char *xpath_expression) {
	xmlXPathObjectPtr xpath_obj;
	xmlChar *text = NULL;
	int i;

	xpath_obj = xmlXPathEvalExpression((const xmlChar *)xpath_expression, xml_ctx->xpath_ctx);
	if (xpath_obj != NULL) {
		if (xpath_obj->nodesetval != NULL) {
			xmlNodeSetPtr nodes = xpath_obj->nodesetval;
			for (i = 0; i < nodes->nodeNr; i++) {
				xmlNodePtr node = nodes->nodeTab[i];
				if (node->children != NULL) {
					text = xmlNodeListGetString(xml_ctx->doc, node->children, 1);
				}
			}
		}
		xmlXPathFreeObject(xpath_obj);
	}

	return (char *)text;
}

const char * linphone_get_xml_attribute_text_content(xmlparsing_context_t *xml_ctx, const char *xpath_expression, const char *attribute_name) {
	xmlXPathObjectPtr xpath_obj;
	xmlChar *text = NULL;

	xpath_obj = xmlXPathEvalExpression((const xmlChar *)xpath_expression, xml_ctx->xpath_ctx);
	if (xpath_obj != NULL) {
		if (xpath_obj->nodesetval != NULL) {
			xmlNodeSetPtr nodes = xpath_obj->nodesetval;
			if ((nodes != NULL) && (nodes->nodeNr >= 1)) {
				xmlNodePtr node = nodes->nodeTab[0];
				xmlAttr *attr = node->properties;
				while (attr) {
					if (strcmp((char *)attr->name, attribute_name) == 0) {
						text = xmlStrcat(text, attr->children->content);
						attr = NULL;
					} else {
						attr = attr->next;
					}
				}
			}
		}
		xmlXPathFreeObject(xpath_obj);
	}

	return (const char *)text;
}

void linphone_free_xml_text_content(const char *text) {
	xmlFree((xmlChar *)text);
}

xmlXPathObjectPtr linphone_get_xml_xpath_object_for_node_list(xmlparsing_context_t *xml_ctx, const char *xpath_expression) {
	return xmlXPathEvalExpression((const xmlChar *)xpath_expression, xml_ctx->xpath_ctx);
}

void linphone_xml_xpath_context_init_carddav_ns(xmlparsing_context_t *xml_ctx) {
	if (xml_ctx && xml_ctx->xpath_ctx) {
		xmlXPathRegisterNs(xml_ctx->xpath_ctx, (const xmlChar*)"d", (const xmlChar*)"DAV:");
		xmlXPathRegisterNs(xml_ctx->xpath_ctx, (const xmlChar*)"card", (const xmlChar*)"urn:ietf:params:xml:ns:carddav");
		xmlXPathRegisterNs(xml_ctx->xpath_ctx, (const xmlChar*)"x1", (const xmlChar*)"http://calendarserver.org/ns/");
	}
}
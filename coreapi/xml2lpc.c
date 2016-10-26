/*
linphone
Copyright (C) 2012 Belledonne Communications SARL
Yann DIORCET (yann.diorcet@linphone.org)

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

#include "xml2lpc.h"
#include <string.h>
#include <libxml/xmlreader.h>


#define XML2LPC_BZ 2048

struct _xml2lpc_context {
	LpConfig *lpc;
	xml2lpc_function cbf;
	void *ctx;

	xmlDoc *doc;
	xmlDoc *xsd;
	char errorBuffer[XML2LPC_BZ];
	char warningBuffer[XML2LPC_BZ];
};


xml2lpc_context* xml2lpc_context_new(xml2lpc_function cbf, void *ctx) {
	xml2lpc_context *xmlCtx = (xml2lpc_context*)malloc(sizeof(xml2lpc_context));
	if(xmlCtx != NULL) {
		xmlCtx->lpc = NULL;
		xmlCtx->cbf = cbf;
		xmlCtx->ctx = ctx;

		xmlCtx->doc = NULL;
		xmlCtx->xsd = NULL;
		xmlCtx->errorBuffer[0]='\0';
		xmlCtx->warningBuffer[0]='\0';
	}
	return xmlCtx;
}

void xml2lpc_context_destroy(xml2lpc_context *ctx) {
	if(ctx->doc != NULL) {
		xmlFreeDoc(ctx->doc);
		ctx->doc = NULL;
	}
	if(ctx->xsd != NULL) {
		xmlFreeDoc(ctx->xsd);
		ctx->xsd = NULL;
	}
	free(ctx);
}

static void xml2lpc_context_clear_logs(xml2lpc_context *ctx) {
	ctx->errorBuffer[0]='\0';
	ctx->warningBuffer[0]='\0';
}

static void xml2lpc_log(xml2lpc_context *xmlCtx, int level, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	if(xmlCtx->cbf != NULL) {
		xmlCtx->cbf((xmlCtx)->ctx, level, fmt, args);
	}
	va_end(args);
}

static void xml2lpc_genericxml_error(void *ctx, const char *fmt, ...) {
	xml2lpc_context *xmlCtx = (xml2lpc_context *)ctx;
	size_t sl = strlen(xmlCtx->errorBuffer);
	va_list args;
	va_start(args, fmt);
	vsnprintf(xmlCtx->errorBuffer + sl, XML2LPC_BZ-sl, fmt, args);
	va_end(args);
}

static void xml2lpc_genericxml_warning(void *ctx, const char *fmt, ...) {
	xml2lpc_context *xmlCtx = (xml2lpc_context *)ctx;
	size_t sl = strlen(xmlCtx->warningBuffer);
	va_list args;
	va_start(args, fmt);
	vsnprintf(xmlCtx->warningBuffer + sl, XML2LPC_BZ-sl, fmt, args);
	va_end(args);
}

#if 0
static void dumpNodes(int level, xmlNode * a_node, xml2lpc_context *ctx) {
	xmlNode *cur_node = NULL;

	for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
		if (cur_node->type == XML_ELEMENT_NODE) {
			xml2lpc_log(ctx, XML2LPC_DEBUG, "node level: %d type: Element, name: %s", level, cur_node->name);
		} else {
			xml2lpc_log(ctx, XML2LPC_DEBUG, "node level: %d type: %d, name: %s", level, cur_node->type, cur_node->name);
		}

		dumpNodes(level + 1, cur_node->children, ctx);
	}
}
#endif


static void dumpNode(xmlNode *node, xml2lpc_context *ctx) {
	xml2lpc_log(ctx, XML2LPC_DEBUG, "node type: %d, name: %s", node->type, node->name);
}

static void dumpAttr(xmlNode *node, xml2lpc_context *ctx) {
	xml2lpc_log(ctx, XML2LPC_DEBUG, "attr name: %s value:%s", node->name, node->children->content);
}

static void dumpContent(xmlNode *node, xml2lpc_context *ctx) {
	if (node->children)
		xml2lpc_log(ctx, XML2LPC_DEBUG, "content: %s", node->children->content);
	else
		xml2lpc_log(ctx, XML2LPC_DEBUG, "content: ");
}

static int processEntry(xmlElement *element, const char *sectionName, xml2lpc_context *ctx) {
	xmlNode *cur_attr = NULL;
	const char *name = NULL;
	const char *value = NULL;
	bool_t overwrite = FALSE;

	for (cur_attr = (xmlNode *)element->attributes; cur_attr; cur_attr = cur_attr->next) {
		dumpAttr(cur_attr, ctx);
		if(strcmp((const char*)cur_attr->name, "name") == 0) {
			name = (const char*)cur_attr->children->content;
		} else if(strcmp((const char*)cur_attr->name, "overwrite") == 0) {
			if(strcmp((const char*)cur_attr->children->content, "true") == 0) {
				overwrite = TRUE;
			}
		}
	}

	dumpContent((xmlNode *)element, ctx);
	if (element->children)
		value = (const char *)element->children->content;
	else
		value = "";

	if(name != NULL) {
		const char *str = lp_config_get_string(ctx->lpc, sectionName, name, NULL);
		if(str == NULL || overwrite) {
			xml2lpc_log(ctx, XML2LPC_MESSAGE, "Set %s|%s = %s", sectionName, name, value);
			lp_config_set_string(ctx->lpc, sectionName, name, value);
		} else {
			xml2lpc_log(ctx, XML2LPC_MESSAGE, "Don't touch %s|%s = %s",sectionName, name, str);
		}
	} else {
		xml2lpc_log(ctx, XML2LPC_WARNING, "ignored entry with no \"name\" attribute line:%d",xmlGetLineNo((xmlNode*)element));
	}
		return 0;
}

static int processSection(xmlElement *element, xml2lpc_context *ctx) {
	xmlNode *cur_node = NULL;
	xmlNode *cur_attr = NULL;
	const char *name = NULL;

	for (cur_attr = (xmlNode *)element->attributes; cur_attr; cur_attr = cur_attr->next) {
		dumpAttr(cur_attr, ctx);
		if(strcmp((const char*)cur_attr->name, "name") == 0) {
			name = (const char*)cur_attr->children->content;
		}
	}

	if(name != NULL) {
		for (cur_node = element->children; cur_node; cur_node = cur_node->next) {
			dumpNode(cur_node, ctx);
			if (cur_node->type == XML_ELEMENT_NODE) {
				if(strcmp((const char*)cur_node->name, "entry") == 0 ) {
					processEntry((xmlElement*)cur_node, name, ctx);
				}
			}

		}
		} else {
			xml2lpc_log(ctx, XML2LPC_WARNING, "ignored section with no \"name\" attribute, line:%d", xmlGetLineNo((xmlNode*)element));
		}

		return 0;
}

static int processConfig(xmlElement *element, xml2lpc_context *ctx) {
	xmlNode *cur_node = NULL;

	for (cur_node = element->children; cur_node; cur_node = cur_node->next) {
		dumpNode(cur_node, ctx);
		if (cur_node->type == XML_ELEMENT_NODE &&
			strcmp((const char*)cur_node->name, "section") == 0 ) {
			processSection((xmlElement*)cur_node, ctx);
			}

		}
	return 0;
}

static int processDoc(xmlNode *node, xml2lpc_context *ctx) {
	dumpNode(node, ctx);

	if (node->type == XML_ELEMENT_NODE &&
		strcmp((const char*)node->name, "config") == 0 ) {
		processConfig((xmlElement*)node, ctx);
	} else {
		xml2lpc_log(ctx, XML2LPC_WARNING, "root element is not \"config\", line:%d", xmlGetLineNo(node));
	}
	return 0;
}

static int internal_convert_xml2lpc(xml2lpc_context *ctx) {
	xmlNode *rootNode;
	int ret;

	xml2lpc_log(ctx, XML2LPC_DEBUG, "Parse started");
	rootNode = xmlDocGetRootElement(ctx->doc);
	//dumpNodes(0, rootNode, cbf, ctx);
	ret = processDoc(rootNode, ctx);
	xml2lpc_log(ctx, XML2LPC_DEBUG, "Parse ended ret:%d", ret);
	return ret;
}

int xml2lpc_validate(xml2lpc_context *xmlCtx) {
	xmlSchemaValidCtxtPtr validCtx;
	xmlSchemaParserCtxtPtr parserCtx;
	int ret;

	xml2lpc_context_clear_logs(xmlCtx);
	parserCtx = xmlSchemaNewDocParserCtxt(xmlCtx->xsd);
	validCtx = xmlSchemaNewValidCtxt(xmlSchemaParse(parserCtx));
	xmlSchemaSetValidErrors(validCtx, xml2lpc_genericxml_error, xml2lpc_genericxml_warning, xmlCtx);
	ret = xmlSchemaValidateDoc(validCtx, xmlCtx->doc);
	if(ret > 0) {
		if(strlen(xmlCtx->warningBuffer) > 0)
			xml2lpc_log(xmlCtx, XML2LPC_WARNING, "%s", xmlCtx->warningBuffer);
		if(strlen(xmlCtx->errorBuffer) > 0)
			xml2lpc_log(xmlCtx, XML2LPC_ERROR, "%s", xmlCtx->errorBuffer);
	} else if(ret < 0) {
		xml2lpc_log(xmlCtx, XML2LPC_ERROR, "Internal error");
	}
	xmlSchemaFreeValidCtxt(validCtx);
	return ret;
}

int xml2lpc_convert(xml2lpc_context *xmlCtx, LpConfig *lpc) {
	xml2lpc_context_clear_logs(xmlCtx);
	if(xmlCtx->doc == NULL) {
		xml2lpc_log(xmlCtx, XML2LPC_ERROR, "No doc set");
		return -1;
	}
	if(lpc == NULL) {
		xml2lpc_log(xmlCtx, XML2LPC_ERROR, "Invalid lpc");
	}
	xmlCtx->lpc = lpc;
	return internal_convert_xml2lpc(xmlCtx);
}

int xml2lpc_set_xml_file(xml2lpc_context* xmlCtx, const char *filename) {
	xml2lpc_context_clear_logs(xmlCtx);
	xmlSetGenericErrorFunc(xmlCtx, xml2lpc_genericxml_error);
	if(xmlCtx->doc != NULL) {
		xmlFreeDoc(xmlCtx->doc);
		xmlCtx->doc = NULL;
	}
	xmlCtx->doc = xmlReadFile(filename, NULL, 0);
	if(xmlCtx->doc == NULL) {
		xml2lpc_log(xmlCtx, XML2LPC_ERROR, "Can't open/parse file \"%s\"", filename);
		xml2lpc_log(xmlCtx, XML2LPC_ERROR, "%s", xmlCtx->errorBuffer);
		return -1;
	}
	return 0;
}

int xml2lpc_set_xml_fd(xml2lpc_context* xmlCtx, int fd) {
	xml2lpc_context_clear_logs(xmlCtx);
	xmlSetGenericErrorFunc(xmlCtx, xml2lpc_genericxml_error);
	if(xmlCtx->doc != NULL) {
		xmlFreeDoc(xmlCtx->doc);
		xmlCtx->doc = NULL;
	}
	xmlCtx->doc = xmlReadFd(fd, 0, NULL, 0);
	if(xmlCtx->doc == NULL) {
		xml2lpc_log(xmlCtx, XML2LPC_ERROR, "Can't open/parse fd \"%d\"", fd);
		xml2lpc_log(xmlCtx, XML2LPC_ERROR, "%s", xmlCtx->errorBuffer);
		return -1;
	}
	return 0;
}

int xml2lpc_set_xml_string(xml2lpc_context* xmlCtx, const char *content) {
	xml2lpc_context_clear_logs(xmlCtx);
	xmlSetGenericErrorFunc(xmlCtx, xml2lpc_genericxml_error);
	if(xmlCtx->doc != NULL) {
		xmlFreeDoc(xmlCtx->doc);
		xmlCtx->doc = NULL;
	}
	xmlCtx->doc = xmlReadDoc((const unsigned char*)content, 0, NULL, 0);
	if(xmlCtx->doc == NULL) {
		xml2lpc_log(xmlCtx, XML2LPC_ERROR, "Can't parse string");
		xml2lpc_log(xmlCtx, XML2LPC_ERROR, "%s", xmlCtx->errorBuffer);
		return -1;
	}
	return 0;
}

int xml2lpc_set_xsd_file(xml2lpc_context* xmlCtx, const char *filename) {
	xml2lpc_context_clear_logs(xmlCtx);
	xmlSetGenericErrorFunc(xmlCtx, xml2lpc_genericxml_error);
	if(xmlCtx->xsd != NULL) {
		xmlFreeDoc(xmlCtx->xsd);
		xmlCtx->xsd = NULL;
	}
	xmlCtx->xsd = xmlReadFile(filename, NULL, 0);
	if(xmlCtx->xsd == NULL) {
		xml2lpc_log(xmlCtx, XML2LPC_ERROR, "Can't open/parse file \"%s\"", filename);
		xml2lpc_log(xmlCtx, XML2LPC_ERROR, "%s", xmlCtx->errorBuffer);
		return -1;
	}
	return 0;
}

int xml2lpc_set_xsd_fd(xml2lpc_context* xmlCtx, int fd) {
	xml2lpc_context_clear_logs(xmlCtx);
	xmlSetGenericErrorFunc(xmlCtx, xml2lpc_genericxml_error);
	if(xmlCtx->xsd != NULL) {
		xmlFreeDoc(xmlCtx->xsd);
		xmlCtx->xsd = NULL;
	}
	xmlCtx->xsd = xmlReadFd(fd, 0, NULL, 0);
	if(xmlCtx->xsd == NULL) {
		xml2lpc_log(xmlCtx, XML2LPC_ERROR, "Can't open/parse fd \"%d\"", fd);
		xml2lpc_log(xmlCtx, XML2LPC_ERROR, "%s", xmlCtx->errorBuffer);
		return -1;
	}
	return 0;
}

int xml2lpc_set_xsd_string(xml2lpc_context* xmlCtx, const char *content) {
	xml2lpc_context_clear_logs(xmlCtx);
	xmlSetGenericErrorFunc(xmlCtx, xml2lpc_genericxml_error);
	if(xmlCtx->xsd != NULL) {
		xmlFreeDoc(xmlCtx->xsd);
		xmlCtx->xsd = NULL;
	}
	xmlCtx->xsd = xmlReadDoc((const unsigned char*)content, 0, NULL, 0);
	if(xmlCtx->xsd == NULL) {
		xml2lpc_log(xmlCtx, XML2LPC_ERROR, "Can't parse string");
		xml2lpc_log(xmlCtx, XML2LPC_ERROR, "%s", xmlCtx->errorBuffer);
		return -1;
	}
	return 0;
}

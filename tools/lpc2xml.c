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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "lpc2xml.h"
#include <string.h>
#include <libxml/xmlsave.h>
#include <libxml/xmlversion.h>

#define LPC2XML_BZ 2048

struct _lpc2xml_context {
	const LpConfig *lpc;
	lpc2xml_function cbf;
	void *ctx;
	
	xmlDoc *doc;
	char errorBuffer[LPC2XML_BZ];
	char warningBuffer[LPC2XML_BZ];
};


lpc2xml_context* lpc2xml_context_new(lpc2xml_function cbf, void *ctx) {
	lpc2xml_context *xmlCtx = (lpc2xml_context*)malloc(sizeof(lpc2xml_context));
	if(xmlCtx != NULL) {
		xmlCtx->lpc = NULL;
		xmlCtx->cbf = cbf;
		xmlCtx->ctx = ctx;
		
		xmlCtx->doc = NULL;
		xmlCtx->errorBuffer[0]='\0';
		xmlCtx->warningBuffer[0]='\0';
	}
	return xmlCtx;
}

void lpc2xml_context_destroy(lpc2xml_context *ctx) {
	if(ctx->doc != NULL) {
		xmlFreeDoc(ctx->doc);
		ctx->doc = NULL;
	}
	free(ctx);
}

static void lpc2xml_context_clear_logs(lpc2xml_context *ctx) {
	ctx->errorBuffer[0]='\0';
	ctx->warningBuffer[0]='\0';
}

static void lpc2xml_log(lpc2xml_context *xmlCtx, int level, const char *fmt, ...) {
	va_list args;	
	va_start(args, fmt);	
	if(xmlCtx->cbf != NULL) {
		xmlCtx->cbf((xmlCtx)->ctx, level, fmt, args);
	}
 	va_end(args);
}

static void lpc2xml_genericxml_error(void *ctx, const char *fmt, ...) {
	lpc2xml_context *xmlCtx = (lpc2xml_context *)ctx;
	int sl = strlen(xmlCtx->errorBuffer);
	va_list args;	
	va_start(args, fmt);	
	vsnprintf(xmlCtx->errorBuffer + sl, LPC2XML_BZ-sl, fmt, args);
	va_end(args);
}

/*
static void lpc2xml_genericxml_warning(void *ctx, const char *fmt, ...) {
	lpc2xml_context *xmlCtx = (lpc2xml_context *)ctx;
	int sl = strlen(xmlCtx->warningBuffer);
	va_list args;	
	va_start(args, fmt);	
	vsnprintf(xmlCtx->warningBuffer + sl, LPC2XML_BZ-sl, fmt, args);
	va_end(args);
}
*/

static int processEntry(const char *section, const char *entry, xmlNode *node, lpc2xml_context *ctx) {
	const char *content = lp_config_get_string(ctx->lpc, section, entry, NULL);
	if(content == NULL) {
		lpc2xml_log(ctx->ctx, LPC2XML_ERROR, "Issue when reading the lpc");
		return -1;
	}
	
	lpc2xml_log(ctx, LPC2XML_MESSAGE, "Set %s|%s = %s", section, entry, content);
	xmlNodeSetContent(node, (const xmlChar *) content);
	return 0;
}

struct __processSectionCtx {
	int ret;
	const char *section;
	xmlNode *node;
	lpc2xml_context *ctx;
};

static void processSection_cb(const char *entry, struct __processSectionCtx *ctx) {
	if(ctx->ret == 0) {
		xmlNode *node = xmlNewChild(ctx->node, NULL, (const xmlChar *)"entry", NULL);
		if(node == NULL) {
			lpc2xml_log(ctx->ctx, LPC2XML_ERROR, "Can't create \"entry\" element");
			ctx->ret = -1;
			return;
		}
		xmlAttr *name_attr = xmlSetProp(node, (const xmlChar *)"name", (const xmlChar *)entry);
		if(name_attr == NULL) {
			lpc2xml_log(ctx->ctx, LPC2XML_ERROR, "Can't create name attribute for \"entry\" element");
			ctx->ret = -1;
			return;
		}
	
		ctx->ret = processEntry(ctx->section, entry, node, ctx->ctx);
	}
}

static int processSection(const char *section, xmlNode *node, lpc2xml_context *ctx) {
	struct __processSectionCtx pc_ctx = {0, section, node, ctx};
	lp_config_for_each_entry(ctx->lpc, section, (void (*)(const char *, void *))processSection_cb, (void*)&pc_ctx);
	return pc_ctx.ret;
}



struct __processConfigCtx {
	int ret;
	xmlNode *node;
	lpc2xml_context *ctx;
};

static void processConfig_cb(const char *section, struct __processConfigCtx *ctx) {
	if(ctx->ret == 0) {
		xmlNode *node = xmlNewChild(ctx->node, NULL, (const xmlChar *)"section", NULL);
		if(node == NULL) {
			lpc2xml_log(ctx->ctx, LPC2XML_ERROR, "Can't create \"section\" element");
			ctx->ret = -1;
			return;
		}
		xmlAttr *name_attr = xmlSetProp(node, (const xmlChar *)"name", (const xmlChar *)section);
		if(name_attr == NULL) {
			lpc2xml_log(ctx->ctx, LPC2XML_ERROR, "Can't create name attribute for \"section\" element");
			ctx->ret = -1;
			return;
		}
		ctx->ret = processSection(section, node, ctx->ctx);
	}
}

static int processConfig(xmlNode *node, lpc2xml_context *ctx) {
	struct __processConfigCtx pc_ctx = {0, node, ctx};
	lp_config_for_each_section(ctx->lpc, (void (*)(const char *, void *))processConfig_cb, (void*)&pc_ctx);
	return pc_ctx.ret;
}

static int processDoc(xmlDoc *doc, lpc2xml_context *ctx) {
	int ret = 0;
	xmlNode *root_node = xmlNewNode(NULL, (const xmlChar *)"config");
	if(root_node == NULL) {
		lpc2xml_log(ctx, LPC2XML_ERROR, "Can't create \"config\" element");
		return -1;
	}
	xmlNs *lpc_ns = xmlNewNs(root_node, (const xmlChar *)"http://www.linphone.org/xsds/lpconfig.xsd", NULL);
	if(lpc_ns == NULL) {
		lpc2xml_log(ctx, LPC2XML_WARNING, "Can't create lpc namespace");
	} else {
		xmlSetNs(root_node, lpc_ns);
	}
	xmlNs *xsi_ns = xmlNewNs(root_node, (const xmlChar *)"http://www.w3.org/2001/XMLSchema-instance", (const xmlChar *)"xsi");
	if(lpc_ns == NULL) {
		lpc2xml_log(ctx, LPC2XML_WARNING, "Can't create xsi namespace");
	}
	xmlAttr *schemaLocation = xmlNewNsProp(root_node, xsi_ns, (const xmlChar *)"schemaLocation", (const xmlChar *)"http://www.linphone.org/xsds/lpconfig.xsd lpconfig.xsd");
	if(schemaLocation == NULL) {
		lpc2xml_log(ctx, LPC2XML_WARNING, "Can't create schemaLocation");
	}
	ret = processConfig(root_node, ctx);
	xmlDocSetRootElement(doc, root_node);
	return ret;
}

static int internal_convert_lpc2xml(lpc2xml_context *ctx) {
	int ret = 0;
	lpc2xml_log(ctx, LPC2XML_DEBUG, "Generation started");
	if(ctx->doc != NULL) {
		xmlFreeDoc(ctx->doc);
		ctx->doc = NULL;
	}
	xmlDoc *doc = xmlNewDoc((const xmlChar *)"1.0");
	ret  = processDoc(doc, ctx);
	if(ret == 0) {
		ctx->doc = doc;
	} else {
		xmlFreeDoc(doc);
	}
	lpc2xml_log(ctx, LPC2XML_DEBUG, "Generation ended ret:%d", ret);
	return ret;
}

int lpc2xml_set_lpc(lpc2xml_context* context, const LpConfig *lpc) {
	context->lpc = lpc;
	return 0;
}

int lpc2xml_convert_file(lpc2xml_context* context, const char *filename) {
	int ret = -1;
	lpc2xml_context_clear_logs(context);
	xmlSetGenericErrorFunc(context, lpc2xml_genericxml_error);
	xmlSaveCtxtPtr save_ctx = xmlSaveToFilename(filename, "UTF-8", XML_SAVE_FORMAT);
	if(save_ctx != NULL) {
		ret = internal_convert_lpc2xml(context);
		if(ret == 0) {
			ret = xmlSaveDoc(save_ctx, context->doc);
			if(ret != 0) {
				lpc2xml_log(context, LPC2XML_ERROR, "Can't save document");
				lpc2xml_log(context, LPC2XML_ERROR, "%s", context->errorBuffer);
			}
		}
		xmlSaveClose(save_ctx);
	} else {
		lpc2xml_log(context, LPC2XML_ERROR, "Can't open file:%s", filename);
		lpc2xml_log(context, LPC2XML_ERROR, "%s", context->errorBuffer);
	}
	return ret;
}

int lpc2xml_convert_fd(lpc2xml_context* context, int fd) {
	int ret = -1;
	lpc2xml_context_clear_logs(context);
	xmlSetGenericErrorFunc(context, lpc2xml_genericxml_error);
	xmlSaveCtxtPtr save_ctx = xmlSaveToFd(fd, "UTF-8", XML_SAVE_FORMAT);
	if(save_ctx != NULL) {
		ret = internal_convert_lpc2xml(context);
		if(ret == 0) {
			ret = xmlSaveDoc(save_ctx, context->doc);
			if(ret != 0) {
				lpc2xml_log(context, LPC2XML_ERROR, "Can't save document");
				lpc2xml_log(context, LPC2XML_ERROR, "%s", context->errorBuffer);
			}
		}
		xmlSaveClose(save_ctx);
	} else {
		lpc2xml_log(context, LPC2XML_ERROR, "Can't open fd:%d", fd);
		lpc2xml_log(context, LPC2XML_ERROR, "%s", context->errorBuffer);
	}
	return ret;
}

int lpc2xml_convert_string(lpc2xml_context* context, char **content) {
	int ret = -1;
	xmlBufferPtr buffer = xmlBufferCreate();
	lpc2xml_context_clear_logs(context);
	xmlSetGenericErrorFunc(context, lpc2xml_genericxml_error);
	xmlSaveCtxtPtr save_ctx = xmlSaveToBuffer(buffer, "UTF-8", XML_SAVE_FORMAT);
	if(save_ctx != NULL) {
		ret = internal_convert_lpc2xml(context);
		if(ret == 0) {
			ret = xmlSaveDoc(save_ctx, context->doc);
			if(ret != 0) {
				lpc2xml_log(context, LPC2XML_ERROR, "Can't save document");
				lpc2xml_log(context, LPC2XML_ERROR, "%s", context->errorBuffer);
			}
		}
		xmlSaveClose(save_ctx);
	} else {
		lpc2xml_log(context, LPC2XML_ERROR, "Can't initialize internal buffer");
		lpc2xml_log(context, LPC2XML_ERROR, "%s", context->errorBuffer);
	}
	if(ret == 0) {
#if LIBXML_VERSION >= 20800
		*content = (char *)xmlBufferDetach(buffer);
#else
		*content = strdup((const char *)xmlBufferContent(buffer));
#endif
	}
	xmlBufferFree(buffer);
	return ret;
}

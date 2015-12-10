#ifndef LINPHONE_VCARD_H
#define LINPHONE_VCARD_H

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _LinphoneVCard LinphoneVCard;

LinphoneVCard* linphone_vcard_new(void);
void linphone_vcard_free(LinphoneVCard *vcard);

void linphone_vcard_set_full_name(LinphoneVCard *vcard, const char *name);
const char* linphone_vcard_get_full_name(LinphoneVCard *vcard);

#ifdef __cplusplus
}
#endif

#endif
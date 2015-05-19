#include "media-encryption.h"

using namespace std;

class MediaEncryptionResponse : public Response {
public:
	MediaEncryptionResponse(LinphoneCore *core);
};

MediaEncryptionResponse::MediaEncryptionResponse(LinphoneCore *core) : Response() {
	LinphoneMediaEncryption encryption = linphone_core_get_media_encryption(core);
	ostringstream ost;
	ost << "Encryption: ";
	switch (encryption) {
		case LinphoneMediaEncryptionNone:
			ost << "none\n";
			break;
		case LinphoneMediaEncryptionSRTP:
			ost << "srtp\n";
			break;
		case LinphoneMediaEncryptionZRTP:
			ost << "zrtp\n";
			break;
		case LinphoneMediaEncryptionDTLS:
			ost << "DTLS\n";
			break;
	}
	setBody(ost.str().c_str());
}

MediaEncryptionCommand::MediaEncryptionCommand() :
		DaemonCommand("media-encryption", "media-encryption [none|srtp|zrtp]",
				"Set the media encryption policy if a parameter is given, otherwise return the media encrytion in use.") {
	addExample(new DaemonCommandExample("media-encryption none",
						"Status: Ok\n\n"
						"Encryption: none"));
	addExample(new DaemonCommandExample("media-encryption srtp",
						"Status: Ok\n\n"
						"Encryption: srtp"));
	addExample(new DaemonCommandExample("media-encryption",
						"Status: Ok\n\n"
						"Encryption: srtp"));
}

void MediaEncryptionCommand::exec(Daemon *app, const char *args) {
	string encryption_str;
	istringstream ist(args);
	ist >> encryption_str;
	if (ist.eof() && (encryption_str.length() == 0)) {
		app->sendResponse(MediaEncryptionResponse(app->getCore()));
	} else if (ist.fail()) {
		app->sendResponse(Response("Incorrect parameter.", Response::Error));
	} else {
		LinphoneMediaEncryption encryption;
		if (encryption_str.compare("none") == 0) {
			encryption = LinphoneMediaEncryptionNone;
		} else if (encryption_str.compare("srtp") == 0) {
			encryption = LinphoneMediaEncryptionSRTP;
		} else if (encryption_str.compare("zrtp") == 0) {
			encryption = LinphoneMediaEncryptionZRTP;
		} else {
			app->sendResponse(Response("Incorrect parameter.", Response::Error));
			return;
		}
		if (linphone_core_set_media_encryption(app->getCore(), encryption)==0){
			app->sendResponse(MediaEncryptionResponse(app->getCore()));
		}else{
			app->sendResponse(Response("Unsupported media encryption", Response::Error));
		}
	}
}

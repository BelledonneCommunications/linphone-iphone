#include "media-encryption.h"

using namespace std;

class MediaEncryptionCommandPrivate {
public:
	void outputMediaEncryption(Daemon *app, ostringstream &ost);
};

void MediaEncryptionCommandPrivate::outputMediaEncryption(Daemon* app, ostringstream& ost) {
	LinphoneMediaEncryption encryption = linphone_core_get_media_encryption(app->getCore());
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
	}
}

MediaEncryptionCommand::MediaEncryptionCommand() :
		DaemonCommand("media-encryption", "media-encryption [none|srtp|zrtp]",
				"Set the media encryption policy if a parameter is given, otherwise return the media encrytion in use."),
		d(new MediaEncryptionCommandPrivate()) {
}

MediaEncryptionCommand::~MediaEncryptionCommand() {
	delete d;
}

void MediaEncryptionCommand::exec(Daemon *app, const char *args) {
	string encryption_str;
	istringstream ist(args);
	ist >> encryption_str;
	if (ist.eof() && (encryption_str.length() == 0)) {
		ostringstream ost;
		d->outputMediaEncryption(app, ost);
		app->sendResponse(Response(ost.str().c_str(), Response::Ok));
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
		linphone_core_set_media_encryption(app->getCore(), encryption);
		ostringstream ost;
		d->outputMediaEncryption(app, ost);
		app->sendResponse(Response(ost.str().c_str(), Response::Ok));
	}
}

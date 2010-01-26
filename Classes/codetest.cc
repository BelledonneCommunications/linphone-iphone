#include "codechecker.hh"

using namespace axcodechecker;

int main(int argc, char *argv[]){
	if (argc<3){
		printf("codetest [key] [phone number]\n");
		return -1;
	}
	CodeChecker *cc=CodeChecker::get();
	cc->setCode(argv[1]);
	cc->setPhoneNumber(argv[2]);
	CodeChecker::Result r=cc->validate();
	if (r==CodeChecker::Ok){
		printf("Key and number are valid; ip address is %s\n",cc->getIpAddress());
	}else if (r==CodeChecker::WrongPhoneNumber){
		printf("Phone number verification failed\n");
	}else if (r==CodeChecker::WrongCode){
		printf("Key verification failed.\n");
	}
	return 0;
}


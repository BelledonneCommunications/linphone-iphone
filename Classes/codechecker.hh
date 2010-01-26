#include "bitlib.hh"

namespace axcodechecker{

class CodeChecker{
	public:
		enum Result{ Ok, WrongPhoneNumber, WrongCode};
		static CodeChecker *get();
		void setCode(const char *code);
		void setPhoneNumber(const char *phoneNumber);
		Result validate();
		const char *getIpAddress()const{
			return mIpAddress;
		}
	private:
		byte sdbmHash(const char *str);
		CodeChecker();
		char mCode[64];
		char mNumber[32];
		char mIpAddress[32];
		static CodeChecker *sUnique;
};

}//end of namespace


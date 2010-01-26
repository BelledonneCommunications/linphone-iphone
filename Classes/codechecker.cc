#include "codechecker.hh"
#include <arpa/inet.h>
#include <string.h>
#include <ctype.h>

#include "decodetable.cc"

using namespace BitLib;

namespace axcodechecker{

CodeChecker * CodeChecker::sUnique=0;
    
CodeChecker * CodeChecker::get(){
	if (sUnique==0)
		sUnique=new CodeChecker();
	return sUnique;
}

CodeChecker::CodeChecker(){
	memset(mIpAddress,0,sizeof(mIpAddress));
	memset(mCode,0,sizeof(mCode));
	memset(mNumber,0,sizeof(mNumber));
}

static UInt64 hexStringToUint64(const char *str){
	return strtoll(str,NULL,16);
}

CodeChecker::Result CodeChecker::validate(){
	UInt64 key = hexStringToUint64(mCode);

	key = Bit::Swizzle(key, decodeTable, decodeTableLen);

	UInt32 ip = (UInt32)(key >> 16);
	byte phoneHash = (byte)(key >> 8);
	byte crc = (byte)key;

	byte b1 = (byte)(phoneHash & 0xff);
	byte b2 = (byte)(ip & 0xff);
	byte b3 = (byte)((ip >> 8) & 0xff);
	byte b4 = (byte)((ip >> 16) & 0xff);
	byte b5 = (byte)((ip >> 24) & 0xff);

	byte crc2 = (byte)(b5 ^ b1 ^ b4 ^ b2 ^ b3);

	if (crc == crc2)
	{
	    if (phoneHash == sdbmHash(mNumber))
	    {
		struct in_addr ia;
		ia.s_addr=htonl(ip);
		inet_ntop(AF_INET,&ia,mIpAddress,sizeof(mIpAddress));
		return Ok;
	    }
	    else
	    {
		return WrongPhoneNumber;
	    }
	}

	return WrongCode;
}

byte CodeChecker::sdbmHash(const char *str){
	uint hash = 0;
	int i;
	int ch;
	for(i=0;str[i]!='\0';++i){
		ch=str[i];
		hash = ch + (hash << 6) + (hash << 16) - hash;
	}
	return (byte)hash;
}

void CodeChecker::setCode(const char *code){
	strncpy(mCode,code,sizeof(mCode));
}

void CodeChecker::setPhoneNumber(const char *phoneNumber){
	strncpy(mNumber,phoneNumber,sizeof(mNumber));
}


}

extern "C" const char* axtunnel_get_ip_from_key(const char* phone,const  char* key) {
	axcodechecker::CodeChecker::get()->setPhoneNumber(phone);
	axcodechecker::CodeChecker::get()->setCode(key);
	axcodechecker::CodeChecker::Result result=axcodechecker::CodeChecker::get()->validate();
	if (result == axcodechecker::CodeChecker::Ok) {
		return axcodechecker::CodeChecker::get()->getIpAddress();
	} else {
		return 0;
	}

}


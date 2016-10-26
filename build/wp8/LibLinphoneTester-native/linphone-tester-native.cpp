#include <string>

#include "linphone-tester-native.h"
#include "ortp/logging.h"
#include "bcunit/Util.h"


using namespace linphone_tester_native;
using namespace Platform;

#define MAX_TRACE_SIZE		512
#define MAX_SUITE_NAME_SIZE	128

static OutputTraceListener^ sTraceListener;

static void nativeOutputTraceHandler(OutputTraceLevel lev, const char *fmt, va_list args)
{
	if (sTraceListener) {
		wchar_t wstr[MAX_TRACE_SIZE];
		std::string str;
		str.resize(MAX_TRACE_SIZE);
		vsnprintf((char *)str.c_str(), MAX_TRACE_SIZE, fmt, args);
		mbstowcs(wstr, str.c_str(), sizeof(wstr));
		String^ msg = ref new String(wstr);
		sTraceListener->outputTrace(lev, msg);
	}
}

static void LinphoneNativeGenericOutputTraceHandler(OrtpLogLevel lev, const char *fmt, va_list args)
{
	OutputTraceLevel level = Message;
	char fmt2[MAX_TRACE_SIZE];
	snprintf(fmt2, MAX_TRACE_SIZE, "%s\n", fmt);
	if (lev == ORTP_DEBUG) level = Debug;
	else if (lev == ORTP_MESSAGE) level = Message;
	else if (lev == ORTP_TRACE) level = Message;
	else if (lev == ORTP_WARNING) level = Warning;
	else if (lev == ORTP_ERROR) level = Error;
	else if (lev == ORTP_FATAL) level = Error;
	nativeOutputTraceHandler(level, fmt2, args);
}

static void LinphoneNativeOutputTraceHandler(OrtpLogLevel lev, const char *fmt, va_list args)
{
	if (lev >= ORTP_WARNING) {
		LinphoneNativeGenericOutputTraceHandler(lev, fmt, args);
	}
}

static void LinphoneNativeVerboseOutputTraceHandler(OrtpLogLevel lev, const char *fmt, va_list args)
{
	LinphoneNativeGenericOutputTraceHandler(lev, fmt, args);
}

static void BCUnitNativeOutputTraceHandler(int lev, const char *fmt, va_list args)
{
	nativeOutputTraceHandler(Raw, fmt, args);
}

LinphoneTesterNative::LinphoneTesterNative()
{
	liblinphone_tester_init();
}

LinphoneTesterNative::~LinphoneTesterNative()
{
	liblinphone_tester_uninit();
}

void LinphoneTesterNative::setOutputTraceListener(OutputTraceListener^ traceListener)
{
	sTraceListener = traceListener;
}

void LinphoneTesterNative::run(Platform::String^ suiteName, Platform::String^ caseName, Platform::Boolean verbose)
{
	std::wstring all(L"ALL");
	std::wstring wssuitename = suiteName->Data();
	std::wstring wscasename = caseName->Data();
	char csuitename[MAX_SUITE_NAME_SIZE] = { 0 };
	char ccasename[MAX_SUITE_NAME_SIZE] = { 0 };
	wcstombs(csuitename, wssuitename.c_str(), sizeof(csuitename));
	wcstombs(ccasename, wscasename.c_str(), sizeof(ccasename));

	if (verbose) {
		linphone_core_enable_logs_with_cb(LinphoneNativeVerboseOutputTraceHandler);
	} else {
		linphone_core_enable_logs_with_cb(LinphoneNativeOutputTraceHandler);
	}
	CU_set_trace_handler(BCUnitNativeOutputTraceHandler);

	liblinphone_tester_run_tests(wssuitename == all ? 0 : csuitename, wscasename == all ? 0 : ccasename);
}

unsigned int LinphoneTesterNative::nbTestSuites()
{
	return liblinphone_tester_nb_test_suites();
}

unsigned int LinphoneTesterNative::nbTests(Platform::String^ suiteName)
{
	std::wstring suitename = suiteName->Data();
	char cname[MAX_SUITE_NAME_SIZE] = { 0 };
	wcstombs(cname, suitename.c_str(), sizeof(cname));
	return liblinphone_tester_nb_tests(cname);
}

Platform::String^ LinphoneTesterNative::testSuiteName(int index)
{
	const char *cname = liblinphone_tester_test_suite_name(index);
	wchar_t wcname[MAX_SUITE_NAME_SIZE];
	mbstowcs(wcname, cname, sizeof(wcname));
	return ref new String(wcname);
}

Platform::String^ LinphoneTesterNative::testName(Platform::String^ suiteName, int testIndex)
{
	std::wstring suitename = suiteName->Data();
	char csuitename[MAX_SUITE_NAME_SIZE] = { 0 };
	wcstombs(csuitename, suitename.c_str(), sizeof(csuitename));
	const char *cname = liblinphone_tester_test_name(csuitename, testIndex);
	wchar_t wcname[MAX_SUITE_NAME_SIZE];
	mbstowcs(wcname, cname, sizeof(wcname));
	return ref new String(wcname);
}

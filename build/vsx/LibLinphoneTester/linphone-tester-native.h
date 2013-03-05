#pragma once

#include "liblinphone_tester.h"

namespace linphone_tester_native
{
	public interface class OutputTraceListener
	{
	public:
		void outputTrace(Platform::String^ msg);
	};

    public ref class LinphoneTesterNative sealed
    {
    public:
        LinphoneTesterNative();
		virtual ~LinphoneTesterNative();
		void setOutputTraceListener(OutputTraceListener^ traceListener);
		unsigned int nbTestSuites();
		unsigned int nbTests(Platform::String^ suiteName);
		Platform::String^ testSuiteName(int index);
		Platform::String^ testName(Platform::String^ suiteName, int testIndex);
		void run(Platform::String^ suiteName, Platform::String^ caseName, Platform::Boolean verbose);
    };
}
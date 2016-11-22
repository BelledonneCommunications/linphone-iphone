#pragma once

#include "linphone/core.h"
#include "liblinphone_tester.h"

namespace BelledonneCommunications
{
	namespace Linphone
	{
		namespace Tester
		{
			public interface class OutputTraceListener
			{
			public:
				void outputTrace(Platform::String^ lev, Platform::String^ msg);
			};

			public ref class NativeTester sealed
			{
			public:
				void setOutputTraceListener(OutputTraceListener^ traceListener);
				unsigned int nbTestSuites();
				unsigned int nbTests(Platform::String^ suiteName);
				Platform::String^ testSuiteName(int index);
				Platform::String^ testName(Platform::String^ suiteName, int testIndex);
				void initialize(Windows::Storage::StorageFolder^ writableDirectory, Platform::Boolean ui);
				bool run(Platform::String^ suiteName, Platform::String^ caseName, Platform::Boolean verbose);
				void runAllToXml();

				static property NativeTester^ Instance
				{
					NativeTester^ get() { return _instance; }
				}
				property Windows::Foundation::IAsyncAction^ AsyncAction
				{
					Windows::Foundation::IAsyncAction^ get() { return _asyncAction; }
				}
			private:
				NativeTester();
				~NativeTester();

				static NativeTester^ _instance;
				Windows::Foundation::IAsyncAction^ _asyncAction;
			};
		}
	}
}
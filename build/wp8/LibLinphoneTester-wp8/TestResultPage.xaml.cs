using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Navigation;
using Microsoft.Phone.Controls;
using Microsoft.Phone.Shell;
using System.Threading.Tasks;
using linphone_tester_native;

namespace LibLinphoneTester_wp8
{
    public delegate void OutputDisplayDelegate(int level, String msg);

    public partial class TestResultPage : PhoneApplicationPage
    {
        public TestResultPage()
        {
            InitializeComponent();
            Browser.Navigate(new Uri("log.html", UriKind.Relative));
        }

        private void Browser_LoadCompleted(object sender, NavigationEventArgs e)
        {
            string suiteName = NavigationContext.QueryString["SuiteName"];
            string caseName;
            if (NavigationContext.QueryString.ContainsKey("CaseName"))
            {
                caseName = NavigationContext.QueryString["CaseName"];
            }
            else
            {
                caseName = "ALL";
            }
            bool verbose = Convert.ToBoolean(NavigationContext.QueryString["Verbose"]);
            var app = (Application.Current as App);
            app.suite = new UnitTestSuite(suiteName, caseName, verbose, new OutputDisplayDelegate(OutputDisplay));
            app.suite.run();
        }

        public void OutputDisplay(int level, String msg)
        {
            this.Dispatcher.BeginInvoke(() =>
            {
                msg = msg.Replace("\r\n", "\n");
                string[] lines = msg.Split('\n');
                bool insertNewLine = false;
                foreach (string line in lines)
                {
                    if (line.Length == 0)
                    {
                        insertNewLine = false;
                        Browser.InvokeScript("append_nl");
                    }
                    else
                    {
                        if (insertNewLine == true)
                        {
                            Browser.InvokeScript("append_nl");
                        }
                        if (level == 0)
                        {
                            Browser.InvokeScript("append_trace", line, "debug");
                        }
                        else if (level == 1)
                        {
                            Browser.InvokeScript("append_trace", line, "message");
                        }
                        else if (level == 2)
                        {
                            Browser.InvokeScript("append_trace", line, "warning");
                        }
                        else if (level == 3)
                        {
                            Browser.InvokeScript("append_trace", line, "error");
                        }
                        else
                        {
                            Browser.InvokeScript("append_text", line);
                        }
                        insertNewLine = true;
                    }
                }
            });
        }
    }

    public class UnitTestSuite : OutputTraceListener
    {
        public UnitTestSuite(string SuiteName, string CaseName, bool Verbose, OutputDisplayDelegate OutputDisplay)
        {
            this.SuiteName = SuiteName;
            this.CaseName = CaseName;
            this.Verbose = Verbose;
            this.Running = false;
            this.OutputDisplay = OutputDisplay;
        }

        async public void run()
        {
            Running = true;
            var tup = new Tuple<string, string, bool>(SuiteName, CaseName, Verbose);
            var t = Task.Factory.StartNew((object parameters) =>
            {
                var tester = (Application.Current as App).tester;
                tester.setOutputTraceListener(this);
                var p = parameters as Tuple<string, string, bool>;
                tester.run(p.Item1, p.Item2, p.Item3);
            }, tup);
            await t;
            Running = false;
        }

        public void outputTrace(int level, String msg)
        {
            if (OutputDisplay != null)
            {
                OutputDisplay(level, msg);
            }
            System.Diagnostics.Debug.WriteLine(msg);
        }

        public bool running {
            get { return Running; }
            protected set { Running = value; }
        }

        private string SuiteName;
        private string CaseName;
        private bool Verbose;
        private bool Running;
        private OutputDisplayDelegate OutputDisplay;
    }
}
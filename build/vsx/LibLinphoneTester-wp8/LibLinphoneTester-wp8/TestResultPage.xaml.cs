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
    public delegate void OutputDisplayDelegate(String msg);

    public partial class TestResultPage : PhoneApplicationPage
    {
        public TestResultPage()
        {
            InitializeComponent();
        }
        
        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            base.OnNavigatedTo(e);
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

        public void OutputDisplay(String msg)
        {
            this.Dispatcher.BeginInvoke(() =>
                {
                    TestResults.Text += msg;
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

        public void outputTrace(String msg)
        {
            if (OutputDisplay != null)
            {
                OutputDisplay(msg);
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
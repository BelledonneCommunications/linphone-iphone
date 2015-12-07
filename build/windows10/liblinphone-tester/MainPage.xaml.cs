using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;
using liblinphone_tester.DataModel;
using liblinphone_tester_runtime_component;
using System.Threading.Tasks;
using Windows.UI.Core;
using Windows.UI.Xaml.Documents;
using Windows.Storage;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace liblinphone_tester
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page, OutputTraceListener
    {
        public MainPage()
        {
            this.InitializeComponent();           
        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            base.OnNavigatedTo(e);
            if ((e.Parameter is Uri) && (e.Parameter.ToString().Equals("liblinphone-tester:autolaunch")))
            {
                AutoLaunch();
            }
            else
            {
                LibLinphoneTester.Instance.initialize(ApplicationData.Current.LocalFolder, true);
                _suites = UnitTestDataSource.GetSuites(LibLinphoneTester.Instance);
            }
        }

        public IEnumerable<UnitTestSuite> Suites
        {
            get { return _suites; }
        }

        private IEnumerable<UnitTestSuite> _suites;

        private void SelectAll_Click(object sender, RoutedEventArgs e)
        {
            bool allSelected = Suites.All(x => x.Selected);
            foreach (UnitTestSuite suite in Suites)
            {
                suite.Selected = !allSelected;
            }
        }

        private void RunSelected_Click(object sender, RoutedEventArgs e)
        {
            int nbCases = 0;
            foreach (UnitTestSuite suite in Suites)
            {
                foreach (UnitTestCase c in suite.Cases)
                {
                    if (c.Selected) nbCases++;
                }
            }
            if (nbCases == 0) return;

            PrepareRun(nbCases);

            var tup = new Tuple<IEnumerable<UnitTestSuite>, bool?>(Suites, Verbose.IsChecked);
            var t = Task.Factory.StartNew(async (object parameters) =>
            {
                var p = parameters as Tuple<IEnumerable<UnitTestSuite>, bool?>;
                IEnumerable<UnitTestSuite> suites = p.Item1;
                bool verbose = p.Item2 != null ? (bool)p.Item2 : false;
                foreach (UnitTestSuite suite in suites)
                {
                    foreach (UnitTestCase c in suite.Cases)
                    {
                        if (c.Selected)
                        {
                            await RunUnitTestCase(c, verbose);
                        }
                    }
                }
            }, tup);
        }

        private void RunSingle_Click(object sender, RoutedEventArgs e)
        {
            PrepareRun(1);

            var tup = new Tuple<UnitTestCase, bool?>(DisplayedTestCase, Verbose.IsChecked);
            var t = Task.Factory.StartNew(async (object parameters) =>
            {
                var p = parameters as Tuple<UnitTestCase, bool?>;
                UnitTestCase c = p.Item1;
                bool verbose = p.Item2 != null ? (bool)p.Item2 : false;
                await RunUnitTestCase(c, verbose);
            }, tup);
        }

        private void PrepareRun(int nbCases)
        {
            CommandBar.IsEnabled = false;
            ProgressIndicator.IsEnabled = true;
            ProgressIndicator.Minimum = 0;
            ProgressIndicator.Maximum = nbCases;
            ProgressIndicator.Value = 0;
            LibLinphoneTester.Instance.setOutputTraceListener(this);
        }

        private async Task RunUnitTestCase(UnitTestCase c, bool verbose)
        {
            UnitTestCaseState newState = UnitTestCaseState.NotRun;
            await Dispatcher.RunAsync(CoreDispatcherPriority.Normal, () =>
            {
                RunningTestCase = c;
            });
            await Dispatcher.RunAsync(CoreDispatcherPriority.Normal, () =>
            {
                c.Traces.Clear();
            });
            c.Dispatcher = Dispatcher;
            if (LibLinphoneTester.Instance.run(c.Suite.Name, c.Name, verbose))
            {
                newState = UnitTestCaseState.Failure;
            }
            else
            {
                newState = UnitTestCaseState.Success;
            }
            await Dispatcher.RunAsync(CoreDispatcherPriority.Normal, () =>
            {
                c.State = newState;
                ProgressIndicator.Value += 1;
                if (ProgressIndicator.Value == ProgressIndicator.Maximum)
                {
                    UnprepareRun();
                }
            });
        }

        private void UnprepareRun()
        {
            LibLinphoneTester.Instance.setOutputTraceListener(null);
            RunningTestCase = null;
            ProgressIndicator.IsEnabled = false;
            CommandBar.IsEnabled = true;
        }

        private void UnitTestCase_Click(object sender, ItemClickEventArgs e)
        {
            DisplayedTestCase = (e.ClickedItem as UnitTestCase);
            TestResultPage.DataContext = DisplayedTestCase;
            TestResultState.Visibility = Visibility.Visible;
            TestResultRun.Visibility = Visibility.Visible;
        }

        public async void outputTrace(String lev, String msg)
        {
            await Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, () =>
            {
                if (RunningTestCase != null)
                {
                    RunningTestCase.Traces.Add(new OutputTrace(lev, msg));
                }
            });
        }

        private void AutoLaunch()
        {
            CommandBar.IsEnabled = false;
            ProgressIndicator.IsIndeterminate = true;
            ProgressIndicator.IsEnabled = true;
            LibLinphoneTester.Instance.initialize(ApplicationData.Current.LocalFolder, false);
            LibLinphoneTester.Instance.runAllToXml();
            if (LibLinphoneTester.Instance.AsyncAction != null)
            {
                LibLinphoneTester.Instance.AsyncAction.Completed += (asyncInfo, asyncStatus) => {
                    App.Current.Exit();
                };
            }
        }

        private UnitTestCase RunningTestCase;
        private UnitTestCase DisplayedTestCase;
    }
}

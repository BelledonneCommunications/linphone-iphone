using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Navigation;
using Microsoft.Phone.Controls;
using Microsoft.Phone.Shell;

namespace LibLinphoneTester_wp8
{
    public partial class MainPage : PhoneApplicationPage
    {
        public MainPage()
        {
            InitializeComponent();

            var tester = (Application.Current as App).tester;
            List<UnitTestSuiteName> source = new List<UnitTestSuiteName>();
            source.Add(new UnitTestSuiteName("ALL"));
            for (int i = 0; i < tester.nbTestSuites(); i++)
            {
                source.Add(new UnitTestSuiteName(tester.testSuiteName(i)));
            }

            Tests.ItemsSource = source;
        }

        private void Tests_Tap(object sender, System.Windows.Input.GestureEventArgs e)
        {
            UnitTestSuiteName test = (sender as LongListSelector).SelectedItem as UnitTestSuiteName;
            if (test == null) return;
            if (test.Name == "ALL")
            {
                NavigationService.Navigate(new Uri("/TestResultPage.xaml?SuiteName=" + test.Name + "&Verbose=" + Verbose.IsChecked.GetValueOrDefault(), UriKind.Relative));
            }
            else
            {
                NavigationService.Navigate(new Uri("/TestCasePage.xaml?SuiteName=" + test.Name + "&Verbose=" + Verbose.IsChecked.GetValueOrDefault(), UriKind.Relative));
            }
        }
    }

    public class UnitTestSuiteName
    {
        public string Name
        {
            get;
            set;
        }

        public UnitTestSuiteName(string name)
        {
            this.Name = name;
        }
    }
}
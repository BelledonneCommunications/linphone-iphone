using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using liblinphone_tester_runtime_component;
using System.Collections.ObjectModel;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Media;
using System.ComponentModel;
using Windows.UI;
using Windows.UI.Xaml.Documents;
using Windows.UI.Core;

namespace liblinphone_tester.DataModel
{
    public class OutputTrace
    {
        public OutputTrace(String lev, String msg)
        {
            Level = lev;
            Msg = msg;
        }

        public String Level { get; private set; }
        public String Msg { get; private set; }
    }

    public class UnitTestSuite
    {
        public UnitTestSuite(string name)
        {
            Name = name;
            Cases = new ObservableCollection<UnitTestCase>();
            Selected = false;
        }

        public string Name { get; private set; }
        public bool Selected
        {
            get { return Cases.All(x => x.Selected); }
            set
            {
                foreach (UnitTestCase c in Cases)
                {
                    c.Selected = value;
                }
            }
        }
        public ObservableCollection<UnitTestCase> Cases { get; private set; }
    }

    public enum UnitTestCaseState
    {
        NotRun,
        Success,
        Failure
    }
    
    public class UnitTestCase : INotifyPropertyChanged
    {
        public UnitTestCase(UnitTestSuite suite, string name)
        {
            _suite = new WeakReference(suite);
            Name = name;
            Selected = false;
            State = UnitTestCaseState.NotRun;
            Traces = new ObservableCollection<OutputTrace>();
        }

        public UnitTestSuite Suite
        {
            get { return _suite.Target as UnitTestSuite; }
        }
        public string Name { get; private set; }
        public bool Selected
        {
            get { return _selected; }
            set
            {
                _selected = value;
                RaisePropertyChanged("Selected");
            }
        }
        public UnitTestCaseState State
        {
            get { return _state; }
            set
            {
                _state = value;
                RaisePropertyChanged("State");
            }
        }
        public ObservableCollection<OutputTrace> Traces
        {
            get { return _traces; }
            set
            {
                _traces = value;
                RaisePropertyChanged("Traces");
            }
        }
        public CoreDispatcher Dispatcher { get; set; }

        public event PropertyChangedEventHandler PropertyChanged;

        protected void RaisePropertyChanged(string name)
        {
            if (PropertyChanged != null)
            {
                PropertyChanged(this, new PropertyChangedEventArgs(name));
            }
        }

        private WeakReference _suite;
        private bool _selected;
        private UnitTestCaseState _state;
        private ObservableCollection<OutputTrace> _traces;
    }

    public sealed class UnitTestDataSource
    {
        private static UnitTestDataSource _unitTestDataSource = new UnitTestDataSource();
        private ObservableCollection<UnitTestSuite> _suites = new ObservableCollection<UnitTestSuite>();

        public ObservableCollection<UnitTestSuite> Suites
        {
            get { return this._suites; }
        }

        public static IEnumerable<UnitTestSuite> GetSuites(LibLinphoneTester tester)
        {
            return _unitTestDataSource.FillSuites(tester);
        }

        private IEnumerable<UnitTestSuite> FillSuites(LibLinphoneTester tester)
        {
            if (this.Suites.Count != 0) return this.Suites;
            for (int i = 0; i < tester.nbTestSuites(); i++)
            {
                UnitTestSuite suite = new UnitTestSuite(tester.testSuiteName(i));
                for (int j = 0; j < tester.nbTests(suite.Name); j++)
                {
                    suite.Cases.Add(new UnitTestCase(suite, tester.testName(suite.Name, j)));
                }
                this.Suites.Add(suite);
            }
            return this.Suites;
        }
    }

    public sealed class UnitTestCaseStateToSymbolConverter : IValueConverter
    {
        object IValueConverter.Convert(object value, Type targetType, object parametr, string language)
        {
            if (!value.GetType().Equals(typeof(UnitTestCaseState)))
            {
                throw new ArgumentException("Only UnitTestCaseState is supported");
            }
            if (targetType.Equals(typeof(Symbol)))
            {
                switch ((UnitTestCaseState)value)
                {
                    case UnitTestCaseState.Success:
                        return Symbol.Like;
                    case UnitTestCaseState.Failure:
                        return Symbol.Dislike;
                    case UnitTestCaseState.NotRun:
                    default:
                        return Symbol.Help;
                }
            }
            else
            {
                throw new ArgumentException(string.Format("Unsupported type {0}", targetType.FullName));
            }
        }

        object IValueConverter.ConvertBack(object value, Type targetType, object parameter, string language)
        {
            throw new NotImplementedException();
        }
    }

    public sealed class UnitTestCaseStateToSymbolColorConverter : IValueConverter
    {
        object IValueConverter.Convert(object value, Type targetType, object parameter, string language)
        {
            if (!value.GetType().Equals(typeof(UnitTestCaseState)))
            {
                throw new ArgumentException("Only UnitTestCaseState is supported");
            }
            if (targetType.Equals(typeof(Brush)))
            {
                switch ((UnitTestCaseState)value)
                {
                    case UnitTestCaseState.Success:
                        return new SolidColorBrush(Colors.ForestGreen);
                    case UnitTestCaseState.Failure:
                        return new SolidColorBrush(Colors.IndianRed);
                    case UnitTestCaseState.NotRun:
                    default:
                        return new SolidColorBrush(Colors.LightGray);
                }
            }
            else
            {
                throw new ArgumentException(string.Format("Unsupported format {0}", targetType.FullName));
            }
        }

        object IValueConverter.ConvertBack(object value, Type targetType, object parameter, string language)
        {
            throw new NotImplementedException();
        }
    }

    public sealed class OutputTraceLevelToColorConverter : IValueConverter
    {
        object IValueConverter.Convert(object value, Type targetType, object parameter, string language)
        {
            if (!value.GetType().Equals(typeof(String)))
            {
                throw new ArgumentException("Only String is supported");
            }
            if (targetType.Equals(typeof(Brush)))
            {
                if ((String)value == "Error")
                {
                    return new SolidColorBrush(Colors.IndianRed);
                }
                else if ((String)value == "Warning")
                {
                    return new SolidColorBrush(Colors.Orange);
                }
                return new SolidColorBrush(Colors.Black);
            }
            else
            {
                throw new ArgumentException(string.Format("Unsupported format {0}", targetType.FullName));
            }
        }

        object IValueConverter.ConvertBack(object value, Type targetType, object parameter, string language)
        {
            throw new NotImplementedException();
        }
    }
}

﻿#pragma checksum "C:\Demos\22_Silverlight\MusicBrainzSolution\MusicBrainzSilverlightApplication\MainPage.xaml" "{406ea660-64cf-4c82-b6f0-42d48172a799}" "4BB5E2D422FEBF40B3C83A0A2F96B28C"
//------------------------------------------------------------------------------
// <auto-generated>
//     This code was generated by a tool.
//     Runtime Version:4.0.30319.1
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
// </auto-generated>
//------------------------------------------------------------------------------

using System;
using System.Windows;
using System.Windows.Automation;
using System.Windows.Automation.Peers;
using System.Windows.Automation.Provider;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Ink;
using System.Windows.Input;
using System.Windows.Interop;
using System.Windows.Markup;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Media.Imaging;
using System.Windows.Resources;
using System.Windows.Shapes;
using System.Windows.Threading;


namespace MusicBrainzSilverlightApplication {
    
    
    public partial class MainPage : System.Windows.Controls.UserControl {
        
        internal System.Windows.Controls.Grid LayoutRoot;
        
        internal System.Windows.Controls.TextBox Keyword;
        
        internal System.Windows.Controls.Button Search;
        
        internal System.Windows.Controls.Button Save;
        
        internal System.Windows.Controls.DataGrid ItemGrid;
        
        internal System.Windows.Controls.TextBox Messages;
        
        private bool _contentLoaded;
        
        /// <summary>
        /// InitializeComponent
        /// </summary>
        [System.Diagnostics.DebuggerNonUserCodeAttribute()]
        public void InitializeComponent() {
            if (_contentLoaded) {
                return;
            }
            _contentLoaded = true;
            System.Windows.Application.LoadComponent(this, new System.Uri("/MusicBrainzSilverlightApplication;component/MainPage.xaml", System.UriKind.Relative));
            this.LayoutRoot = ((System.Windows.Controls.Grid)(this.FindName("LayoutRoot")));
            this.Keyword = ((System.Windows.Controls.TextBox)(this.FindName("Keyword")));
            this.Search = ((System.Windows.Controls.Button)(this.FindName("Search")));
            this.Save = ((System.Windows.Controls.Button)(this.FindName("Save")));
            this.ItemGrid = ((System.Windows.Controls.DataGrid)(this.FindName("ItemGrid")));
            this.Messages = ((System.Windows.Controls.TextBox)(this.FindName("Messages")));
        }
    }
}


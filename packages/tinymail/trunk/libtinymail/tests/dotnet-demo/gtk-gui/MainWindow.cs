// ------------------------------------------------------------------------------
//  <autogenerated>
//      This code was generated by a tool.
//      Mono Runtime Version: 2.0.50727.42
// 
//      Changes to this file may cause incorrect behavior and will be lost if 
//      the code is regenerated.
//  </autogenerated>
// ------------------------------------------------------------------------------



public partial class MainWindow {
    
    private Gtk.VBox vbox1;
    
    private Gtk.HPaned hpaned1;
    
    private Gtk.ScrolledWindow scrolledwindow2;
    
    private Gtk.TreeView folders_treeview;
    
    private Gtk.VPaned vpaned1;
    
    private Gtk.ScrolledWindow scrolledwindow1;
    
    private Gtk.TreeView headers_treeview;
    
    private Gtk.ScrolledWindow msg_scrolledwindow;
    
    private Gtk.HBox hbox1;
    
    private Gtk.ProgressBar progressbar;
    
    private Gtk.Button connect_button;
    
    protected virtual void Build() {
        Stetic.Gui.Initialize();
        // Widget MainWindow
        this.Name = "MainWindow";
        this.Title = Mono.Unix.Catalog.GetString("MainWindow");
        this.WindowPosition = ((Gtk.WindowPosition)(4));
        // Container child MainWindow.Gtk.Container+ContainerChild
        this.vbox1 = new Gtk.VBox();
        this.vbox1.Name = "vbox1";
        this.vbox1.Spacing = 6;
        // Container child vbox1.Gtk.Box+BoxChild
        this.hpaned1 = new Gtk.HPaned();
        this.hpaned1.CanFocus = true;
        this.hpaned1.Name = "hpaned1";
        this.hpaned1.Position = 85;
        // Container child hpaned1.Gtk.Paned+PanedChild
        this.scrolledwindow2 = new Gtk.ScrolledWindow();
        this.scrolledwindow2.CanFocus = true;
        this.scrolledwindow2.Name = "scrolledwindow2";
        this.scrolledwindow2.VscrollbarPolicy = ((Gtk.PolicyType)(1));
        this.scrolledwindow2.HscrollbarPolicy = ((Gtk.PolicyType)(1));
        this.scrolledwindow2.ShadowType = ((Gtk.ShadowType)(1));
        // Container child scrolledwindow2.Gtk.Container+ContainerChild
        this.folders_treeview = new Gtk.TreeView();
        this.folders_treeview.CanFocus = true;
        this.folders_treeview.Name = "folders_treeview";
        this.scrolledwindow2.Add(this.folders_treeview);
        this.hpaned1.Add(this.scrolledwindow2);
        Gtk.Paned.PanedChild w2 = ((Gtk.Paned.PanedChild)(this.hpaned1[this.scrolledwindow2]));
        w2.Resize = false;
        // Container child hpaned1.Gtk.Paned+PanedChild
        this.vpaned1 = new Gtk.VPaned();
        this.vpaned1.CanFocus = true;
        this.vpaned1.Name = "vpaned1";
        this.vpaned1.Position = 85;
        // Container child vpaned1.Gtk.Paned+PanedChild
        this.scrolledwindow1 = new Gtk.ScrolledWindow();
        this.scrolledwindow1.CanFocus = true;
        this.scrolledwindow1.Name = "scrolledwindow1";
        this.scrolledwindow1.ShadowType = ((Gtk.ShadowType)(1));
        // Container child scrolledwindow1.Gtk.Container+ContainerChild
        this.headers_treeview = new Gtk.TreeView();
        this.headers_treeview.CanFocus = true;
        this.headers_treeview.Name = "headers_treeview";
        this.scrolledwindow1.Add(this.headers_treeview);
        this.vpaned1.Add(this.scrolledwindow1);
        Gtk.Paned.PanedChild w4 = ((Gtk.Paned.PanedChild)(this.vpaned1[this.scrolledwindow1]));
        w4.Resize = false;
        // Container child vpaned1.Gtk.Paned+PanedChild
        this.msg_scrolledwindow = new Gtk.ScrolledWindow();
        this.msg_scrolledwindow.CanFocus = true;
        this.msg_scrolledwindow.Name = "msg_scrolledwindow";
        this.msg_scrolledwindow.VscrollbarPolicy = ((Gtk.PolicyType)(1));
        this.msg_scrolledwindow.HscrollbarPolicy = ((Gtk.PolicyType)(1));
        this.msg_scrolledwindow.ShadowType = ((Gtk.ShadowType)(1));
        this.vpaned1.Add(this.msg_scrolledwindow);
        this.hpaned1.Add(this.vpaned1);
        this.vbox1.Add(this.hpaned1);
        Gtk.Box.BoxChild w7 = ((Gtk.Box.BoxChild)(this.vbox1[this.hpaned1]));
        w7.Position = 0;
        // Container child vbox1.Gtk.Box+BoxChild
        this.hbox1 = new Gtk.HBox();
        this.hbox1.Name = "hbox1";
        this.hbox1.Spacing = 6;
        // Container child hbox1.Gtk.Box+BoxChild
        this.progressbar = new Gtk.ProgressBar();
        this.progressbar.Name = "progressbar";
        this.hbox1.Add(this.progressbar);
        Gtk.Box.BoxChild w8 = ((Gtk.Box.BoxChild)(this.hbox1[this.progressbar]));
        w8.Position = 0;
        // Container child hbox1.Gtk.Box+BoxChild
        this.connect_button = new Gtk.Button();
        this.connect_button.CanFocus = true;
        this.connect_button.Name = "connect_button";
        this.connect_button.UseUnderline = true;
        this.connect_button.Label = Mono.Unix.Catalog.GetString("button1");
        this.hbox1.Add(this.connect_button);
        Gtk.Box.BoxChild w9 = ((Gtk.Box.BoxChild)(this.hbox1[this.connect_button]));
        w9.Position = 1;
        w9.Expand = false;
        w9.Fill = false;
        this.vbox1.Add(this.hbox1);
        Gtk.Box.BoxChild w10 = ((Gtk.Box.BoxChild)(this.vbox1[this.hbox1]));
        w10.Position = 1;
        w10.Expand = false;
        w10.Fill = false;
        this.Add(this.vbox1);
        if ((this.Child != null)) {
            this.Child.ShowAll();
        }
        this.DefaultWidth = 530;
        this.DefaultHeight = 452;
        this.Show();
        this.DeleteEvent += new Gtk.DeleteEventHandler(this.OnDeleteEvent);
        this.connect_button.Clicked += new System.EventHandler(this.OnConnectButtonClicked);
    }
}
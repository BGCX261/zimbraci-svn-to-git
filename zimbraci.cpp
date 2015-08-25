/*
 *      zimbraci.cpp
 *      
 *      Copyright 2009 ZAGLI <i.zagli@hho.edu.tr>
 *      
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *      
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *      
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

using namespace std;

#include <libnotify/notify.h>
#include <libgnome/libgnome.h>
#include <libglade-2.0/glade/glade.h>
#include <gconf/2/gconf/gconf.h>
#include <libsoup/soup.h>
#include <iostream>
#include <string>
#include <time.h>
#include <glib/gi18n.h>

//~ #define __debug__

GtkStatusIcon *si;
GtkWidget *label, *imaj; 
NotifyNotification *uyari;
//~ GTimer *timer;
static long interval = 60;
static long oldinterval = -1;
static gboolean prefs_shown=false;
static guint32 timeout_id = -1;
time_t lastrun;

static void Log(const char *logstr) {
    
	//~ time_t rawtime;
	//~ struct tm * timeinfo;
	//~ char buffer [80];
	//~ time ( &rawtime );
	//~ timeinfo = localtime ( &rawtime );
	//~ strftime (buffer,80,"%H:%M - ",timeinfo);
    //~ cout << buffer << logstr << "\n";
}

static string num2str( int num) { 
    char str[100]; sprintf( str, "%d", num); 
    return string(str);
}
//~ static string num2str( size_t num) { 
    //~ return num2str((int)num);
//~ }
//~ static string num2str(long int *num) { 
    //~ char str[100]; sprintf( str, "%l", *num); 
    //~ return string(str);
//~ }
//~ static string num2str(double num) { 
    //~ char str[10]; sprintf( str, "%.0f", num); 
    //~ return string(str);
//~ }

static string get_pref_string ( const char *pref_key, gchar *def_val=(gchar *)"") {
	GConfEngine *gce = gconf_engine_get_default();
	string pref_val("");
	pref_val = string(gconf_engine_get_string(gce,(string("/zimbraci/")+pref_key).c_str(),NULL));
	if (pref_val.empty()) return string(def_val);//(gchar *)"N/A";
	return pref_val;
}
static gboolean get_pref_bool ( const char *pref_key) {
	GConfEngine *gce = gconf_engine_get_default();
	gboolean pref_val;
	pref_val = gconf_engine_get_bool(gce,(string("/zimbraci/")+pref_key).c_str(),NULL);
	gconf_engine_unref(gce);
	return pref_val;
}

static void display_notification( const gchar *metin=NULL, const gchar *resim=NULL, gboolean force=false) {

    Log("Notification display");
    Log(metin?metin:"");
	gtk_status_icon_set_tooltip_text(si, (string("Zimbraci:\n")+metin).c_str());
	if (get_pref_bool("din") || force) {
		notify_notification_update(uyari,"Zimbraci",metin?metin:NULL,resim?resim:NULL);
		notify_notification_show(uyari,NULL);
	}
}

// --- crop_between string function
static std::string crop_between(std::string str, std::string str1, std::string str2, size_t pos=0) {

    Log("Cropping");
    return str.substr(str.find(str1,pos)+str1.length(),str.find(str2,pos)-(str.find(str1,pos)+str1.length()));

}

//~ static gboolean check_timeout(); // implementation ahead

	//~ interval = atoi(get_pref_string("tmo").c_str())*60;
	//~ if (interval != oldinterval) {
		//~ oldinterval = interval;
		//~ set_zimbraci_timeout();
	//~ }

static void query_complete(SoupSession *session, SoupMessage *mesg, gpointer user_data) {
    
    Log("Query Complete");
    string  msg(crop_between(string(mesg->response_body->data), "<soap:Envelope ", "</soap:Envelope>")),
            mesaj("");
    int count=0;
    static int oldcount=-1;

#ifdef __debug__
    cout << "Cevap :" << msg.c_str() << "\n";
#endif
    
    if (!msg.empty()) {
    
        int i=0, pi;
        std::string su, e, c, isim;
        while((i=msg.find("<m ", i))>0) {
            count++; if (count>10) break;
            c = msg.substr(i, msg.find("</m>",i));
            su = crop_between(c,"<su>", "</su>");
            e = crop_between(c,"<e ","/>");
            pi = e.find("p=\"")<0 ? (e.find("a=\"")<0?e.find("d=\""):0):e.find("p=\"");
            isim = e.substr(pi+3,e.find("\"",pi+3)-(pi+3));
            mesaj +=(!mesaj.empty()?"\n* ":"* ")+isim+": "+su;
            i+=3;
        }
        
        std::string imgsuffix=string(!count?_("NONE"):num2str(count>10?10:count)) + string(count>10?"+":"");
        //~ if (count) 
            gtk_status_icon_set_from_file( si, string("./mail_"+imgsuffix+".png").c_str()); //"/usr/share/icons/crystalsvg/22x22/actions/mail_"
        //~ else
            //~ gtk_status_icon_set_from_file( si, "/usr/share/icons/crystalsvg/22x22/actions/mail_generic.png");
        //~ msg = _("There " + string(count>1?"are":"is")) + string(count>10?_(" more than "):" ") + string(!count?"no":num2str(count>10?10:count)) + _(" unread message"+string(count>1?"s":"")+".");
        msg = _("Messages found: ") + imgsuffix;
    }
    else msg = _("Possible error, nothing returned by server");

	display_notification( msg.c_str(),NULL,count!=oldcount);
	oldcount=count;

	time_t rawtime;
	struct tm * timeinfo;
	char buffer [80];
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	strftime (buffer,80,"%H:%M - ",timeinfo);

    gtk_status_icon_set_tooltip_text(si, (" Zimbraci:\n"+string(buffer)+msg+(count?"\n"+mesaj:"")).c_str());
    
}

static void auth_complete(SoupSession *session, SoupMessage *msg, gpointer user_data) {
    
    Log("Auth Complete");
    if (!msg->response_body->data) {
        Log("Conection / Auth failed.");
        time_t rawtime;
        struct tm * timeinfo;
        char buffer [80];
        time ( &rawtime );
        timeinfo = localtime ( &rawtime );
        strftime (buffer,80,"%H:%M - ",timeinfo);
        gtk_status_icon_set_tooltip_text(si, (" Zimbraci:\n"+string(buffer)+"Connection Failed.").c_str());
        gtk_status_icon_set_from_file( si, "./mail_FAIL.png");
        return;
    }
    string authToken = crop_between(msg->response_body->data, "<authToken>", "</authToken>");
    
	if (!authToken.empty()) {
        string mesg( "<soap:Envelope xmlns:soap=\"http://www.w3.org/2003/05/soap-envelope\"><soap:Header><context xmlns=\"urn:zimbra\"><authToken>" +
                    authToken +
                    "</authToken></context></soap:Header><soap:Body><SearchRequest xmlns=\"urn:zimbraMail\" types=\"message\"><query>"+
                    string(get_pref_string("qry",(gchar *)"is:unread AND under:(inbox OR junk)"))+"</query></SearchRequest></soap:Body></soap:Envelope>\n");

        SoupMessage *sm = soup_message_new("POST",("http://"+string(get_pref_string("url"))+"/service/soap").c_str());
        
        soup_message_set_request( sm, "application/soap+xml", SOUP_MEMORY_COPY, mesg.c_str(), mesg.length());
        SoupMessageHeaders *smh = sm->request_headers;//soup_message_headers_new(SOUP_MESSAGE_HEADERS_REQUEST);
        soup_message_headers_append( smh, "charset", "utf-8");
        soup_message_headers_append( smh, "Connection", "Keep-Alive");
        soup_message_headers_append( smh, "Cache-Control", "no-cache");
        soup_message_headers_append( smh, "Pragma", "no-cache");
        
        //~ SoupMessageBody *smb = sm->request_body; //soup_message_body_new(void);
        //~ soup_message_body_append( smb, SOUP_MEMORY_COPY, msg.c_str(), msg.length());

        SoupSession *ss = soup_session_async_new();
        
        //~ cout << sm->request_body->data << "\n";
        soup_session_queue_message( ss, sm, query_complete, NULL);
    }
}

static void set_zimbraci_timeout();

static void check_zimbra() {
    
    try {
    //~ if (lastrun)  cout << num2str(difftime(time(NULL),lastrun)) << "\n";
    lastrun = time(NULL);
    Log("Zimbra check");
	interval = atoi(get_pref_string("tmo").c_str())*60;
	if (interval != oldinterval) {
		oldinterval = interval;
		set_zimbraci_timeout();
	}

    string mesg( "<soap:Envelope xmlns:soap=\"http://www.w3.org/2003/05/soap-envelope\"><soap:Header><context xmlns=\"urn:zimbra\"><nonotify/><noqualify/></context></soap:Header><soap:Body><AuthRequest xmlns=\"urn:zimbraAccount\"><account>"+
				string(get_pref_string("usr")) + "</account><password>" +
                string(get_pref_string("psw")) + "</password></AuthRequest></soap:Body></soap:Envelope>"),
                host_url("/service/soap"),
                veri(mesg+"\n");

    SoupMessage *sm = soup_message_new("POST",("http://"+string(get_pref_string("url"))+"/service/soap").c_str());
    
    soup_message_set_request( sm, "application/soap+xml", SOUP_MEMORY_COPY, mesg.c_str(), mesg.length());
    SoupMessageHeaders *smh = sm->request_headers;//soup_message_headers_new(SOUP_MESSAGE_HEADERS_REQUEST);
    soup_message_headers_append( smh, "charset", "utf-8");
    soup_message_headers_append( smh, "Connection", "Keep-Alive");
    soup_message_headers_append( smh, "Cache-Control", "no-cache");
    soup_message_headers_append( smh, "Pragma", "no-cache");
    
    //~ SoupMessageBody *smb = sm->request_body; //soup_message_body_new(void);
    //~ soup_message_body_append( smb, SOUP_MEMORY_COPY, msg.c_str(), msg.length());

    SoupSession *ss = soup_session_async_new();
    
    //~ cout << sm->request_body->data << "\n";
        soup_session_queue_message( ss, sm, auth_complete, NULL);
    } catch(...) {cout << "Auth failed."; }
}

static void set_zimbraci_timeout() {
    Log("Setting timeout");
	if (timeout_id != (guint32)-1) g_source_remove(timeout_id);
	timeout_id = g_timeout_add_seconds( interval, (GtkFunction) check_zimbra, NULL);
	if (timeout_id==0) timeout_id = -1;
}

static void prefs_popup() {

    Log("Preferences");
    if (prefs_shown) return;
    
    GtkWidget *confdialog;
	gint result;
    
    if (!prefs_shown) {        
        
        prefs_shown = true;
    
        GladeXML *xml=glade_xml_new("./zimbraci-conf.glade", NULL, NULL);
        
        glade_xml_signal_autoconnect(xml);
                
        gtk_entry_set_text( GTK_ENTRY(glade_xml_get_widget(xml,"entry2")), get_pref_string("url").c_str()); 
        gtk_entry_set_text( GTK_ENTRY(glade_xml_get_widget(xml,"entry3")), get_pref_string("usr").c_str()); 
        gtk_entry_set_text( GTK_ENTRY(glade_xml_get_widget(xml,"entry4")), get_pref_string("psw").c_str()); 
        gtk_entry_set_text( GTK_ENTRY(glade_xml_get_widget(xml,"entryQ")), get_pref_string("qry",(gchar *)"is:unread AND under:(inbox OR junk)").c_str()); 
        gtk_entry_set_text( GTK_ENTRY(glade_xml_get_widget(xml,"entry5")), get_pref_string("tmo").c_str()); 
        gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml,"checkbutton1")), get_pref_bool("din")); 
        
        confdialog = glade_xml_get_widget(xml,"zimbraci-conf");
        
        result = gtk_dialog_run(GTK_DIALOG(confdialog));
		GConfEngine *gce = gconf_engine_get_default();
        switch (result) {
            case GTK_RESPONSE_OK:
				gconf_engine_set_string(gce,"/zimbraci/url", gtk_entry_get_text( GTK_ENTRY(glade_xml_get_widget(xml,"entry2"))),NULL); 
				gconf_engine_set_string(gce,"/zimbraci/usr", gtk_entry_get_text( GTK_ENTRY(glade_xml_get_widget(xml,"entry3"))),NULL); 
				gconf_engine_set_string(gce,"/zimbraci/psw", gtk_entry_get_text( GTK_ENTRY(glade_xml_get_widget(xml,"entry4"))),NULL); 
				gconf_engine_set_string(gce,"/zimbraci/tmo", gtk_entry_get_text( GTK_ENTRY(glade_xml_get_widget(xml,"entry5"))),NULL); 
				gconf_engine_set_string(gce,"/zimbraci/qry", gtk_entry_get_text( GTK_ENTRY(glade_xml_get_widget(xml,"entryQ"))),NULL); 
				gconf_engine_set_bool(gce,"/zimbraci/din", gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(glade_xml_get_widget(xml,"checkbutton1"))),NULL); 
				// check new interval, if already passed after last run then run it 5 secs later.
				interval = atoi(get_pref_string("tmo").c_str())*60;
				if (interval <= difftime(time(NULL),lastrun)) {
					interval=5; // run check_zimbraci after 5 seconds. it will auto adjust new interval
				}
				if (interval > oldinterval) {
                    // interval extended, so we need to adjust first run as we already spend some time
                    interval -= difftime(time(NULL),lastrun);
					set_zimbraci_timeout();
				}
                break;
            default:
                break;
         }
		gconf_engine_unref(gce);
     }

    gtk_widget_destroy (confdialog);

    prefs_shown = false;
	
}

//~ static gboolean check_timeout() {
    //~ 
    //~ if (g_timer_elapsed(timer,NULL) < (interval*.98)) {
        //~ 
        //~ Log("Check timeout failed");
        //~ display_notification( string(_("Should not run yet. (")+num2str((double)g_timer_elapsed(timer,NULL))+"/"+num2str(interval)+")").c_str(),NULL,true);
        //~ return TRUE;
    //~ }
    //~ 
    //~ Log("Check timeout passed");
    //~ g_timer_start(timer);
	//~ 
    //~ Log("Timer reset.");
	//~ check_zimbra();
    //~ return TRUE;
//~ }

//~ static void icon_activate(GtkStatusIcon *status_icon, gpointer user_data) {
    //~ 
    //~ check_zimbra();   
    //~ gtk_show_uri(NULL,("http://"+get_pref_string("url")).c_str(),GDK_CURRENT_TIME,NULL);
//~ }

static void prefs_menu_activate(GtkMenuItem *menu_item, gpointer user_data) {
    
    prefs_popup();   
}

static void execute_menu_activate(GtkMenuItem *menu_item, gpointer user_data) {

    check_zimbra();
    set_zimbraci_timeout();
}

static void home_menu_activate(GtkMenuItem *menu_item, gpointer user_data) {

    gtk_show_uri(NULL,("http://"+get_pref_string("url")).c_str(),GDK_CURRENT_TIME,NULL);
}

static void quit_menu_activate(GtkMenuItem *menu_item, gpointer user_data) {
    
    gtk_main_quit();    
}

static void menu_popup(GtkStatusIcon *status_icon, guint button, guint activate_time, gpointer user_data) {
    
    if (button == 3 && user_data) {
        gtk_widget_show_all (GTK_WIDGET (user_data));
        gtk_menu_popup(GTK_MENU(user_data),NULL,NULL,gtk_status_icon_position_menu,si,button,activate_time);
    }
}

int main( int argc, char *argv[]) {

    gtk_init(&argc, &argv);
    
    textdomain("zimbraci");

    si = gtk_status_icon_new_from_file("./mail_generic.png");
    //~ g_signal_connect (G_OBJECT (si), "activate", G_CALLBACK (icon_activate),NULL);
    
    GtkMenu *m = GTK_MENU(gtk_menu_new());
    GtkImageMenuItem *imi;

    imi = GTK_IMAGE_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_EXECUTE, NULL));
    gtk_menu_item_set_label(GTK_MENU_ITEM(imi),_("Check now"));
    g_signal_connect (G_OBJECT (imi), "activate", G_CALLBACK (execute_menu_activate),NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(m),GTK_WIDGET(imi));

    imi = GTK_IMAGE_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_HOME, NULL));
    gtk_menu_item_set_label(GTK_MENU_ITEM(imi),_("Open in browser"));
    g_signal_connect (G_OBJECT (imi), "activate", G_CALLBACK (home_menu_activate),NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(m),GTK_WIDGET(imi));

    gtk_menu_shell_append(GTK_MENU_SHELL(m),gtk_separator_menu_item_new());

    imi = GTK_IMAGE_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, NULL));
    g_signal_connect (G_OBJECT (imi), "activate", G_CALLBACK (prefs_menu_activate),NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(m),GTK_WIDGET(imi));

    gtk_menu_shell_append(GTK_MENU_SHELL(m),gtk_separator_menu_item_new());

    imi = GTK_IMAGE_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_CLOSE, NULL));
    g_signal_connect (G_OBJECT (imi), "activate", G_CALLBACK (quit_menu_activate),NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(m),GTK_WIDGET(imi));

    g_signal_connect (G_OBJECT (si), "popup-menu", G_CALLBACK (menu_popup),m);

    notify_init ("Zimbraci");
    glade_init();
	
    // mail_find.png -- mail_delete.png -- mail_new.png -- identity.png -- mail_generic.png

    uyari = notify_notification_new ("Zimbraci",_("Active"),NULL,NULL);
    //~ notify_notification_show(uyari,NULL);

    //~ timer = g_timer_new();
    //~ g_timer_start(timer);

	if (string(get_pref_string("url")).empty() ||
		string(get_pref_string("usr")).empty() ||
		string(get_pref_string("psw")).empty() ||
		string(get_pref_string("tmo")).empty() ||
		string(get_pref_string("qry")).empty()) 
		prefs_popup();
	else
		timeout_id = g_timeout_add_seconds( interval, (GtkFunction) check_zimbra, NULL);

    lastrun = time(NULL);
    
    gtk_main();

    return 0;
}

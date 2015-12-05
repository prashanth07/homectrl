#include<stdio.h>
#include "common.h"

void error_handler(int err_code)
{
	tkWidget *dialog, *dg_label, *dg_OK;
	
	switch (err_code){
		case LOGIN_SUCCESS:
		  
		    dialog = gtk_dialog_new();
		    dg_label = gtk_label_new ("Login Success");
		    break;
		  
		
		case SUCCESS:
		  
		    dialog = gtk_dialog_new();
		    dg_label = gtk_label_new ("Success");
		    break;
		  
		case LOGOUT_SUCCESS:
		  
		    dialog = gtk_dialog_new();
		    dg_label = gtk_label_new ("Logout Success");
		    break;
		  
		case ERR_ILLEGAL_CMD:
		  
		    dialog = gtk_dialog_new();
		    dg_label = gtk_label_new ("Error: Illegal command");
		    break;
		  
		case ERR_LOGIN_FAILURE:
		  
		    dialog = gtk_dialog_new();
		    dg_label = gtk_label_new ("Error: Login Failed");
		    break;
		  
		case ERR_NOT_LOGGED_IN:
		  
		    dialog = gtk_dialog_new();
		    dg_label = gtk_label_new ("Error: Not logged in");
		    break;
		  
		case ERR_CONNECTION_FULL:
		  
		    dialog = gtk_dialog_new();
		    dg_label = gtk_label_new ("ERROR: Connection Full");
		    break;
		  
		case ERR_SCHEDULE:
		  
		    dialog = gtk_dialog_new();
		    dg_label = gtk_label_new ("Error: Schedule");
		    break;
		  
		case ERR_UNKNOWN_PACKET:
		  
			dialog = gtk_dialog_new();
		    	dg_label = gtk_label_new ("Error: Unknown Packet");
		    	break;
		
		default:
			dialog = gtk_dialog_new();
			dg_label = gtk_label_new ("Error: Unknown Packet1");
			break;
			
	}



       	dg_OK = gtk_button_new_with_label ("OK");
	gtk_signal_connect_object (GTK_OBJECT (dg_OK), "clicked", gtk_widget_destroy, GTK_OBJECT(dialog));
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->action_area), dg_OK);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), dg_label);
	gtk_widget_show_all (dialog);
	gtk_widget_grab_focus(dialog);

}

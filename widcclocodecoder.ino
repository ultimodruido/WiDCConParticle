/****************
 * Global variables
 ***************/

int LED = D7;

enum States {
    STATE_INIT,
    STATE_LOGIN,
    STATE_RUNNING,
    STATE_CONFIG,
    STATE_ERROR
};

enum States my_state;

TCPClient widcc_client;

byte server[] = { 192, 168, 1, 109 };
int port = 7246;

struct LocoDescriptor {

    //char loco_id[32] = "";
    int loco_real_speed = 0;
    int loco_target_speed = 0;
    boolean direction = true;
    boolean light_auto = true;
    boolean light_front = false;
    boolean light_rear = false;
    boolean F1 = false;
    boolean F2 = false;
    boolean F3 = false;
    boolean F4 = false;

};
// real loco values
LocoDescriptor my_loco;
// memory space for inputs received
LocoDescriptor widcc_command;

void f_send_alive();
Timer alive_timer(3000, f_send_alive);

/****************
 * Logging
 ***************/
boolean LOGGING = true;

TCPClient log_client;
int log_port = 7247;

void f_log(char* msg) {

    if ( LOGGING ) {
        if (log_client.connect(server, log_port)) {
            log_client.println( msg );
        }
        log_client.stop();
    }
}


/****************
 * WiDCC Protocol
 ***************/
char WiDCCProtocolVersion[8] = "0.1.0";

void f_msg_login(char* msg_char) {
    String msg_string = String::format("LOGIN|");
    msg_string.concat(System.deviceID());
    msg_string.concat(String::format("#%s", WiDCCProtocolVersion));
    msg_string.toCharArray(msg_char, 64);
}

void f_msg_alive(char* msg_char) {
    String msg_string = String("ALIVE|");
    msg_string.concat(System.deviceID());
    msg_string.concat(String::format("#%u|%u|%u#%u|%u|%u#%u|%u|%u|%u", \
                                        my_loco.loco_real_speed, my_loco.loco_target_speed, my_loco.direction, \
                                        my_loco.light_auto, my_loco.light_front, my_loco.light_rear, \
                                        my_loco.F1, my_loco.F2, my_loco.F3, my_loco.F4));
    msg_string.toCharArray(msg_char, 64);
}

boolean f_make_bool(int val) {
	  return (1 && val );
}

/*void f_read_msg_command(char* msg_char) {
    // update my_loco here
    //RGB.color(0, 0, 255);
    // Get main message splitters
    String msg_str = String::format("%s", msg_char);
    int split_1 = msg_str.indexOf("#");
    int split_2 = msg_str.indexOf("#", split_1+1);
    int split_3 = msg_str.indexOf("#", split_1+2);
    // Find driver command
    int subsplit = msg_str.indexOf("|", split_1+1);
    String cmd = msg_str.substring(split_1+1, subsplit);
    widcc_command.loco_target_speed = cmd.toInt();
    cmd = msg_str.substring(subsplit + 1, split_2);
    widcc_command.direction = f_make_bool(cmd.toInt());
    // Find lights instructions
    // light auto
    subsplit = msg_str.indexOf("|", split_2+1);
    cmd = msg_str.substring(split_2+1, subsplit );
    widcc_command.light_auto = f_make_bool(cmd.toInt());
    // light front
    int subsplit2 = msg_str.indexOf("|", subsplit+1);
    cmd = msg_str.substring(subsplit + 1, subsplit2);
    widcc_command.light_front = f_make_bool(cmd.toInt());
    // light rear
    cmd = msg_str.substring(subsplit2+1, split_3);
    widcc_command.light_rear = f_make_bool(cmd.toInt());

    // extra functions F1-F4 not implemented
}*/

boolean f_check_cmd_id(String* msg, String* cmd) {
	  //char sys_id[32];
	  //snprintf(sys_id, 32, "CHECK");
	  //f_log(sys_id);
	  //f_log(msg);
	  //sscanf(msg,"%s|%s#%*s", cmd, id);
	  //f_log(cmd);
	  //f_log(id);
	  String dev_id = msg->substring(msg->indexOf("|")+1, msg->indexOf("#"));
	  cmd->concat( msg->substring(0, msg->indexOf("|")) );
	  //System.deviceID();
	  //how to compare char* & string?

	  //dev_id.toCharArray(sys_id, 32);
	  //f_log(sys_id);
	  if (!strncmp(id, sys_id, 32)) {
	  	    return true;
	  }
	  else {
	  	    return false;
	  }
}

/****************
 * States functions
 ***************/

void f_state_init() {
    // wait for wifi to be ready
    //digitalWrite(LED, HIGH);
    char log_msg[] = "STATUS INIT";
    f_log(log_msg);
    if (WiFi.ready()) {
    	    my_state = STATE_LOGIN;
    }
    else {
    	    delay(1000);
    }
}

void f_state_login() {
    char log_msg[] = "STATUS LOGIN";
    f_log(log_msg);
    if (WiFi.ready()) {
        // open TCP client and connect
        if (widcc_client.connect(server, port)) {
            //char log_msg2[] = "TCP connected";
            //f_log(log_msg2);
            // prepare a login message and se
            char widcc_login_msg[64];
            f_msg_login(widcc_login_msg);
            widcc_client.println( widcc_login_msg );

            String reply = "";
            // loop to get the reply
            while (widcc_client.connected()) {
                if (widcc_client.available()) {
                    reply.concat(String::format("%c", widcc_client.read()));
                }
            }
            widcc_client.stop();

            // read t TODOhe reply

            if (reply.indexOf("OK") != -1) {
    	           // in case of successful login, activate
    	           // the alive timer which regularly
    	           // contacts the server
                   alive_timer.start();
                   my_state = STATE_RUNNING;
            }
        }
    }
    else {
 	    // if tye connecton get lost, move bac to Init  else {
        my_state = STATE_INIT;
    }

}

void f_state_running() {
    char log_msg[] = "STATUS RUNNING";
    f_log(log_msg);
    if (!WiFi.ready()) {
        my_state = STATE_INIT;
        alive_timer.stop();
//        RGB.color(0, 255, 255);
//        delay(1500);
    }
    else {
        //RGB.color(255, 255, 0);
        delay(1500);
    }
    //TODO in RUNNING update the real loco
}

void f_send_alive() {
    char log_msg[] = "ALIVE";
    f_log(log_msg);
    if (widcc_client.connect(server, port)) {
        char widcc_message[64];
        f_msg_alive( widcc_message );
        widcc_client.println( widcc_message );

        String widcc_reply = "";
        //char widcc_reply[64];

        while (widcc_client.connected()) {
            if (widcc_client.available()) {
                widcc_reply.concat(String::format("%c", client.read()));
                //char c;
                //c = widcc_client.read();
                //strncat(widcc_reply, (char*)widcc_client.read(), 1);
            }
        }
        widcc_client.stop();

        //strncat(widcc_reply, , 1);

        String cmd;
        //String id[32];


        if ( f_check_cmd_id(&widcc_reply, &cmd)) {
        	   //char command[16] = f_check_command(&reply);
           	 //RGB.color(255, 0, 255);
            // Todo switch
            //if ( !strncmp(cmd, "OK", 2) ) {
            if (command.compareTo("OK") == 0) {
                RGB.color(0, 255, 0);
            }
            else if ( !strncmp(cmd, "COMMAND", 7)) {
                //RGB.color(0, 0, 255);
                //char widcc_reply[64];
                //reply.toCharArray(widcc_reply, 64);
                //f_read_msg_command( widcc_reply );
            }
            else if ( !strncmp(cmd, "UPDATE", 6)) {
            }
            else if ( !strncmp(cmd, "EMERGENCY", 9)) {
                RGB.color(255, 0, 0);
            }
            else if ( !strncmp(cmd, "IDENTIFY", 8)) {
                RGB.color(255, 255, 0);
            }
            else if ( !strncmp(cmd, "CONFIG", 6)) {
                RGB.color(0, 255, 255);
            }

        }
        else {
            // Log wrong device id
            char msg[] = "Message received with wrong device id";
            f_log(msg);
        }
    }
}


void setup() {

    pinMode(LED, OUTPUT);

    RGB.control(true);

    my_state = STATE_INIT;

}

void loop() {

    switch (my_state) {
        case STATE_INIT:
            f_state_init();
            break;
        case STATE_LOGIN:
            f_state_login();
            break;
        case STATE_RUNNING:
            f_state_running();
            break;
        case STATE_ERROR:
            break;
        case STATE_CONFIG:
            break;
    }

}

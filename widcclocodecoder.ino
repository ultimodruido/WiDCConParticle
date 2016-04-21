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

void f_read_msg_command(String* msg) {
    // Get main message splitters
    int split_1 = msg->indexOf("#");
    int split_2 = msg->indexOf("#", split_1+1);
    int split_3 = msg->indexOf("#", split_2+1);

    // Find motor driver command (target_speed|direction)
    String msg_part = msg->substring(split_1+1, split_2);
    int divider_1 = msg_part.indexOf("|");

    String instruction = msg_part.substring(0, divider_1 );
    widcc_command.loco_target_speed = instruction.toInt();
    //instruction.clear();

    instruction = msg_part.substring( divider_1+1, msg_part.length());
    widcc_command.direction = f_make_bool(instruction.toInt());
    //instruction.clear();

    // Find loco's lights command (auto|front|rear)
    //msg_part.clear();
    msg_part = msg->substring(split_2+1, split_3);
    divider_1 = msg_part.indexOf("|");
    int divider_2 = msg_part.indexOf("|", divider_1+1 );

    instruction = msg_part.substring(0, divider_1 );
    widcc_command.light_auto = f_make_bool( instruction.toInt());
    //instruction.clear();

    instruction = msg_part.substring(divider_1+1, divider_2);
    widcc_command.light_front = f_make_bool( instruction.toInt());
    //instruction.clear();

    instruction = msg_part.substring(divider_2+1, msg_part.length() );
    widcc_command.light_rear = f_make_bool( instruction.toInt());
    //instruction.clear();

    instruction = msg_part.substring( divider_2+1, msg_part.length());
    widcc_command.direction = f_make_bool(instruction.toInt());
    //instruction.clear();

    // extra functions F1-F4 not implemented yet
}

boolean f_check_cmd_id(String* msg, String* cmd) {
	  int msg_div = msg->indexOf("|");
	  String id = msg->substring(msg_div+1, msg->indexOf("#"));
	  cmd->concat(msg->substring(0, msg_div));

	  if (id.compareTo(System.deviceID()) == 0) {
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

            if (reply.indexOf("OK") == 0) {
    	           // in case of successful login, activate
    	           // the alive timer which regularly
    	           // contacts the server
                   alive_timer.start();
                   my_state = STATE_RUNNING;
            }
        }
    }
    else {
 	      // if type connecton get lost, move bac to Init
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

        while (widcc_client.connected()) {
            if (widcc_client.available()) {
                widcc_reply.concat(String::format("%c", widcc_client.read()));
            }
        }
        widcc_client.stop();

        String cmd = "";

        if ( f_check_cmd_id(&widcc_reply, &cmd)) {
            // Switch to identify the command
            if (cmd.compareTo("OK") == 0) {
            	   // display LED GREEN
                RGB.color(0, 255, 0);
            }
            else if ( cmd.compareTo("COMMAND") == 0) {
            	   // display LED BLUE
                RGB.color(0, 0, 255);
                // execute interpreter function
                f_read_msg_command( &widcc_reply );
            }
            else if ( cmd.compareTo("UPDATE") == 0) {
            }
            else if ( cmd.compareTo("EMERGENCY") == 0) {
            	   // display LED RED
                RGB.color(255, 0, 0);
            }
            else if ( cmd.compareTo("IDENTIFY") == 0)  {
                RGB.color(255, 255, 0);
            }
            else if ( cmd.compareTo("CONFIG") == 0) {
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

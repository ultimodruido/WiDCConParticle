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

TCPClient client;

byte server[] = { 192, 168, 1, 109 };
int port = 7245;

//char message[64];

struct LocoDescriptor {

    char loco_id[16] = "";
    int loco_real_speed = 0;
    int loco_target_speed = 0;
    boolean direction = false;
    boolean light_auto = false;
    boolean light_front = false;
    boolean light_rear = false;
    boolean F1 = false;
    boolean F2 = false;
    boolean F3 = false;
    boolean F4 = false;
    
};

LocoDescriptor my_loco;

void f_send_alive();
Timer alive_timer(3000, f_send_alive);

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

void f_set_msg_command(char* msg_char) {
    // update my_loco here
    RGB.color(0, 0, 255);
    
}


/****************
 * States functions
 ***************/

void f_state_init() {
    // wait for wifi to be ready
    //digitalWrite(LED, HIGH); 
    if (WiFi.ready()) {
    	    my_state = STATE_LOGIN;
    }
    else {
    	    delay(1000);
    }
}

void f_state_login() {
    if (WiFi.ready()) {
        // open TCP client and connect
        if (client.connect(server, port)) {
            // prepare a login message and se
            char widcc_login_msg[64]; 
            f_msg_login(widcc_login_msg);
            client.println( widcc_login_msg );
            
            String reply = ""; 
            // loop to get the reply
            while (client.connected()) {
                if (client.available()) {
                    reply.concat(String::format("%c", client.read()));
                }
            }
            client.stop();
            
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
    
}

void f_send_alive() {
    if (client.connect(server, port)) {
        char widcc_message[64];
        f_msg_alive( widcc_message );
        client.println( widcc_message );
            
        String reply = ""; 
            
        while (client.connected()) {
            if (client.available()) {
                reply.concat(String::format("%c", client.read()));
            }
        }
        client.stop();

        int splitter = reply.indexOf('#');
        String command_id = reply.substring(0, splitter);
        int splitter_2 = command_id.indexOf('|');
        String command = command_id.substring(0, splitter_2);
        String cmd_loco_id = command_id.substring( splitter_2 + 1, command_id.length());
        
        //to be removed!!!
        if (1) {
           TCPClient sub_client;
    
            if (sub_client.connect({ 192, 168, 1, 109 }, 7247)) {
                //sub_client.println( command );
                sub_client.println( cmd_loco_id );
            }  
            sub_client.stop();            
        }

        
        if ( cmd_loco_id.equals(System.deviceID())) {
        	RGB.color(255, 0, 255);
            // Todo switch
            if (command.compareTo("OK") == 0) {
                RGB.color(0, 255, 0);
            }
            else if (command.equals(String::format("COMMAND"))) {
                //RGB.color(0, 0, 255);
                char widcc_reply[64];
                reply.toCharArray(widcc_reply, 64);
                f_set_msg_command( widcc_reply );
            }
            else if (command.equals(String::format("UPDATE"))) {
            }
            else if (command.equals(String::format("EMERGENCY"))) {
                RGB.color(255, 0, 0);
            }
            else if (command.equals(String::format("IDENTIFY"))) {
                RGB.color(255, 255, 0);
            }
            else if (command.equals(String::format("CONFIG"))) {
                RGB.color(0, 255, 255);
            }

        }
        else if (reply.indexOf("OK") != -1) {
            RGB.color(255, 255, 255);
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
          
        case STATE_RUNNING:
            f_state_running();
            break;
        case STATE_ERROR:
            break;  break;
        case STATE_CONFIG:
            break;
    }

}
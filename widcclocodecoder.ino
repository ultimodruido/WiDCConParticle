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

    int real_speed = 0;
    int target_speed = 0;
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

struct WiDCCInstruction {
  boolean updated = false;
  LocoDescriptor loco;
};
// memory space for inputs received
WiDCCInstruction my_loco_buffer;

void f_send_alive();
Timer alive_timer(3000, f_send_alive);

/****************
 * Logging
 ***************/
boolean LOGGING = false;

TCPClient log_client;
int log_port = 7247;

void f_log(String* msg) {

    if ( LOGGING ) {
        if (log_client.connect(server, log_port)) {
            log_client.println( *msg );
        }
        log_client.stop();
    }
}


/****************
 * WiDCC Protocol
 ***************/
char WiDCCProtocolVersion[8] = "0.1.0";

void f_tcp_receive_msg(TCPClient* widcc_client, String* widcc_reply) {
    while (widcc_client->connected()) {
        if (widcc_client->available()) {
            widcc_reply->concat(String::format("%c", widcc_client->read()));
        }
    }
}

/*void f_msg_login(String* msg) {
    msg->concat("LOGIN|");
    msg->concat(System.deviceID());
    msg->concat(String::format("#%s#", WiDCCProtocolVersion));

}*/

void f_msg_alive(String* msg) {
    msg->concat("ALIVE|");
    msg->concat(System.deviceID());
    msg->concat( String::format("#%u|%u|%u#%u|%u|%u#%u|%u|%u|%u#", \
                                        my_loco.real_speed, my_loco.target_speed, my_loco.direction, \
                                        my_loco.light_auto, my_loco.light_front, my_loco.light_rear, \
                                        my_loco.F1, my_loco.F2, my_loco.F3, my_loco.F4));
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
    my_loco_buffer.loco.target_speed = instruction.toInt();

    instruction = msg_part.substring( divider_1+1, msg_part.length());
    my_loco_buffer.loco.direction = f_make_bool(instruction.toInt());

    // Find loco's lights command (auto|front|rear)
    msg_part = msg->substring(split_2+1, split_3);
    divider_1 = msg_part.indexOf("|");
    int divider_2 = msg_part.indexOf("|", divider_1+1 );

    instruction = msg_part.substring(0, divider_1 );
    my_loco_buffer.loco.light_auto = f_make_bool( instruction.toInt());

    instruction = msg_part.substring(divider_1+1, divider_2);
    my_loco_buffer.loco.light_front = f_make_bool( instruction.toInt());

    instruction = msg_part.substring(divider_2+1, msg_part.length() );
    my_loco_buffer.loco.light_rear = f_make_bool( instruction.toInt());

    // extra functions F1-F4 not implemented yet
    // TODO

    // set updated flag
    my_loco_buffer.updated = true;
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
    String msg = "STATUS INIT 5";
    f_log(&msg);
    if (WiFi.ready()) {
    	    //my_state = STATE_LOGIN;
          my_state = STATE_RUNNING;
    }
    else {
    	    delay(1000);
    }
}
/*
void f_state_login() {
    String msg = "STATUS LOGIN";
    f_log(&msg);
    if (WiFi.ready()) {
        // open TCP client and connect
        if (widcc_client.connect(server, port)) {
            //char log_msg2[] = "TCP connected";
            //f_log(log_msg2);
            // prepare a login message and se
            String* widcc_msg = new String("");
            f_msg_login(widcc_msg);
            widcc_client.println( *widcc_msg );
            f_log(widcc_msg);

            // clear the string to free memory
            delete widcc_msg;

            String* widcc_reply = new String("");
            // loop to get the reply
            f_tcp_receive_msg(&widcc_client, widcc_reply);

            widcc_client.stop();

            f_log(widcc_reply);

            if (widcc_reply->indexOf("LOGINOK") == 0) {
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

}*/

void f_state_running() {
  String msg = "STATUS RUNNING";
  f_log(&msg);
  if (!WiFi.ready()) {
    my_state = STATE_INIT;
    alive_timer.stop();
  }

  if (my_loco_buffer.updated) {
    //update the loco parameters
    my_loco.target_speed = my_loco_buffer.loco.target_speed;
    my_loco.direction = my_loco_buffer.loco.target_speed;

    my_loco.light_auto = my_loco_buffer.loco.light_auto;
    my_loco.light_front = my_loco_buffer.loco.light_front;
    my_loco.light_rear = my_loco_buffer.loco.light_rear;

    my_loco.F1 = my_loco_buffer.loco.F1;
    my_loco.F2 = my_loco_buffer.loco.F2;
    my_loco.F3 = my_loco_buffer.loco.F3;
    my_loco.F4 = my_loco_buffer.loco.F4;

    //clear updated flag
    my_loco_buffer.updated = false;
  }

  delay(10);
}

void f_send_alive() {
  String msg = "ALIVE";
  f_log(&msg);
  if (widcc_client.connect(server, port)) {
    String* widcc_msg = new String("");
    f_msg_alive( widcc_msg );
    widcc_client.println( *widcc_msg );

   //reuse the String to get the reply
    delete widcc_msg;

    String* widcc_reply = new String("");

    f_tcp_receive_msg(&widcc_client, widcc_reply);
    widcc_client.stop();

    f_log(widcc_reply);
    String* cmd = new String("");

    if ( f_check_cmd_id(widcc_reply, cmd)) {
      // Switch to identify the command
      if (cmd->compareTo("OK") == 0) {
        // display LED GREEN
        RGB.color(0, 255, 0);
      }
      else if ( cmd->compareTo("COMMAND") == 0) {
        // display LED BLUE
        RGB.color(0, 0, 255);
        // execute interpreter function
        //f_read_msg_command( widcc_reply );
      }
      else if ( cmd->compareTo("UPDATE") == 0) {
        // display LED WHITE
        RGB.color(255, 255, 255);
      }
      else if ( cmd->compareTo("EMERGENCY") == 0) {
        // display LED RED
        RGB.color(255, 0, 0);
      }
      else if ( cmd->compareTo("IDENTIFY") == 0)  {
        RGB.color(255, 255, 0);
      }
      else if ( cmd->compareTo("CONFIG") == 0) {
        RGB.color(0, 255, 255);
      }
    }
    else {
      // Log wrong device id
      String msg = "Message received with wrong device id";
      f_log(&msg);
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
            //f_state_login();
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

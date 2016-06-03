/****************
* Header section
***************/

// enumeration for state machine
enum States {
  STATE_INIT,
  STATE_TURN_ON,
  STATE_RUNNING,
  STATE_ERROR
};

// definition of a class containing parameters
// and configuration of a decoder. it has to be
// initialzed twice:
// 1) contains the real decoder values
// 2) is a buffer for storing the values
//    transmitted by the server
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

  // control variable
  boolean modified = false;
};

// program control variabes are collected in
// a counters class
struct Counters {
  int runs = 0;
  int alive = 0;
  int tcp_comunication = 0;
};
//////////////////////////////
// function declaration section
//////////////////////////////
// various
void f_log(String*);
boolean f_make_bool(int );

// widcc protocol related
void f_tcp_receive_msg(TCPClient* , String* );
void f_msg_alive(String* );
//void f_read_msg_command(String* );
boolean f_check_cmd_id(String* , String* );

// state machine related
void f_state_init();
void f_state_turn_on();
void f_state_running();
void f_state_error();
void f_send_alive();

/****************
* Global variables
***************/

// control variable for the state machine
enum States my_state;

// wifi related variables
TCPClient widcc_client;
byte server[] = { 192, 168, 1, 101 };
int port = 7246;


// real loco values
LocoDescriptor my_loco;
// memory space for inputs received
LocoDescriptor widcc_instruction;

// init program control variables
Counters counter;

// time related parameters
const int ALIVE_INTERVAL = 3000;
const int ALIVE_ERROR_REPLIES = 4; // means 2 seconds
const int RUNNING_DELAY = 100;
Timer alive_timer( ALIVE_INTERVAL , f_send_alive);

/****************
* Logging
***************/
boolean LOGGING = true;

void f_log(String* msg) {

  if ( LOGGING ) {
    Serial.println(*msg);
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

void f_msg_alive(String* msg) {
  msg->concat("[*] ALIVE|");
  msg->concat(System.deviceID());
  msg->concat( String::format("#%u|%u|%u#%u|%u|%u#%u|%u|%u|%u#", \
  my_loco.real_speed, my_loco.target_speed, my_loco.direction, \
  my_loco.light_auto, my_loco.light_front, my_loco.light_rear, \
  my_loco.F1, my_loco.F2, my_loco.F3, my_loco.F4));
}

boolean f_make_bool(int val) {
  return (1 && val );
}

/*void f_read_msg_command(String* msg) {
  // Get main message splitters
  int split_1 = msg->indexOf("#");
  int split_2 = msg->indexOf("#", split_1+1);
  int split_3 = msg->indexOf("#", split_2+1);

  // Find motor driver command (target_speed|direction)
  String msg_part = msg->substring(split_1+1, split_2);
  int divider_1 = msg_part.indexOf("|");

  String instruction = msg_part.substring(0, divider_1 );
  widcc_instruction.target_speed = instruction.toInt();

  instruction = msg_part.substring( divider_1+1, msg_part.length());
  widcc_instruction.direction = f_make_bool(instruction.toInt());

  // Find loco's lights command (auto|front|rear)
  msg_part = msg->substring(split_2+1, split_3);
  divider_1 = msg_part.indexOf("|");
  int divider_2 = msg_part.indexOf("|", divider_1+1 );

  instruction = msg_part.substring(0, divider_1 );
  widcc_instruction.light_auto = f_make_bool( instruction.toInt());

  instruction = msg_part.substring(divider_1+1, divider_2);
  my_loco_buffer.loco.light_front = f_make_bool( instruction.toInt());

  instruction = msg_part.substring(divider_2+1, msg_part.length() );
  widcc_instruction.light_rear = f_make_bool( instruction.toInt());

  // extra functions F1-F4 not implemented yet
  // TODO

  // set updated flag
  widcc_instruction.modified = true;
} */

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

  if (LOGGING) {
    Serial.begin(9600);
  }

  // wait for wifi to be ready
  //digitalWrite(LED, HIGH);
  String msg = "STATUS INIT 6";
  f_log(&msg);

  delay(1000);

  if (WiFi.ready()) {
    //my_state = STATE_LOGIN;
    my_state = STATE_TURN_ON;

  }
  else {
    my_state = STATE_ERROR;
  }
  delay(500);
}

void f_state_turn_on() {
	String msg = "STATUS TURN ON";
  f_log(&msg);

  if(!alive_timer.isActive()) {
    alive_timer.start();
  }

  if(counter.tcp_comunication > 0) {
  	  my_state = STATE_RUNNING;
  	  counter.tcp_comunication = 0;
  }
}

void f_state_running() {
  //String msg = "STATUS RUNNING";
  //f_log(&msg);

  //update flags
  counter.runs++;

  if (!WiFi.ready()) {
    my_state = STATE_ERROR;

  }
  else {
    if (widcc_instruction.modified) {
      //update the loco parameters
      String msg2 = "STATUS RUNNING - update";
      f_log(&msg2);
      my_loco.target_speed = widcc_instruction.target_speed;
      my_loco.direction = widcc_instruction.direction;

      my_loco.light_auto = widcc_instruction.light_auto;
      my_loco.light_front = widcc_instruction.light_front;
      my_loco.light_rear = widcc_instruction.light_rear;

      my_loco.F1 = widcc_instruction.F1;
      my_loco.F2 = widcc_instruction.F2;
      my_loco.F3 = widcc_instruction.F3;
      my_loco.F4 = widcc_instruction.F4;

      //clear updated flag
      widcc_instruction.modified = false;
    }
  }
//delay(RUNNING_DELAY);
}

void f_state_error() {
	String msg = "STATUS ERROR";
  f_log(&msg);

  // stop timer
  if(alive_timer.isActive()) {
    alive_timer.stop();
  }

  // do something else
  // CODE HERE...

  // move back to turn on
  if (WiFi.ready()) {
    my_state = STATE_TURN_ON;
  }
}

void f_send_alive() {
  String msg = "ALIVE";
  f_log(&msg);

  String msg2 = String::format("RUNNING STATUS counter: %i", counter.runs);
  f_log(&msg2);

  if (counter.runs == 0) {
    String warning = "NO RUNS!!!";
    f_log(&warning);
  }
  // reset runs
 counter.runs = 0;

  if (counter.alive >= ALIVE_ERROR_REPLIES ) {
    // my_state = STATE_ERROR;
    String warning2 = "*** ERROR *** NO ALIVE REPLIES ***";
    f_log(&warning2);
  }
  // update alive flag
  counter.alive++;

  if (WiFi.ready()) {
    if (widcc_client.connect(server, port)) {
      String* widcc_msg = new String("");
      f_msg_alive( widcc_msg );
      widcc_client.println( *widcc_msg );

      //free the String mmory
      delete widcc_msg;

      String* widcc_reply = new String("");

      f_tcp_receive_msg(&widcc_client, widcc_reply);
      widcc_client.stop();


      //increase tcp_communication counter
      counter.tcp_comunication++;
      f_log(widcc_reply);
      String* cmd = new String("");

      if ( f_check_cmd_id(widcc_reply, cmd)) {

        // message received - update alive flag
        counter.alive = 0;

        String input_received = "Input received: ";

        // Switch to identify the command
        if (cmd->compareTo("OK") == 0) {
          input_received += "OK";
          // display LED GREEN
          RGB.color(0, 255, 0);
        }
        else if ( cmd->compareTo("COMMAND") == 0) {
          input_received += "COMMAND";
          // display LED BLUE
          RGB.color(0, 0, 255);
          // execute interpreter function
          //f_read_msg_command( widcc_reply );
        }
        else if ( cmd->compareTo("UPDATE") == 0) {
          input_received += "UPDATE";
          // display LED WHITE
          RGB.color(255, 255, 255);
        }
        else if ( cmd->compareTo("EMERGENCY") == 0) {
          input_received += "EMERGENCY";
          // display LED RED
          RGB.color(255, 0, 0);
        }
        else if ( cmd->compareTo("IDENTIFY") == 0)  {
          input_received += "IDENTIFY";
          RGB.color(255, 255, 0);
        }
        else if ( cmd->compareTo("CONFIG") == 0) {
          input_received += "CONFIG";
          RGB.color(0, 255, 255);
        }
        f_log(&input_received);
      }
      else {
        // Log wrong device id
        String msg = "Message received with wrong device id";
        f_log(&msg);
      }
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
    case STATE_TURN_ON:
      f_state_turn_on();
      break;
    case STATE_RUNNING:
      f_state_running();
      break;
    case STATE_ERROR:
      f_state_error();
      break;
  }

}
// Standard includes
#include "pebble.h"

#define MAX_MESSAGES 5
#define MAX_TEXT_LENGTH 124

// App-specific data
Window *window; // All apps must have at least one window

static AppTimer* kill_timer;

// All the UI layers
static ActionBarLayer *action_bar;
static ScrollLayer *scroll_layer;
static TextLayer *master_text_layer;
static TextLayer *text_layer[MAX_MESSAGES];
static BitmapLayer *header_bubble_layer[MAX_MESSAGES];
static TextLayer *header_text_layer[MAX_MESSAGES];
static TextLayer *from_text_layer[MAX_MESSAGES];
static TextLayer *subject_text_layer[MAX_MESSAGES];
static uint8_t numMessagesFilled;
static uint8_t nextWriteIndex;
static TextLayer *deleteConfirmLayer;
static BitmapLayer *trashImageLayer;
static BitmapLayer *questionImageLayer;
static TextLayer *pressAgainTextLayer;

// All the bitmaps we use
static GBitmap* upArrow;
static GBitmap* downArrow;
static GBitmap* trash;
static GBitmap* trashWhite;
static GBitmap* reply1;
static GBitmap* reply2;
static GBitmap* open;
static GBitmap* question;
static GBitmap* bubble;

// The mode defines what the action bar commands will be and is one of ModeType
static uint8_t mode;
static int32_t utc_offset;

static bool actions_enabled_val;

// Lorum ipsum to have something to scroll
static int32_t header_time[MAX_MESSAGES];
static uint32_t account_id[MAX_MESSAGES];
static char header_text[MAX_MESSAGES][MAX_TEXT_LENGTH];
static char scroll_text[MAX_MESSAGES][MAX_TEXT_LENGTH];
static char from_text[MAX_MESSAGES][MAX_TEXT_LENGTH];
static char subject_text[MAX_MESSAGES][MAX_TEXT_LENGTH];
static char uuid_text[MAX_MESSAGES][MAX_TEXT_LENGTH];

enum ModeType {
  MODE_SCROLL = 0x0,
  MODE_ACTION = 0x1,
  MODE_DELETE_CONFIRM = 0x2,
};

enum InMsgType {
  KEY_MSG_UUID = 0x0,
  KEY_MSG_TIME = 0x1,
  KEY_MSG_FROM = 0x2,
  KEY_MSG_SUBJECT = 0x3,
  KEY_MSG_TEXT = 0x4,
  KEY_VIBE_PATTERN = 0x5,
  KEY_ACTION_SUPPORT = 0x6,
  KEY_UTC_OFFSET = 0x7,
  KEY_ACCOUNT_ID = 0x8,
};

enum OutMsgType {
  KEY_CMD = 0x9,
};

enum OutMsgCommands {
  VAL_CMD_DELETE = 0x0,
  VAL_CMD_REPLY1 = 0x1,
  VAL_CMD_REPLY2 = 0x2,
  VAL_CMD_OPEN = 0x3,
};

enum VibePatterns {
  VIBE_PATTERN_SHORT = 0x0,
  VIBE_PATTERN_LONG = 0x1,
  VIBE_PATTERN_DOUBLE = 0x2,
} VibePatterns;

enum PersistenceKeys {
  STORAGE_ACTION_ENABLED_KEY = 0x0,
  STORAGE_NUM_FILLED_KEY = 0x1,
  STORAGE_NEXT_INDEX_KEY = 0x2,
  STORAGE_UTC_OFFSET_KEY = 0x3,
};

//
// Handle persistent modes
//

void reschedule_kill_timer() {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Resetting kill timer");
  light_enable_interaction();
  if( kill_timer != NULL )
    app_timer_reschedule(kill_timer, 30*1000);
}

bool actions_enabled() {
  bool enabled_value = false;
  if (persist_exists(STORAGE_ACTION_ENABLED_KEY)) {
    enabled_value = persist_read_bool(STORAGE_ACTION_ENABLED_KEY);
  }
  return enabled_value;
}

void set_actions_enabled(bool enabled) {
  persist_delete(STORAGE_ACTION_ENABLED_KEY);
  persist_write_bool(STORAGE_ACTION_ENABLED_KEY, enabled);
}

void persist_messages(int8_t index,bool store_metadata)
{
  /*
  static char header_text[MAX_MESSAGES][MAX_TEXT_LENGTH];
  static char scroll_text[MAX_MESSAGES][MAX_TEXT_LENGTH];
  static char from_text[MAX_MESSAGES][MAX_TEXT_LENGTH];
  static char subject_text[MAX_MESSAGES][MAX_TEXT_LENGTH];
  static char uuid_text[MAX_MESSAGES][MAX_TEXT_LENGTH];
  */
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Persisting current messages for index %d...",index);

  if( index == -1 || index == 0 )
  {
    persist_delete(0x11);
    persist_write_string(0x11,scroll_text[0]);
    if( store_metadata )
    {
      persist_delete(0x10);
      persist_write_int(0x10,header_time[0]);
      
      persist_delete(0x12);
      persist_write_string(0x12,from_text[0]);
      
      persist_delete(0x13);
      persist_write_string(0x13,subject_text[0]);
      
      persist_delete(0x14);
      persist_write_string(0x14,uuid_text[0]);
      
      persist_delete(0x15);
      persist_write_int(0x15,account_id[0]);
    }
  }
  
  if( index == -1 || index == 1 )
  {
    persist_delete(0x21);
    persist_write_string(0x21,scroll_text[1]);
    if( store_metadata )
    {
      persist_delete(0x20);
      persist_write_int(0x20,header_time[1]);
      
      persist_delete(0x22);
      persist_write_string(0x22,from_text[1]);
      
      persist_delete(0x23);
      persist_write_string(0x23,subject_text[1]);
      
      persist_delete(0x24);
      persist_write_string(0x24,uuid_text[1]);
      
      persist_delete(0x25);
      persist_write_int(0x25,account_id[1]);
    }
  }
  
  if( index == -1 || index == 2 )
  {
    persist_delete(0x31);
    persist_write_string(0x31,scroll_text[2]);
    if( store_metadata )
    {
      persist_delete(0x30);
      persist_write_int(0x30,header_time[2]);
      
      persist_delete(0x32);
      persist_write_string(0x32,from_text[2]);
      
      persist_delete(0x33);
      persist_write_string(0x33,subject_text[2]);
      
      persist_delete(0x34);
      persist_write_string(0x34,uuid_text[2]);
      
      persist_delete(0x35);
      persist_write_int(0x35,account_id[2]);
    }
  }
  
  if( index == -1 || index == 3 )
  {
    persist_delete(0x41);
    persist_write_string(0x41,scroll_text[3]);
    if( store_metadata )
    {
      persist_delete(0x40);
      persist_write_int(0x40,header_time[3]);
      
      persist_delete(0x42);
      persist_write_string(0x42,from_text[3]);
      
      persist_delete(0x43);
      persist_write_string(0x43,subject_text[3]);
      
      persist_delete(0x44);
      persist_write_string(0x44,uuid_text[3]);
      
      persist_delete(0x45);
      persist_write_int(0x45,account_id[3]);
    }
  }
  
  if( index == -1 || index == 4 )
  {
    persist_delete(0x51);
    persist_write_string(0x51,scroll_text[4]);
    if( store_metadata )
    {
      persist_delete(0x50);
      persist_write_int(0x50,header_time[4]);
      
      persist_delete(0x52);
      persist_write_string(0x52,from_text[4]);
      
      persist_delete(0x53);
      persist_write_string(0x53,subject_text[4]);
      
      persist_delete(0x54);
      persist_write_string(0x54,uuid_text[4]);
      
      persist_delete(0x55);
      persist_write_int(0x55,account_id[4]);
    }
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Persisting complete.");
}

//
// Handle AppMessage
//

void out_sent_handler(DictionaryIterator *sent, void *context) {
   // outgoing message was delivered
   APP_LOG(APP_LOG_LEVEL_DEBUG, "Outgoing message was delivered successfully.");

}


void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
   // outgoing message failed
   APP_LOG(APP_LOG_LEVEL_DEBUG, "Failed to send outgoing message");

}

void in_received_handler(DictionaryIterator *iter, void *context) {
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Received new message from phone.");

  // incoming message received
  reschedule_kill_timer();
  
  Tuple *uuid_tuple = dict_find(iter, KEY_MSG_UUID);
  Tuple *offset_tuple = dict_find(iter, KEY_UTC_OFFSET);
  Tuple *time_tuple = dict_find(iter, KEY_MSG_TIME);
  Tuple *from_tuple = dict_find(iter, KEY_MSG_FROM);
  Tuple *subject_tuple = dict_find(iter, KEY_MSG_SUBJECT);
  Tuple *text_tuple = dict_find(iter,KEY_MSG_TEXT);
  Tuple *vibe_pattern_tuple = dict_find(iter,KEY_VIBE_PATTERN);
  Tuple *action_support_tuple = dict_find(iter,KEY_ACTION_SUPPORT);
  Tuple *account_id_tuple = dict_find(iter,KEY_ACCOUNT_ID);
    
  // Act on the found fields received
  if( offset_tuple )
  {
    persist_delete(STORAGE_UTC_OFFSET_KEY);
    persist_write_int(STORAGE_UTC_OFFSET_KEY,offset_tuple->value->int32);
    utc_offset = offset_tuple->value->int32;
  }
  if (uuid_tuple && subject_tuple) {
    
    for( int8_t i = 0; i < MAX_MESSAGES; i++ )
    {
       if( strcmp(uuid_tuple->value->cstring,uuid_text[i]) == 0 )
       {
         APP_LOG(APP_LOG_LEVEL_DEBUG, "Received duplicate for email UUID: %s", uuid_tuple->value->cstring);
         return;
       }
    }
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Found initial data for email UUID: %s", uuid_tuple->value->cstring);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Message From(%s) with Subject(%s)", from_tuple->value->cstring, subject_tuple->value->cstring);

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Copying message data into buffers at index %d...",nextWriteIndex);
    
    header_time[nextWriteIndex] = time_tuple->value->int32;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Timestamp on message is %d.",(int)header_time[nextWriteIndex]);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Now is %d.",(int)time(NULL));
    strcpy(uuid_text[nextWriteIndex],uuid_tuple->value->cstring);
    strcpy(from_text[nextWriteIndex],from_tuple->value->cstring);
    strcpy(subject_text[nextWriteIndex],subject_tuple->value->cstring);
    strcpy(scroll_text[nextWriteIndex],"...");
    account_id[nextWriteIndex] = account_id_tuple->value->uint32;
    
    persist_messages(nextWriteIndex,true);
 
    nextWriteIndex++;
    if( nextWriteIndex >  MAX_MESSAGES-1 )
      nextWriteIndex = 0;
    numMessagesFilled++;
    if( numMessagesFilled > MAX_MESSAGES )
      numMessagesFilled = MAX_MESSAGES;

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Updating UI text layers...");

    if( numMessagesFilled == MAX_MESSAGES )
    {
      int8_t startIndex = nextWriteIndex-1;
      if( startIndex < 0 )
        startIndex = MAX_MESSAGES-1;
      
      int8_t iterator = startIndex;

      int8_t writeIndex = 0;
      while( writeIndex < MAX_MESSAGES )
      {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Updating UI index %d with data from %d...",writeIndex,iterator);

        time_t emailTime = header_time[iterator];
        time_t now = time(NULL)-utc_offset;
     
        time_t age = now - emailTime;
        if( age < 2*60 )
        {
          strcpy(header_text[iterator],"Just Now");
        }
        else if( age < 60*60 )
        {
          time_t minutes = age / 60;
          snprintf(header_text[iterator],MAX_TEXT_LENGTH,"%d Minutes Ago",(int)minutes);
        }
        else if( age < 2*60*60 )
        {
          strcpy(header_text[iterator],"An Hour Ago");
        }
        else if( age < 24*60*60 )
        {
          time_t hours = age/60/60;
          snprintf(header_text[iterator],MAX_TEXT_LENGTH,"%d Hours Ago",(int)hours);
        }
        else
        {
          time_t days = age/60/60/24;
          snprintf(header_text[iterator],MAX_TEXT_LENGTH,"%d Days Ago",(int)days);
        }

        text_layer_set_text(from_text_layer[writeIndex],from_text[iterator]);
        text_layer_set_text(subject_text_layer[writeIndex],subject_text[iterator]);
        text_layer_set_text(text_layer[writeIndex],scroll_text[iterator]);
        text_layer_set_text(header_text_layer[writeIndex], header_text[iterator]);

        writeIndex++;
        iterator--;
        if( iterator < 0 ) 
          iterator = MAX_MESSAGES-1;
      }
    }
    else
    {
      int8_t writeIndex = 0;
      for( int8_t i = numMessagesFilled-1; i >= 0; i-- )
      {
        time_t emailTime = header_time[i];
        time_t now = time(NULL)-utc_offset;
     
        time_t age = now - emailTime;
        if( age < 2*60 )
        {
          strcpy(header_text[i],"Just Now");
        }
        else if( age < 60*60 )
        {
          time_t minutes = age / 60;
          snprintf(header_text[i],MAX_TEXT_LENGTH,"%d Minutes Ago",(int)minutes);
        }
        else if( age < 2*60*60 )
        {
          strcpy(header_text[i],"An Hour Ago");
        }
        else if( age < 24*60*60 )
        {
          time_t hours = age/60/60;
          snprintf(header_text[i],MAX_TEXT_LENGTH,"%d Hours Ago",(int)hours);
        }
        else
        {
          time_t days = age/60/60/24;
          snprintf(header_text[i],MAX_TEXT_LENGTH,"%d Days Ago",(int)days);
        }

        text_layer_set_text(from_text_layer[writeIndex],from_text[i]);
        text_layer_set_text(subject_text_layer[writeIndex],subject_text[i]);
        text_layer_set_text(text_layer[writeIndex],scroll_text[i]);
        text_layer_set_text(header_text_layer[writeIndex], header_text[i]);
        writeIndex++;
      }
    }    
    
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Total messages stored is now %d", numMessagesFilled);
  }
  else if( uuid_tuple && text_tuple) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Found email body data for email UUID: %s", uuid_tuple->value->cstring);    
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Found email body data: %s", text_tuple->value->cstring);

    int8_t toWrite = nextWriteIndex-1;
    if( toWrite < 0 )
      toWrite = MAX_MESSAGES-1;

    if( strcmp(uuid_tuple->value->cstring,uuid_text[toWrite]) == 0 )
    {
      TextLayer* bodyText = text_layer[0];
      
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Copying email body into buffer...");

      strcpy(scroll_text[toWrite],text_tuple->value->cstring);
      persist_messages(toWrite,false);
      
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Updated UI text layers...");
      text_layer_set_text(bodyText, scroll_text[toWrite]);
    }
  }
  else if( action_support_tuple ) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Found action enable message: %d", action_support_tuple->value->int8);
    actions_enabled_val = action_support_tuple->value->int8==1?true:false;
  }
  
  if( vibe_pattern_tuple ) {
    switch(vibe_pattern_tuple->value->int8)
    {
      case VIBE_PATTERN_SHORT:
      vibes_short_pulse();
      break;
      
      case VIBE_PATTERN_LONG:
      vibes_long_pulse();
      break;
      
      case VIBE_PATTERN_DOUBLE:
      vibes_double_pulse();
      break;
    }
  }
    
}

void in_dropped_handler(AppMessageResult reason, void *context) {
   // incoming message dropped
}

//
// Button Action Events
//
void back_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  
  layer_set_hidden(text_layer_get_layer(deleteConfirmLayer), true);
  if( mode == MODE_SCROLL && actions_enabled_val )
  {
    reschedule_kill_timer();
    mode = MODE_ACTION;
    
    action_bar_layer_set_icon(action_bar, BUTTON_ID_UP, reply1);
    action_bar_layer_set_icon(action_bar, BUTTON_ID_DOWN, reply2);
    action_bar_layer_set_icon(action_bar, BUTTON_ID_SELECT, open);
  }
  else if( actions_enabled_val )
  {
    reschedule_kill_timer();
    mode = MODE_SCROLL;
    
    action_bar_layer_set_icon(action_bar, BUTTON_ID_UP, upArrow);  
    action_bar_layer_set_icon(action_bar, BUTTON_ID_DOWN, downArrow);
    action_bar_layer_set_icon(action_bar, BUTTON_ID_SELECT, trash);
  }
  else
  {
    if( kill_timer != NULL )
      app_timer_cancel(kill_timer);
    kill_timer = NULL;
    window_stack_pop_all(true); 
  }
}

void down_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  Window *window = (Window *)context; // This context defaults to the window, but may be changed with \ref window_set_click_context.
  reschedule_kill_timer();
  
  if( mode == MODE_SCROLL )
  {
    GRect scrollFrame = layer_get_frame(scroll_layer_get_layer(scroll_layer));
    GPoint current = scroll_layer_get_content_offset(scroll_layer);
    int16_t yPos = (current.y / scrollFrame.size.h)*-1;

    yPos++;
  
    if( yPos > numMessagesFilled-1 )
      yPos = numMessagesFilled-1;
    if( yPos < 0 )
      yPos = 0;
  
    scroll_layer_set_content_offset(scroll_layer, GPoint(0,yPos*scrollFrame.size.h*-1), true);
  }
  else if( mode == MODE_ACTION )
  {
     // POST REPLY2 MESSAGE TO PHONE
     GRect scrollFrame = layer_get_frame(scroll_layer_get_layer(scroll_layer));
     GPoint current = scroll_layer_get_content_offset(scroll_layer);
     int16_t yPos = (current.y / scrollFrame.size.h)*-1;
    
     int8_t toSend = nextWriteIndex-1;
     if( toSend < 0 )
        toSend = MAX_MESSAGES-1;

     while( yPos > 0 )
     {
       yPos--;
       toSend--;
       if( toSend < 0 )
         toSend = MAX_MESSAGES-1;
     }

     DictionaryIterator *iter;
     app_message_outbox_begin(&iter);
     Tuplet value = TupletInteger(KEY_CMD, VAL_CMD_REPLY2);
     dict_write_tuplet(iter, &value);
     Tuplet acct = TupletInteger(KEY_ACCOUNT_ID, (int32_t)account_id[toSend]);
     dict_write_tuplet(iter, &acct);
     Tuplet msg = TupletCString(KEY_MSG_UUID, uuid_text[toSend]);
     dict_write_tuplet(iter,&msg);
     app_message_outbox_send();
  }
  else
  {
    layer_set_hidden(text_layer_get_layer(deleteConfirmLayer), true);
    mode = MODE_SCROLL;
    
    action_bar_layer_set_icon(action_bar, BUTTON_ID_UP, upArrow);  
    action_bar_layer_set_icon(action_bar, BUTTON_ID_DOWN, downArrow);
    action_bar_layer_set_icon(action_bar, BUTTON_ID_SELECT, trash);
  }
}

void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  Window *window = (Window *)context; // This context defaults to the window, but may be changed with \ref window_set_click_context.
  reschedule_kill_timer();
  
  if( mode == MODE_SCROLL )
  {
    GRect scrollFrame = layer_get_frame(scroll_layer_get_layer(scroll_layer));
    GPoint current = scroll_layer_get_content_offset(scroll_layer);
    int16_t yPos = (current.y / scrollFrame.size.h)*-1;

    yPos--;
  
    if( yPos < 0 )
      yPos = 0;
  
    scroll_layer_set_content_offset(scroll_layer, GPoint(0,yPos*scrollFrame.size.h*-1), true);
  }
  else if( mode == MODE_ACTION )
  {
     // POST REPLY1 ACTION TO PHONE
     GRect scrollFrame = layer_get_frame(scroll_layer_get_layer(scroll_layer));
     GPoint current = scroll_layer_get_content_offset(scroll_layer);
     int16_t yPos = (current.y / scrollFrame.size.h)*-1;
    
     int8_t toSend = nextWriteIndex-1;
     if( toSend < 0 )
        toSend = MAX_MESSAGES-1;

     while( yPos > 0 )
     {
       yPos--;
       toSend--;
       if( toSend < 0 )
         toSend = MAX_MESSAGES-1;
     }

     DictionaryIterator *iter;
     app_message_outbox_begin(&iter);
     Tuplet value = TupletInteger(KEY_CMD, VAL_CMD_REPLY1);
     dict_write_tuplet(iter, &value);
     Tuplet acct = TupletInteger(KEY_ACCOUNT_ID, (int32_t)account_id[toSend]);
     dict_write_tuplet(iter, &acct);
     Tuplet msg = TupletCString(KEY_MSG_UUID, uuid_text[toSend]);
     dict_write_tuplet(iter,&msg);
     app_message_outbox_send();
  }
  else
  {
    layer_set_hidden(text_layer_get_layer(deleteConfirmLayer), true);
    mode = MODE_SCROLL;
    
    action_bar_layer_set_icon(action_bar, BUTTON_ID_UP, upArrow);  
    action_bar_layer_set_icon(action_bar, BUTTON_ID_DOWN, downArrow);
    action_bar_layer_set_icon(action_bar, BUTTON_ID_SELECT, trash);
  }
}

void middle_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  reschedule_kill_timer();
  
  GRect scrollFrame = layer_get_frame(scroll_layer_get_layer(scroll_layer));
  GPoint current = scroll_layer_get_content_offset(scroll_layer);
  int16_t yPos = (current.y / scrollFrame.size.h)*-1;
    
  int8_t toSend = nextWriteIndex-1;
  if( toSend < 0 )
    toSend = MAX_MESSAGES-1;

  while( yPos > 0 )
  {
    yPos--;
    toSend--;
    if( toSend < 0 )
      toSend = MAX_MESSAGES-1;
  }

  if( mode == MODE_SCROLL )
  {
    mode = MODE_DELETE_CONFIRM;
    layer_set_hidden(text_layer_get_layer(deleteConfirmLayer), false);
  }
  else if( mode == MODE_ACTION )
  {
    // POST OPEN TO PHONE
     DictionaryIterator *iter;
     app_message_outbox_begin(&iter);
     Tuplet value = TupletInteger(KEY_CMD, VAL_CMD_OPEN);
     dict_write_tuplet(iter, &value);
     Tuplet acct = TupletInteger(KEY_ACCOUNT_ID, (int32_t)account_id[toSend]);
     dict_write_tuplet(iter, &acct);
     Tuplet msg = TupletCString(KEY_MSG_UUID, uuid_text[toSend]);
     dict_write_tuplet(iter,&msg);
     app_message_outbox_send();
  }
  else
  {
     // POST DELETE TO PHONE  
     DictionaryIterator *iter;
     app_message_outbox_begin(&iter);
     Tuplet value = TupletInteger(KEY_CMD, VAL_CMD_DELETE);
     dict_write_tuplet(iter, &value);
     Tuplet acct = TupletInteger(KEY_ACCOUNT_ID, (int32_t)account_id[toSend]);
     dict_write_tuplet(iter, &acct);
     Tuplet msg = TupletCString(KEY_MSG_UUID, uuid_text[toSend]);
     dict_write_tuplet(iter,&msg);
     app_message_outbox_send();

    // Go back to standard scroll mode
    layer_set_hidden(text_layer_get_layer(deleteConfirmLayer), true);
    mode = MODE_SCROLL;
    
    action_bar_layer_set_icon(action_bar, BUTTON_ID_UP, upArrow);  
    action_bar_layer_set_icon(action_bar, BUTTON_ID_DOWN, downArrow);
    action_bar_layer_set_icon(action_bar, BUTTON_ID_SELECT, trash);
  }
}

void click_config_provider(void *context) {
 // single click / repeat-on-hold config:
  window_single_click_subscribe(BUTTON_ID_DOWN, down_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_SELECT, middle_single_click_handler);
  window_single_click_subscribe(BUTTON_ID_BACK, back_single_click_handler);
}


void handle_kill_timer(void *data)
{
    kill_timer = NULL;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Closing app");
    window_stack_pop_all(true);  
}

void load_messages()
{
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Loading current messages...");

  header_time[0] = persist_read_int(0x10);
  persist_read_string(0x11,scroll_text[0],MAX_TEXT_LENGTH);
  persist_read_string(0x12,from_text[0],MAX_TEXT_LENGTH);
  persist_read_string(0x13,subject_text[0],MAX_TEXT_LENGTH);
  persist_read_string(0x14,uuid_text[0],MAX_TEXT_LENGTH);
  account_id[0] = persist_read_int(0x15);

  header_time[1] = persist_read_int(0x20);
  persist_read_string(0x21,scroll_text[1],MAX_TEXT_LENGTH);
  persist_read_string(0x22,from_text[1],MAX_TEXT_LENGTH);
  persist_read_string(0x23,subject_text[1],MAX_TEXT_LENGTH);
  persist_read_string(0x24,uuid_text[1],MAX_TEXT_LENGTH);
  account_id[1] = persist_read_int(0x25);

  header_time[2] = persist_read_int(0x30);
  persist_read_string(0x31,scroll_text[2],MAX_TEXT_LENGTH);
  persist_read_string(0x32,from_text[2],MAX_TEXT_LENGTH);
  persist_read_string(0x33,subject_text[2],MAX_TEXT_LENGTH);
  persist_read_string(0x34,uuid_text[2],MAX_TEXT_LENGTH);
  account_id[2] = persist_read_int(0x35);

  header_time[3] = persist_read_int(0x40);
  persist_read_string(0x41,scroll_text[3],MAX_TEXT_LENGTH);
  persist_read_string(0x42,from_text[3],MAX_TEXT_LENGTH);
  persist_read_string(0x43,subject_text[3],MAX_TEXT_LENGTH);
  persist_read_string(0x44,uuid_text[3],MAX_TEXT_LENGTH);
  account_id[3] = persist_read_int(0x45);

  header_time[4] = persist_read_int(0x50);
  persist_read_string(0x51,scroll_text[4],MAX_TEXT_LENGTH);
  persist_read_string(0x52,from_text[4],MAX_TEXT_LENGTH);
  persist_read_string(0x53,subject_text[4],MAX_TEXT_LENGTH);
  persist_read_string(0x54,uuid_text[4],MAX_TEXT_LENGTH);
  account_id[4] = persist_read_int(0x55);

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Load complete.");
}

//
// Handle the start-up of the app
//
static void do_init(void) {

  actions_enabled_val = actions_enabled();
  
  numMessagesFilled = 0;
  if (persist_exists(STORAGE_NUM_FILLED_KEY)) {
    numMessagesFilled = (uint8_t)persist_read_int(STORAGE_NUM_FILLED_KEY);
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Num messages filled: %i",numMessagesFilled);

  nextWriteIndex = 0;
  if( persist_exists(STORAGE_NEXT_INDEX_KEY) ) {
    nextWriteIndex = (uint8_t)persist_read_int(STORAGE_NEXT_INDEX_KEY);
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Next write index: %i",nextWriteIndex);

  utc_offset = persist_read_int(STORAGE_UTC_OFFSET_KEY);
  
  // Create our app's base window
  window = window_create();
  window_stack_push(window, true);
  window_set_background_color(window, GColorBlack);
  
  Layer *root_layer = window_get_root_layer(window);
  GRect frame = layer_get_frame(root_layer);

  GRect bounds = layer_get_frame(root_layer);

  // Initialize the scroll layer
  scroll_layer = scroll_layer_create(bounds);
  scroll_layer_set_shadow_hidden(scroll_layer, true);
  

  // This binds the scroll layer to the window so that up and down map to scrolling
  // You may use scroll_layer_set_callbacks to add or override interactivity
  //scroll_layer_set_click_config_onto_window(scroll_layer, window);

  master_text_layer = text_layer_create(GRect(0,0,bounds.size.w,5*bounds.size.h));
  text_layer_set_background_color(master_text_layer,GColorWhite);
  scroll_layer_add_child(scroll_layer,text_layer_get_layer(master_text_layer));
  
  bubble = gbitmap_create_with_resource(RESOURCE_ID_BUBBLE_BLACK);
  
  // Initialize the text layers
  for( int i = 0; i < MAX_MESSAGES; i++ )
  {
    strcpy(scroll_text[i],"....");
    strcpy(from_text[i],"...");
    strcpy(subject_text[i],"...");
  }
  load_messages();

  for( int i = 0; i < MAX_MESSAGES; i++ )
  {
    GRect textBounds = GRect(2,(i*bounds.size.h)+62,bounds.size.w-ACTION_BAR_WIDTH-5,bounds.size.h-18-46);
    GRect headerImageBounds = GRect(20,(i*bounds.size.h)+2,14,14);
    GRect headerLabelBounds = GRect(40,(i*bounds.size.h),bounds.size.w-50-5,18);
    GRect fromLabelBounds = GRect(2,(i*bounds.size.h)+16,bounds.size.w-ACTION_BAR_WIDTH-5,16);
    GRect subjectLabelBounds = GRect(2,(i*bounds.size.h)+32,bounds.size.w-ACTION_BAR_WIDTH-5,30);
    
    text_layer[i] = text_layer_create(textBounds);
    layer_set_clips(text_layer_get_layer(text_layer[i]), true);

    header_bubble_layer[i] = bitmap_layer_create(headerImageBounds);
    bitmap_layer_set_bitmap(header_bubble_layer[i], bubble);
    
    header_text_layer[i] = text_layer_create(headerLabelBounds);
    text_layer_set_font(header_text_layer[i], fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_background_color(header_text_layer[i], GColorClear);
    text_layer_set_text(header_text_layer[i], "Just Now");
    layer_set_clips(text_layer_get_layer(header_text_layer[i]), false);

    from_text_layer[i] = text_layer_create(fromLabelBounds);
    text_layer_set_font(from_text_layer[i], fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
    text_layer_set_background_color(from_text_layer[i], GColorClear);
    layer_set_clips(text_layer_get_layer(from_text_layer[i]), true);

    subject_text_layer[i] = text_layer_create(subjectLabelBounds);
    text_layer_set_font(subject_text_layer[i], fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
    text_layer_set_background_color(subject_text_layer[i], GColorClear);
    text_layer_set_overflow_mode(subject_text_layer[i], GTextOverflowModeFill);
    layer_set_clips(text_layer_get_layer(subject_text_layer[i]), true);

    
    // Change the font to a nice readable one
    // This is system font; you can inspect pebble_fonts.h for all system fonts
    // or you can take a look at feature_custom_font to add your own font
    text_layer_set_font(text_layer[i], fonts_get_system_font(FONT_KEY_GOTHIC_14));
    text_layer_set_overflow_mode(text_layer[i], GTextOverflowModeFill);
    
    // Add the layers for display
    scroll_layer_add_child(scroll_layer, text_layer_get_layer(text_layer[i]));
    scroll_layer_add_child(scroll_layer, text_layer_get_layer(header_text_layer[i]));
    scroll_layer_add_child(scroll_layer, bitmap_layer_get_layer(header_bubble_layer[i]));
    scroll_layer_add_child(scroll_layer, text_layer_get_layer(from_text_layer[i]));
    scroll_layer_add_child(scroll_layer, text_layer_get_layer(subject_text_layer[i]));    
  }

    if( numMessagesFilled == MAX_MESSAGES )
    {
      int8_t startIndex = nextWriteIndex-1;
      if( startIndex < 0 )
        startIndex = MAX_MESSAGES-1;
      
      int8_t iterator = startIndex;

      int8_t writeIndex = 0;
      while( writeIndex < MAX_MESSAGES )
      {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Updating UI index %d with data from %d...",writeIndex,iterator);

        APP_LOG(APP_LOG_LEVEL_DEBUG, "Setting from text to: %s",from_text[iterator]);

        time_t emailTime = header_time[iterator];
        time_t now = time(NULL)-utc_offset;
     
        time_t age = now - emailTime;
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Age of alert is %d seconds ago.",(int)age);
        
        if( age < 2*60 )
        {
          strcpy(header_text[iterator],"Just Now");
        }
        else if( age < 60*60 )
        {
          time_t minutes = age / 60;
          snprintf(header_text[iterator],MAX_TEXT_LENGTH,"%d Minutes Ago",(int)minutes);
        }
        else if( age < 2*60*60 )
        {
          strcpy(header_text[iterator],"An Hour Ago");
        }
        else if( age < 24*60*60 )
        {
          time_t hours = age/60/60;
          snprintf(header_text[iterator],MAX_TEXT_LENGTH,"%d Hours Ago",(int)hours);
        }
        else
        {
          time_t days = age/60/60/24;
          snprintf(header_text[iterator],MAX_TEXT_LENGTH,"%d Days Ago",(int)days);
        }

        text_layer_set_text(from_text_layer[writeIndex],from_text[iterator]);
        text_layer_set_text(subject_text_layer[writeIndex],subject_text[iterator]);
        text_layer_set_text(text_layer[writeIndex],scroll_text[iterator]);
        text_layer_set_text(header_text_layer[writeIndex], header_text[iterator]);

        writeIndex++;
        iterator--;
        if( iterator < 0 ) 
          iterator = MAX_MESSAGES-1;
      }
    }
    else
    {
      int8_t writeIndex = 0;
      for( int8_t i = numMessagesFilled-1; i >= 0; i-- )
      {
        time_t emailTime = header_time[i];
        time_t now = time(NULL)-utc_offset;
     
        time_t age = now - emailTime;
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Age of alert is %d seconds ago.",(int)age);

        if( age < 2*60 )
        {
          strcpy(header_text[i],"Just Now");
        }
        else if( age < 60*60 )
        {
          time_t minutes = age / 60;
          snprintf(header_text[i],MAX_TEXT_LENGTH,"%d Minutes Ago",(int)minutes);
        }
        else if( age < 2*60*60 )
        {
          strcpy(header_text[i],"An Hour Ago");
        }
        else if( age < 24*60*60 )
        {
          time_t hours = age/60/60;
          snprintf(header_text[i],MAX_TEXT_LENGTH,"%d Hours Ago",(int)hours);
        }
        else
        {
          time_t days = age/60/60/24;
          snprintf(header_text[i],MAX_TEXT_LENGTH,"%d Days Ago",(int)days);
        }

        text_layer_set_text(from_text_layer[writeIndex],from_text[i]);
        text_layer_set_text(subject_text_layer[writeIndex],subject_text[i]);
        text_layer_set_text(text_layer[writeIndex],scroll_text[i]);
        text_layer_set_text(header_text_layer[writeIndex], header_text[i]);
        writeIndex++;
      }
    }    

  // Trim text layer and scroll content to fit text box
  scroll_layer_set_content_size(scroll_layer, GSize(bounds.size.w, 5*bounds.size.h));

  layer_add_child(root_layer, scroll_layer_get_layer(scroll_layer));
  
  // Initialize the action bar:
  action_bar = action_bar_layer_create();
  // Associate the action bar with the window:
  action_bar_layer_add_to_window(action_bar, window);
  // Set the click config provider:
  action_bar_layer_set_click_config_provider(action_bar,
                                             click_config_provider);
  
  upArrow = gbitmap_create_with_resource(RESOURCE_ID_UP_ARROW_BLACK);
  action_bar_layer_set_icon(action_bar, BUTTON_ID_UP, upArrow);
  
  downArrow = gbitmap_create_with_resource(RESOURCE_ID_DOWN_ARROW_BLACK);
  action_bar_layer_set_icon(action_bar, BUTTON_ID_DOWN, downArrow);
  
  trash = gbitmap_create_with_resource(RESOURCE_ID_TRASH_BLACK);
  trashWhite = gbitmap_create_with_resource(RESOURCE_ID_TRASH_WHITE);
  action_bar_layer_set_icon(action_bar, BUTTON_ID_SELECT, trash);
  
  reply1 = gbitmap_create_with_resource(RESOURCE_ID_REPLY_1_BLACK);
  reply2 = gbitmap_create_with_resource(RESOURCE_ID_REPLY_2_BLACK);
  open = gbitmap_create_with_resource(RESOURCE_ID_OPEN_BLACK);
  
  mode = MODE_SCROLL;

  deleteConfirmLayer = text_layer_create(GRect((bounds.size.w/2)-45,(bounds.size.h/2)-45,70,90));
  text_layer_set_background_color(deleteConfirmLayer, GColorBlack);
  layer_set_hidden(text_layer_get_layer(deleteConfirmLayer), true);
  
  question = gbitmap_create_with_resource(RESOURCE_ID_QUESTION_WHITE);
  trashImageLayer = bitmap_layer_create(GRect(28,5,15,15));
  bitmap_layer_set_bitmap(trashImageLayer, trashWhite);
  
  questionImageLayer = bitmap_layer_create(GRect(28,25,15,15));
  bitmap_layer_set_bitmap(questionImageLayer, question);
  
  pressAgainTextLayer = text_layer_create(GRect(2,40,66,45));
  text_layer_set_background_color(pressAgainTextLayer, GColorBlack);
  text_layer_set_text_color(pressAgainTextLayer, GColorWhite);
  text_layer_set_font(pressAgainTextLayer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text(pressAgainTextLayer, "Press Again To Delete");
  text_layer_set_text_alignment(pressAgainTextLayer, GTextAlignmentCenter);
  
  layer_add_child(text_layer_get_layer(deleteConfirmLayer), text_layer_get_layer(pressAgainTextLayer));
  layer_add_child(text_layer_get_layer(deleteConfirmLayer), bitmap_layer_get_layer(trashImageLayer));
  layer_add_child(text_layer_get_layer(deleteConfirmLayer), bitmap_layer_get_layer(questionImageLayer));
  layer_add_child(root_layer, text_layer_get_layer(deleteConfirmLayer));
  
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_register_outbox_sent(out_sent_handler);
  app_message_register_outbox_failed(out_failed_handler);
  
  const int inbound_size = 120;
  const int outbound_size = 120;
  app_message_open(inbound_size, outbound_size);   
  
  kill_timer = app_timer_register(30*1000, handle_kill_timer, NULL);
}

static void check_persist_size()
{
  int size = 0;
  
  size += persist_get_size(STORAGE_NUM_FILLED_KEY);
  size += persist_get_size(STORAGE_NEXT_INDEX_KEY);
  size += persist_get_size(STORAGE_ACTION_ENABLED_KEY);
  size += persist_get_size(STORAGE_UTC_OFFSET_KEY);
  size += persist_get_size(0x10);
  size += persist_get_size(0x11);
  size += persist_get_size(0x12);
  size += persist_get_size(0x13);
  size += persist_get_size(0x14);
  size += persist_get_size(0x20);
  size += persist_get_size(0x21);
  size += persist_get_size(0x22);
  size += persist_get_size(0x23);
  size += persist_get_size(0x24);
  size += persist_get_size(0x30);
  size += persist_get_size(0x31);
  size += persist_get_size(0x32);
  size += persist_get_size(0x33);
  size += persist_get_size(0x34);
  size += persist_get_size(0x40);
  size += persist_get_size(0x41);
  size += persist_get_size(0x42);
  size += persist_get_size(0x43);
  size += persist_get_size(0x44);
  size += persist_get_size(0x50);
  size += persist_get_size(0x51);
  size += persist_get_size(0x52);
  size += persist_get_size(0x53);
  size += persist_get_size(0x54);
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Current storage: %d kb",size/1024);
}
static void do_deinit(void) {
  
  set_actions_enabled(actions_enabled_val);
  
  persist_delete(STORAGE_NUM_FILLED_KEY);
  persist_write_int(STORAGE_NUM_FILLED_KEY, (uint32_t)numMessagesFilled);
  
  persist_delete(STORAGE_NEXT_INDEX_KEY);
  persist_write_int(STORAGE_NEXT_INDEX_KEY, (uint32_t)nextWriteIndex);
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Storing storage values to memory - Num Filled(%d) Next Index(%d)",numMessagesFilled,nextWriteIndex);
  check_persist_size();
  
  action_bar_layer_destroy(action_bar);
  accel_tap_service_unsubscribe();
  tick_timer_service_unsubscribe();
  scroll_layer_destroy(scroll_layer);
  text_layer_destroy(master_text_layer);
  bitmap_layer_destroy(trashImageLayer);
  bitmap_layer_destroy(questionImageLayer);
  text_layer_destroy(deleteConfirmLayer);
  text_layer_destroy(pressAgainTextLayer);
  
  gbitmap_destroy(upArrow);
  gbitmap_destroy(downArrow);
  gbitmap_destroy(trash);
  gbitmap_destroy(reply1);
  gbitmap_destroy(reply2);
  gbitmap_destroy(open);
  gbitmap_destroy(question);
  gbitmap_destroy(trashWhite);
  gbitmap_destroy(bubble);
  
  if( kill_timer )
  {
    app_timer_cancel(kill_timer);
    kill_timer = NULL;
  }
  
  for( int i = 0; i < MAX_MESSAGES; i++ )
  {
    text_layer_destroy(text_layer[i]);
    text_layer_destroy(header_text_layer[i]);
    bitmap_layer_destroy(header_bubble_layer[i]);
    text_layer_destroy(from_text_layer[i]);
    text_layer_destroy(subject_text_layer[i]);
  }
  window_destroy(window);
}

// The main event/run loop for our app
int main(void) {
  do_init();
  app_event_loop();
  do_deinit();
}

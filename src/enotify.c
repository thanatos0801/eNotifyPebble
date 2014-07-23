// Standard includes
#include "pebble.h"

#define MAX_MESSAGES 5
#define MAX_TEXT_LENGTH 124
#define MAX_UUID_LENGTH 50

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
static GBitmap* deleted_bubble;

// The mode defines what the action bar commands will be and is one of ModeType
static uint8_t mode;

// Lorum ipsum to have something to scroll
static int32_t header_time[MAX_MESSAGES];
static uint32_t account_id[MAX_MESSAGES];
static uint8_t deleted[MAX_MESSAGES];
static char header_text[MAX_MESSAGES][MAX_TEXT_LENGTH];
static char scroll_text[MAX_MESSAGES][MAX_TEXT_LENGTH];
static char from_text[MAX_MESSAGES][MAX_TEXT_LENGTH];
static char subject_text[MAX_MESSAGES][MAX_TEXT_LENGTH];
static char uuid_text[MAX_MESSAGES][MAX_TEXT_LENGTH];

typedef struct app_data_t
{
  int num_messages_filled;
  int next_write_index;
  int utc_offset;
  int actions_enabled;
} app_data_t;
static app_data_t app_metadata;

typedef struct message_1_t
{
  int header_time;
  int account_id;
  uint8_t deleted;
  char uuid_text[MAX_TEXT_LENGTH-10];
  char scroll_text[MAX_TEXT_LENGTH];
}  __attribute__((__packed__)) message_1_t;

typedef struct message_2_t
{
  char from_text[MAX_TEXT_LENGTH];
  char subject_text[MAX_TEXT_LENGTH];
}  __attribute__((__packed__)) message_2_t;

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

//
// Handle persistent modes
//

void reschedule_kill_timer() {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Resetting kill timer");
  light_enable_interaction();
  if( kill_timer != NULL )
    app_timer_reschedule(kill_timer, 30*1000);
}

void persist_messages(int8_t index)
{
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Persisting current messages for index %d...",index);
  message_1_t msg1;
  message_2_t msg2;
  
  strcpy(msg1.scroll_text,"");
  strcpy(msg2.from_text,"");
  strcpy(msg2.subject_text,"");
  strcpy(msg1.uuid_text,"");
  
  if( index == -1 || index == 0 )
  {
    strcpy(msg1.scroll_text,scroll_text[0]);
    msg1.header_time = header_time[0];
    strcpy(msg2.from_text,from_text[0]);
    strcpy(msg2.subject_text,subject_text[0]);
    strcpy(msg1.uuid_text,uuid_text[0]);
    msg1.account_id = account_id[0];
    msg1.deleted = deleted[0];
    persist_write_data(0x10, &msg1, sizeof(message_1_t));
    persist_write_data(0x11, &msg2, sizeof(message_2_t));
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Saved message with subject: %s",msg2.subject_text);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Saved message text: %s",msg1.scroll_text);
  }
  
  if( index == -1 || index == 1 )
  {
    strcpy(msg1.scroll_text,scroll_text[1]);
    msg1.header_time = header_time[1];
    strcpy(msg2.from_text,from_text[1]);
    strcpy(msg2.subject_text,subject_text[1]);
    strcpy(msg1.uuid_text,uuid_text[1]);
    msg1.account_id = account_id[1];
    msg1.deleted = deleted[1];
    persist_write_data(0x20, &msg1, sizeof(message_1_t));
    persist_write_data(0x21, &msg2, sizeof(message_2_t));
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Saved message with subject: %s",msg2.subject_text);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Saved message text: %s",msg1.scroll_text);
  }
  
  if( index == -1 || index == 2 )
  {
    strcpy(msg1.scroll_text,scroll_text[2]);
    msg1.header_time = header_time[2];
    strcpy(msg2.from_text,from_text[2]);
    strcpy(msg2.subject_text,subject_text[2]);
    strcpy(msg1.uuid_text,uuid_text[2]);
    msg1.account_id = account_id[2];
    msg1.deleted = deleted[2];
    persist_write_data(0x30, &msg1, sizeof(message_1_t));
    persist_write_data(0x31, &msg2, sizeof(message_2_t));
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Saved message with subject: %s",msg2.subject_text);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Saved message text: %s",msg1.scroll_text);
  }
  
  if( index == -1 || index == 3 )
  {
    strcpy(msg1.scroll_text,scroll_text[3]);
    msg1.header_time = header_time[3];
    strcpy(msg2.from_text,from_text[3]);
    strcpy(msg2.subject_text,subject_text[3]);
    strcpy(msg1.uuid_text,uuid_text[3]);
    msg1.account_id = account_id[3];
    msg1.deleted = deleted[3];
    persist_write_data(0x40, &msg1, sizeof(message_1_t));
    persist_write_data(0x41, &msg2, sizeof(message_2_t));
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Saved message with subject: %s",msg2.subject_text);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Saved message text: %s",msg1.scroll_text);
  }
  
  if( index == -1 || index == 4 )
  {
    strcpy(msg1.scroll_text,scroll_text[4]);
    msg1.header_time = header_time[4];
    strcpy(msg2.from_text,from_text[4]);
    strcpy(msg2.subject_text,subject_text[4]);
    strcpy(msg1.uuid_text,uuid_text[4]);
    msg1.account_id = account_id[4];
    msg1.deleted = deleted[4];
    persist_write_data(0x50, &msg1, sizeof(message_1_t));
    persist_write_data(0x51, &msg2, sizeof(message_2_t));
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Saved message with subject: %s",msg2.subject_text);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Saved message text: %s",msg1.scroll_text);
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
   APP_LOG(APP_LOG_LEVEL_DEBUG, "Failed to send outgoing message: %d",reason);
   switch(reason)
     {
     case APP_MSG_ALREADY_RELEASED:
     APP_LOG(APP_LOG_LEVEL_DEBUG, "Already Released");
     break;
     
     case APP_MSG_BUFFER_OVERFLOW:
     APP_LOG(APP_LOG_LEVEL_DEBUG, "Buffer Overflow");
     break;
     
     case APP_MSG_BUSY:
     APP_LOG(APP_LOG_LEVEL_DEBUG, "Busy");
     break;
     
     case APP_MSG_INVALID_ARGS:
     APP_LOG(APP_LOG_LEVEL_DEBUG, "Invalid Args");
     break;
     
     case APP_MSG_NOT_CONNECTED:
     APP_LOG(APP_LOG_LEVEL_DEBUG, "Not Connected");
     break;
     
    case APP_MSG_OUT_OF_MEMORY:
     APP_LOG(APP_LOG_LEVEL_DEBUG, "Out of Memory");
     break;
     
     case APP_MSG_SEND_REJECTED:
     APP_LOG(APP_LOG_LEVEL_DEBUG, "Send Rejected");
     break;
     
     case APP_MSG_SEND_TIMEOUT:
     APP_LOG(APP_LOG_LEVEL_DEBUG, "Send Timeout");
     break;
     
     default:
     break;
   }
}

void refresh_screen() {
    if( app_metadata.num_messages_filled == MAX_MESSAGES )
    {
      int8_t startIndex = app_metadata.next_write_index-1;
      if( startIndex < 0 )
        startIndex = MAX_MESSAGES-1;
      
      int8_t iterator = startIndex;

      int8_t writeIndex = 0;
      while( writeIndex < MAX_MESSAGES )
      {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Updating UI index %d with data from %d...",writeIndex,iterator);

        time_t emailTime = header_time[iterator];
        time_t now = time(NULL)-app_metadata.utc_offset;
     
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

        if( deleted[iterator] )
          bitmap_layer_set_bitmap(header_bubble_layer[writeIndex], deleted_bubble);
        else
          bitmap_layer_set_bitmap(header_bubble_layer[writeIndex], bubble);
        
        writeIndex++;
        iterator--;
        if( iterator < 0 ) 
          iterator = MAX_MESSAGES-1;
      }
    }
    else
    {
      int8_t writeIndex = 0;
      for( int8_t i = app_metadata.num_messages_filled-1; i >= 0; i-- )
      {
        time_t emailTime = header_time[i];
        time_t now = time(NULL)-app_metadata.utc_offset;
     
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
        if( deleted[i] )
          bitmap_layer_set_bitmap(header_bubble_layer[writeIndex], deleted_bubble);
        else
          bitmap_layer_set_bitmap(header_bubble_layer[writeIndex], bubble);

        writeIndex++;
      }
    }  
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
    app_metadata.utc_offset = offset_tuple->value->int32;
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

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Copying message data into buffers at index %d...",app_metadata.next_write_index);
    
    header_time[app_metadata.next_write_index] = time_tuple->value->int32;
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Timestamp on message is %d.",(int)header_time[app_metadata.next_write_index]);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Now is %d.",(int)time(NULL));
    strcpy(uuid_text[app_metadata.next_write_index],uuid_tuple->value->cstring);
    strcpy(from_text[app_metadata.next_write_index],from_tuple->value->cstring);
    strcpy(subject_text[app_metadata.next_write_index],subject_tuple->value->cstring);
    strcpy(scroll_text[app_metadata.next_write_index],"...");
    account_id[app_metadata.next_write_index] = account_id_tuple->value->uint32;
    deleted[app_metadata.next_write_index] = 0;
    
    persist_messages(app_metadata.next_write_index);
 
    app_metadata.next_write_index++;
    if( app_metadata.next_write_index >  MAX_MESSAGES-1 )
      app_metadata.next_write_index = 0;
    app_metadata.num_messages_filled++;
    if( app_metadata.num_messages_filled > MAX_MESSAGES )
      app_metadata.num_messages_filled = MAX_MESSAGES;

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Updating UI text layers...");
    refresh_screen();  
    
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Total messages stored is now %d", app_metadata.num_messages_filled);
  }
  else if( uuid_tuple && text_tuple) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Found email body data for email UUID: %s", uuid_tuple->value->cstring);    
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Found email body data: %s", text_tuple->value->cstring);

    int8_t toWrite = app_metadata.next_write_index-1;
    if( toWrite < 0 )
      toWrite = MAX_MESSAGES-1;

    if( strcmp(uuid_tuple->value->cstring,uuid_text[toWrite]) == 0 )
    {
      TextLayer* bodyText = text_layer[0];
      
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Copying email body into buffer...");

      strcpy(scroll_text[toWrite],text_tuple->value->cstring);
      persist_messages(toWrite);
      
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Updated UI text layers...");
      text_layer_set_text(bodyText, scroll_text[toWrite]);
    }
  }
  else if( action_support_tuple ) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Found action enable message: %d", action_support_tuple->value->int8);
    app_metadata.actions_enabled = action_support_tuple->value->int8;
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
  if( mode == MODE_SCROLL && app_metadata.actions_enabled == 1 )
  {
    reschedule_kill_timer();
    mode = MODE_ACTION;
    
    action_bar_layer_set_icon(action_bar, BUTTON_ID_UP, reply1);
    action_bar_layer_set_icon(action_bar, BUTTON_ID_DOWN, reply2);
    action_bar_layer_set_icon(action_bar, BUTTON_ID_SELECT, open);
  }
  else if( app_metadata.actions_enabled == 1 )
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
  
    if( yPos > app_metadata.num_messages_filled-1 )
      yPos = app_metadata.num_messages_filled-1;
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
    
     int8_t toSend = app_metadata.next_write_index-1;
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
    
     int8_t toSend = app_metadata.next_write_index-1;
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
    
  int8_t toSend = app_metadata.next_write_index-1;
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
    
     deleted[toSend] = 1;
     persist_messages(toSend);
     refresh_screen();

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
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Loading current messages...sizeof(message_1_t)=%d, sizeof(message_2_t)=%d",sizeof(message_1_t),sizeof(message_2_t));

  message_1_t msg1;
  message_2_t msg2;

  if( persist_exists(0x10) )
  {
  persist_read_data(0x10, &msg1, sizeof(message_1_t));
  persist_read_data(0x11, &msg2, sizeof(message_2_t));
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Found message from: %s",msg2.from_text);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Subject: %s",msg2.subject_text);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Text: %s",msg1.scroll_text);
  header_time[0] = msg1.header_time;
  strcpy(scroll_text[0],msg1.scroll_text);
  strcpy(from_text[0],msg2.from_text);
  strcpy(subject_text[0],msg2.subject_text);
  strcpy(uuid_text[0],msg1.uuid_text);
  account_id[0] = msg1.account_id;
  deleted[0] = msg1.deleted;
  }
  
  if( persist_exists(0x20) )
  {
  persist_read_data(0x20, &msg1, sizeof(message_1_t));
  persist_read_data(0x21, &msg2, sizeof(message_2_t));
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Found message from: %s",msg2.from_text);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Subject: %s",msg2.subject_text);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Text: %s",msg1.scroll_text);

  header_time[1] = msg1.header_time;
  strcpy(scroll_text[1],msg1.scroll_text);
  strcpy(from_text[1],msg2.from_text);
  strcpy(subject_text[1],msg2.subject_text);
  strcpy(uuid_text[1],msg1.uuid_text);
  account_id[1] = msg1.account_id;
  deleted[1] = msg1.deleted;
  }
  
  if( persist_exists(0x30) )
  {
  persist_read_data(0x30, &msg1, sizeof(message_1_t));
  persist_read_data(0x31, &msg2, sizeof(message_2_t));
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Found message from: %s",msg2.from_text);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Subject: %s",msg2.subject_text);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Text: %s",msg1.scroll_text);

  header_time[2] = msg1.header_time;
  strcpy(scroll_text[2],msg1.scroll_text);
  strcpy(from_text[2],msg2.from_text);
  strcpy(subject_text[2],msg2.subject_text);
  strcpy(uuid_text[2],msg1.uuid_text);
  account_id[2] = msg1.account_id;
  deleted[2] = msg1.deleted;
  }
  
  if( persist_exists(0x40) )
  {
  persist_read_data(0x40, &msg1, sizeof(message_1_t));
  persist_read_data(0x41, &msg2, sizeof(message_2_t));
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Found message from: %s",msg2.from_text);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Subject: %s",msg2.subject_text);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Text: %s",msg1.scroll_text);

  header_time[3] = msg1.header_time;
  strcpy(scroll_text[3],msg1.scroll_text);
  strcpy(from_text[3],msg2.from_text);
  strcpy(subject_text[3],msg2.subject_text);
  strcpy(uuid_text[3],msg1.uuid_text);
  account_id[3] = msg1.account_id;
  deleted[3] = msg1.deleted;
  }
  
  if( persist_exists(0x50) )
  {
  persist_read_data(0x50, &msg1, sizeof(message_1_t));
  persist_read_data(0x51, &msg2, sizeof(message_2_t));
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Found message from: %s",msg2.from_text);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Subject: %s",msg2.subject_text);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Text: %s",msg1.scroll_text);

  header_time[4] = msg1.header_time;
  strcpy(scroll_text[4],msg1.scroll_text);
  strcpy(from_text[4],msg2.from_text);
  strcpy(subject_text[4],msg2.subject_text);
  strcpy(uuid_text[4],msg1.uuid_text);
  account_id[4] = msg1.account_id;
  deleted[4] = msg1.deleted;
  }
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Load complete.");
}

//
// Handle the start-up of the app
//
static void do_init(void) {

  if( persist_exists(0x0) )
  {
    persist_read_data(0x0, &app_metadata, sizeof(app_data_t));
  }
  else
  {
    app_metadata.num_messages_filled = 0;
    app_metadata.next_write_index = 0;
    app_metadata.utc_offset = 0;
    app_metadata.actions_enabled = 0;
  }
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Num messages filled: %i",app_metadata.num_messages_filled);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Next write index: %i",app_metadata.next_write_index);

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
  deleted_bubble = gbitmap_create_with_resource(RESOURCE_ID_DELETED_BLACK);

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

  refresh_screen();

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
  
  if( persist_exists(0x0) )
    size = size + persist_get_size(0x0);
  if( persist_exists(0x10) )
    size = size + persist_get_size(0x10);
  if( persist_exists(0x20) )
    size = size + persist_get_size(0x20);
  if( persist_exists(0x30) )
    size = size + persist_get_size(0x30);
  if( persist_exists(0x40) )
    size = size + persist_get_size(0x40);
  if( persist_exists(0x50) )
    size = size + persist_get_size(0x50);
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Current storage: %d b",size);
}

static void do_deinit(void) {
  
  persist_write_data(0x0, &app_metadata, sizeof(app_data_t));
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Stored storage values to memory - Num Filled(%d) Next Index(%d)",app_metadata.num_messages_filled,app_metadata.next_write_index);
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
  gbitmap_destroy(deleted_bubble);
  
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


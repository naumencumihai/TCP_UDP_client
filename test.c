#include "helpers.h"

void remove_message(struct queued_message *array, int index, int array_length) {
  int i;
  for(i = index; i < array_length - 1; i++) {
    array[i] = array[i + 1];
  }
  struct queued_message null_msg;
  array[array_length - 1] = null_msg;
}

int main(void) {

  struct queued_message msg1;
  struct queued_message msg2;
  struct queued_message msg3;

  struct queued_message queued_messages[1000];

  strcpy(msg1.topic, "topic1");
  msg1.data_type = 1;
  strcpy(msg1.content, "conten1t");
  strcpy(msg1.id, "gigel");

  strcpy(msg2.topic, "topic2");
  msg2.data_type = 1;
  strcpy(msg2.content, "content2");
   strcpy(msg2.id, "mirel");

  strcpy(msg3.topic, "topic3");
  msg3.data_type = 0;
  strcpy(msg3.content, "content3");
   strcpy(msg3.id, "ion");

  queued_messages[0] = msg1;
  queued_messages[1] = msg2;
  queued_messages[2] = msg3;
  
  for (int i = 0; i < 3; i++) {
    printf("topic: %s; dt: %d; content: %s; id: %s\n",
            queued_messages[i].topic, queued_messages[i].data_type,
            queued_messages[i].content, queued_messages[i].id);
  }

  remove_message(queued_messages, 1, 3);

  for (int i = 0; i < 2; i++) {
    printf("topic: %s; dt: %d; content: %s; id: %s\n",
            queued_messages[i].topic, queued_messages[i].data_type,
            queued_messages[i].content, queued_messages[i].id);
  }

  return 0;
}
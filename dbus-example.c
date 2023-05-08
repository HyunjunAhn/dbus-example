/*
 * Example low-level D-Bus code.
 * Written by Matthew Johnson <dbus@matthew.ath.cx>
 *
 * This code has been released into the Public Domain.
 * You may do whatever you like with it.
 *
 * Subsequent tweaks by Will Ware <wware@alum.mit.edu>
 * Still in the public domain.
 */
#include <dbus/dbus.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

//#define SIGNAL_OBJ0 "/test/signal/Object0"
//#define SIGNAL_OBJ1 "/test/signal/Object1"
#define SIGNAL_OBJ0 "/CMD00"
#define SIGNAL_OBJ1 "/CMD01"
#define SIGNAL_IFACE0 "test.iface0"
#define SIGNAL_IFACE1 "test.iface1"
#define SIGNAL_SOURCE0 "test.iface0.source"
#define SIGNAL_SOURCE1 "test.iface1.source"
#define SIGNAL_SINK0 "test.iface0.sink"
#define SIGNAL_SINK1 "test.iface1.sink"

#define METHOD_IFACE "test.method.Type"

#define MSG_NAME0 "msg003\0"
#define MSG_NAME1 "msg005\0"
#define MSG_NAME2 "msg000\0"

typedef struct 
{
   DBusError err;
   DBusConnection* conn;
   char iface[100];
   char sigName[100];
} dbus_session_init_params;

typedef struct 
{
   int type;
   void *value;
} msgField_t;

typedef struct 
{
   char msgName[7];
   int msgFiledCnt;
   msgField_t *msgField;
} msgStruct_t;


void prtSendLog(DBusMessage* msg, msgField_t *msgField)
{
   if(msgField->type == DBUS_TYPE_STRING)
   {
      printf("Signal Sent with value [ %s ] (path:%s, iface:%s, msg:%s)\n", (char *)msgField->value, 
      dbus_message_get_path(msg), dbus_message_get_interface(msg), 
      dbus_message_get_member(msg));
   }
   else if(msgField->type == DBUS_TYPE_INT16)
   {
      printf("Signal Sent with value [ %d ] (path:%s, iface:%s, msg:%s)\n", *(__int16_t *)msgField->value, 
      dbus_message_get_path(msg), dbus_message_get_interface(msg), 
      dbus_message_get_member(msg));
   }
   else if(msgField->type == DBUS_TYPE_INT32)
   {
      printf("Signal Sent with value [ %d ] (path:%s, iface:%s, msg:%s)\n", *(__int32_t *)msgField->value, 
      dbus_message_get_path(msg), dbus_message_get_interface(msg), 
      dbus_message_get_member(msg));
   }
}

void *getPtrOfVal(msgField_t *msgField)
{
   void *p;
   if(msgField->type == DBUS_TYPE_STRING)
   {
      p = &msgField->value;
   }
   else
   {
      p = msgField->value;
   }
   return p;
}

/**
 * Connect to the DBUS bus and send a broadcast signal
 */
void sendStruMsg(dbus_session_init_params *dbus, char *obj, msgStruct_t *msgStruct)
{
   DBusMessage* msg;
   DBusMessageIter args;
   DBusConnection* conn = dbus->conn;
   dbus_uint32_t serial = 0;
   int msgMemberCnt = msgStruct->msgFiledCnt;
   msgField_t *msgField = msgStruct->msgField; 

   // create a signal & check for errors
   msg = dbus_message_new_signal(obj, // object name of the signal
                                 dbus->iface, // interface name of the signal
                                 msgStruct->msgName); // name of the signal
   if (NULL == msg)
   {
      fprintf(stderr, "Message Null\n");
      exit(1);
   }

   // append arguments onto signal
   dbus_message_iter_init_append(msg, &args);

   for(int i=0;i<msgMemberCnt;i++)
   {
      if (!dbus_message_iter_append_basic(&args, msgField[i].type, getPtrOfVal(&msgField[i]))) {
         fprintf(stderr, "Out Of Memory!\n");
         exit(1);
      }
      prtSendLog(msg, &msgField[i]);
   }

   // send the message and flush the connection
   if (!dbus_connection_send(conn, msg, &serial)) {
      fprintf(stderr, "Out Of Memory!\n");
      exit(1);
   }
   dbus_connection_flush(conn);

   // free the message
   dbus_message_unref(msg);
}



/**
 * Call a method on a remote object
 */
void query(char* value)
{
   DBusMessage* msg;
   DBusMessageIter args;
   DBusConnection* conn;
   DBusError err;
   DBusPendingCall* pending;
   int ret;
   char* rcvdMsg = "";

   printf("[%s] message sent.\n", value);

   // initialiset the errors
   dbus_error_init(&err);

   // connect to the system bus and check for errors
   conn = dbus_bus_get_private(DBUS_BUS_SESSION, &err);
   if (dbus_error_is_set(&err)) {
      fprintf(stderr, "Connection Error (%s)\n", err.message);
      dbus_error_free(&err);
   }
   if (NULL == conn) {
      exit(1);
   }

   // request our name on the bus
   ret = dbus_bus_request_name(conn, "test.method.caller", DBUS_NAME_FLAG_REPLACE_EXISTING , &err);
   if (dbus_error_is_set(&err)) {
      fprintf(stderr, "Name Error (%s)\n", err.message);
      dbus_error_free(&err);
   }
   if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
      fprintf(stderr, "Not Primary Owner (%d)\n", ret);
      //exit(1);
   }

   // create a new method call and check for errors
   msg = dbus_message_new_method_call("test.method.server", // target for the method call
                                      SIGNAL_OBJ0, // object to call on
                                      METHOD_IFACE, // interface to call on
                                      "Method"); // method name
   if (NULL == msg) {
      fprintf(stderr, "Message Null\n");
      exit(1);
   }

   // append arguments
   dbus_message_iter_init_append(msg, &args);
   if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &value)) {
      fprintf(stderr, "Out Of Memory!\n");
      exit(1);
   }

   // send message and get a handle for a reply
   //if (!dbus_connection_send_with_reply (conn, msg, &pending, -1)) { // -1 is default timeout
   if (!dbus_connection_send_with_reply (conn, msg, &pending, 1500)) { // -1 is default timeout
      fprintf(stderr, "Out Of Memory!\n");
      exit(1);
   }
   if (NULL == pending) {
      fprintf(stderr, "Pending Call Null\n");
      exit(1);
   }
   dbus_connection_flush(conn);

   // free message
   dbus_message_unref(msg);

   // block until we recieve a reply
   dbus_pending_call_block(pending);

   // get the reply message
   msg = dbus_pending_call_steal_reply(pending);
   if (NULL == msg) {
      fprintf(stderr, "Reply Null\n");
      exit(1);
   }
   // free the pending message handle
   dbus_pending_call_unref(pending);

   // read the parameters
   /*
   bool stat;
   if (!dbus_message_iter_init(msg, &args))
      fprintf(stderr, "Message has no arguments!\n");
   else if (DBUS_TYPE_BOOLEAN != dbus_message_iter_get_arg_type(&args))
      fprintf(stderr, "Argument is not boolean!\n");
   else
      dbus_message_iter_get_basic(&args, &stat);

   if (!dbus_message_iter_next(&args))
      fprintf(stderr, "Message has too few arguments!\n");
   else if (DBUS_TYPE_UINT32 != dbus_message_iter_get_arg_type(&args))
      fprintf(stderr, "Argument is not int!\n");
   else
      dbus_message_iter_get_basic(&args, &level);

   printf("Got Reply: %d, %d\n", stat, level);
   */

   if (!dbus_message_iter_init(msg, &args))
      fprintf(stderr, "Message has no arguments!\n");
   else if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args))
      fprintf(stderr, "Argument is not string!\n");
   else
      dbus_message_iter_get_basic(&args, &rcvdMsg);
   printf("Got Reply [%s]\n", rcvdMsg);


   // free reply
   dbus_message_unref(msg);
}

void reply_to_method_call(DBusMessage* msg, DBusConnection* conn)
{
   DBusMessage* reply;
   DBusMessageIter args;
   dbus_uint32_t serial = 0;
   //char* param = "";
   char* recMsg = "";
   char repMsg[100] = "RPLY_MSG_FOR_";
   char *pRepMsg = repMsg;

   // read the arguments
   if (!dbus_message_iter_init(msg, &args))
      fprintf(stderr, "Message has no arguments!\n");
   else if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args))
      fprintf(stderr, "Argument is not string!\n");
   else
      dbus_message_iter_get_basic(&args, &recMsg);

   printf("[%s] message received!\n", recMsg);

   // create a reply from the message
   reply = dbus_message_new_method_return(msg);

   // add the arguments to the reply
   dbus_message_iter_init_append(reply, &args);

   /*
   bool stat = true;
   dbus_uint32_t level = 21614;
   if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &stat)) {
      fprintf(stderr, "Out Of Memory!\n");
      exit(1);
   }
   if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &level)) {
      fprintf(stderr, "Out Of Memory!\n");
      exit(1);
   }
   */
   strcat(repMsg, recMsg);
   if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &pRepMsg)) {
      fprintf(stderr, "Out Of Memory!\n");
      exit(1);
   }

   // send the reply && flush the connection
   if (!dbus_connection_send(conn, reply, &serial)) {
      fprintf(stderr, "Out Of Memory!\n");
      exit(1);
   }

   printf("Reply to [%s] with [%s]\n", recMsg, repMsg);

   dbus_connection_flush(conn);

   // free the reply
   dbus_message_unref(reply);
}

/**
 * Server that exposes a method call and waits for it to be called
 */
void listen()
{
   DBusMessage* msg;
   DBusConnection* conn;
   DBusError err;
   int ret;

   printf("Listening for method calls\n");

   // initialise the error
   dbus_error_init(&err);

   // connect to the bus and check for errors
   conn = dbus_bus_get_private(DBUS_BUS_SESSION, &err);
   if (dbus_error_is_set(&err)) {
      fprintf(stderr, "Connection Error (%s)\n", err.message);
      dbus_error_free(&err);
   }
   if (NULL == conn) {
      fprintf(stderr, "Connection Null\n");
      exit(1);
   }

   // request our name on the bus and check for errors
   ret = dbus_bus_request_name(conn, "test.method.server", DBUS_NAME_FLAG_REPLACE_EXISTING , &err);
   if (dbus_error_is_set(&err)) {
      fprintf(stderr, "Name Error (%s)\n", err.message);
      dbus_error_free(&err);
   }
   if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
      fprintf(stderr, "Not Primary Owner (%d)\n", ret);
      //exit(1);
   }

   // loop, testing for new messages
   while (true) {
      // non blocking read of the next available message
      dbus_connection_read_write(conn, 0);
      msg = dbus_connection_pop_message(conn);

      // loop again if we haven't got a message
      if (NULL == msg) {
         usleep(10000);
         continue;
      }

      // check this is a method call for the right interface & method
      if (dbus_message_is_method_call(msg, METHOD_IFACE, "Method"))
      {
         usleep(1000000); //For pending test
         reply_to_method_call(msg, conn);
      }

      // free the message
      dbus_message_unref(msg);
   }

}

void parseRcvdMsg(DBusMessage* msg, char *iface, char *rcvdSig)
{
   printf("Object Path: %s\n", dbus_message_get_path(msg));

   // check if the message is a signal from the correct interface and with the correct name
   // read the parameters
   DBusMessageIter args;
   char* sigvalue_str;
   int sigvalue_int32; 
   __int16_t sigvalue_int16; 
   __int8_t sigvalue_int8; 
   int nextValid = 1;
   if (!dbus_message_iter_init(msg, &args))
      printf("Message Has No Parameters\n");
   else
   {
      while(nextValid)
      {
         if (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&args))
         {
            dbus_message_iter_get_basic(&args, &sigvalue_int32);
            printf("Got Signal with value [ %-6d ] with %s-%s\n", sigvalue_int32, iface, rcvdSig);
         }
         else if (DBUS_TYPE_INT16 == dbus_message_iter_get_arg_type(&args))
         {
            dbus_message_iter_get_basic(&args, &sigvalue_int16);
            printf("Got Signal with value [ %-6d ] with %s-%s\n", sigvalue_int16, iface, rcvdSig);
         }
         else if (DBUS_TYPE_BYTE == dbus_message_iter_get_arg_type(&args))
         {
            dbus_message_iter_get_basic(&args, &sigvalue_int8);
            printf("Got Signal with value [ %-6d ] with %s-%s\n", sigvalue_int8, iface, rcvdSig);
         }
         else if (DBUS_TYPE_STRING == dbus_message_iter_get_arg_type(&args))
         {
            dbus_message_iter_get_basic(&args, &sigvalue_str);
            printf("Got Signal with value [ \'%s\' ] with %s-%s\n", sigvalue_str, iface, rcvdSig);
         }
         else
         {
            printf("Unknown type\n");
         }
         nextValid = dbus_message_iter_next(&args);
      }
   }

   // free the message
   dbus_message_unref(msg);
}

int receiveRoutin(dbus_session_init_params *dbus)
{
   DBusConnection* conn = dbus->conn;
   DBusMessage* msg;
   char *iface = dbus->iface;
   int ret = -1;

   // non blocking read of the next available message
   dbus_connection_read_write(conn, 0);
   msg = dbus_connection_pop_message(conn);

   // loop again if we haven't read a message
   if (NULL != msg) 
   {
      if (dbus_message_is_signal(msg, iface, MSG_NAME0))
      {
         parseRcvdMsg(msg, iface, MSG_NAME0);
      }
      else if (dbus_message_is_signal(msg, iface, MSG_NAME1))
      {
         parseRcvdMsg(msg, iface, MSG_NAME1);
      }
      else if (dbus_message_is_signal(msg, iface, MSG_NAME2))
      {
         parseRcvdMsg(msg, iface, MSG_NAME2);
      }
      else
      {
         /* No operation */
      }
      ret = 0;
   }
   return ret;
}

void dbusInit(dbus_session_init_params *dbus)
{
   DBusError *err = &dbus->err;
   int ret;
   DBusConnection** conn = &dbus->conn;
   char *sigName = dbus->sigName;

   // initialise the errors
   dbus_error_init(err);
   // connect to the bus and check for errors
   //*conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
   *conn = dbus_bus_get_private(DBUS_BUS_SESSION, err);
   if (dbus_error_is_set(err)) {
      fprintf(stderr, "Connection Error (%s)\n", err->message);
      dbus_error_free(err);
   }
   if (NULL == *conn) {
      printf("conn == NULL!\n");
      exit(1);
   }

   // request our name on the bus and check for errors
   ret = dbus_bus_request_name(*conn, sigName, DBUS_NAME_FLAG_REPLACE_EXISTING , err);
   if (dbus_error_is_set(err)) {
      fprintf(stderr, "Name Error (%s)\n", err->message);
      dbus_error_free(err);
   }
   if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
      printf("Not primary owner\n");
      //exit(1);
   }
   printf("Dbus Init - %s (conn:%p)\n", sigName, *conn);
}

void dbusRecvIfaceSet(dbus_session_init_params *dbus)
{
   DBusError *err = &dbus->err;
   char tmp[100];
   char *iface = dbus->iface;
   DBusConnection** conn = &dbus->conn;

   // add a rule for which messages we want to see
   sprintf(tmp, "type='signal',interface=\'%s\'", iface);
   dbus_bus_add_match(*conn, tmp, err); // see signals from the given interface
   dbus_connection_flush(*conn);
   if (dbus_error_is_set(err)) {
      fprintf(stderr, "Match Error, iface:%s (%s)\n", iface, err->message);
      exit(1);
   }
}

/**
 * Listens for signals on the bus
 */
void* receive(void *arg)
{
   int msg=-1;
   dbus_session_init_params *dbus = (dbus_session_init_params *)arg;

   // loop listening for signals being emmitted
   while (true) {
      msg = receiveRoutin(dbus);
      if(msg < 0) 
      {
         usleep(10000);
      }
   }
}

void usage()
{
   printf ("Syntax: dbus-example [send] [iface_num] [msgNum]\n");
   printf ("        dbus-example [receive|listen|query] [<param>]\n");
}

#define MSG_NUM_3 3
#define MSG_NUM_5 5
int main(int argc, char** argv)
{
   pthread_t tid;
   dbus_session_init_params *dbus_send;

   dbus_session_init_params dbus0_send = (dbus_session_init_params){{0}, NULL, SIGNAL_IFACE0, SIGNAL_SOURCE0};
   dbus_session_init_params dbus0_recv = (dbus_session_init_params){{0}, NULL, SIGNAL_IFACE0, SIGNAL_SINK0};
   dbus_session_init_params dbus1_send = (dbus_session_init_params){{0}, NULL, SIGNAL_IFACE1, SIGNAL_SOURCE1};
   dbus_session_init_params dbus1_recv = (dbus_session_init_params){{0}, NULL, SIGNAL_IFACE1, SIGNAL_SINK1};

//----------------------------------------------------------------
// Test case 0
   int tmpNum0 = 234;
   msgField_t d003[3] = { 
                          (msgField_t){DBUS_TYPE_STRING, "asdf"}, 
                          (msgField_t){DBUS_TYPE_INT32, &tmpNum0}, 
                          (msgField_t){DBUS_TYPE_STRING, "TEST!!!"} 
                        };
   msgStruct_t msg003 = (msgStruct_t){ MSG_NAME0, 3, d003 };

// Test case 1
   int tmpNum1 = 32767;
   int tmpNum2 = -256;
   int tmpNum3 = 127;
   msgField_t d005[3] = { 
                          (msgField_t){DBUS_TYPE_INT16, &tmpNum1}, 
                          (msgField_t){DBUS_TYPE_INT32, &tmpNum2},
                          (msgField_t){DBUS_TYPE_BYTE, &tmpNum3}
                        };
   msgStruct_t msg005 = (msgStruct_t){ MSG_NAME1, 3, d005 };
//----------------------------------------------------------------

   char* param = "no param";
   if (4 <= argc && NULL != argv[3]) param = argv[3];
   else if (3 <= argc && NULL != argv[2]) param = argv[2];
   else if (2 > argc)
   {
      usage();
   }
   else
   {
      /* No operation */
   }

   if (4 <= argc)
   {
      if (0 == strcmp(argv[1], "send"))
      {
         /* Select interface */
         if(atoi(argv[2]) == 0)
         {
            dbus_send = &dbus0_send;
         }
         else if(atoi(argv[2]) == 1)
         {
            dbus_send = &dbus1_send;
         }
         else
         {
            /* No operation */
         }

         /* Send message selected by message number */
         dbusInit(dbus_send);
         if(atoi(argv[3]) == MSG_NUM_3)
         {
            sendStruMsg(dbus_send , SIGNAL_OBJ0, &msg003);
         }
         else if(atoi(argv[3]) == MSG_NUM_5)
         {
            sendStruMsg(dbus_send , SIGNAL_OBJ0, &msg005);
         }
         else
         {
            printf("invalid message num\n");
            exit(0);
         }
      }
      else
      {
         usage();
      }
   }
   else if (0 == strcmp(argv[1], "receive"))
   {
      dbusInit(&dbus0_recv);
      dbusInit(&dbus1_recv);
      dbusRecvIfaceSet(&dbus0_recv);
      dbusRecvIfaceSet(&dbus1_recv);
   
      pthread_create(&tid, NULL, receive, (void *)&dbus0_recv);
      pthread_create(&tid, NULL, receive, (void *)&dbus1_recv);
      while (1)
      {
         usleep(100000);
      }
   }
   else if (0 == strcmp(argv[1], "listen"))
      listen();
   else if (0 == strcmp(argv[1], "query"))
      query(param);
   else 
   {
      usage();
   }
   return 0;
}
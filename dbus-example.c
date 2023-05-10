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

//#define DBUS_OBJ0 "/test/signal/Object0"
//#define DBUS_OBJ1 "/test/signal/Object1"
#define DBUS_OBJ0      "/CMD00"
#define DBUS_OBJ1      "/CMD01"
#define DBUS_IFACE0    "test.iface0"
#define DBUS_IFACE1    "test.iface1"

#define SIGNAL_SOURCE0 "test.signal0.source"
#define SIGNAL_SOURCE1 "test.signal1.source"
#define SIGNAL_SINK0   "test.signal0.sink"
#define SIGNAL_SINK1   "test.signal1.sink"

#define METHOD_SERVER0 "test.method0.server"
#define METHOD_SERVER1 "test.method1.server"
#define METHOD_CALLER0 "test.method0.caller"
#define METHOD_CALLER1 "test.method1.caller"

#define MSG_NAME0 "msg0003\0"
#define MSG_NAME1 "msg0005\0"
#define MSG_NAME2 "msg0009\0"
#define MSG_NAME3 "msg000A\0"

#define MSG_NUM_3 3
#define MSG_NUM_5 5
#define MSG_NUM_10 10 

typedef struct 
{
   DBusError err;
   DBusConnection* conn;
   char obj[100];
   char iface[100];
   char sigName[100];
   char *server;
   int pendingTimeMs;
} dbus_session_params;

typedef struct 
{
   int type;
   void *value;
} msgField_t;

typedef struct 
{
   char msgName[8];
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
   else if(msgField->type == DBUS_TYPE_BYTE)
   {
      printf("Signal Sent with value [ %d ] (path:%s, iface:%s, msg:%s)\n", *(__int8_t *)msgField->value, 
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

void buildMsg(dbus_session_params *dbus, char *obj, char *server, msgStruct_t *msgStruct, DBusMessage** msg, DBusMessageIter* args)
{
   int msgMemberCnt = msgStruct->msgFiledCnt;
   msgField_t *msgField = msgStruct->msgField; 

   // create a signal & check for errors
   if(server == NULL)
   {
      *msg = dbus_message_new_signal(obj, // object name of the signal
                                    dbus->iface, // interface name of the signal
                                    msgStruct->msgName); // name of the signal
   }
   else
   {
      *msg = dbus_message_new_method_call(server, // target for the method call
                                       obj, // object to call on
                                       dbus->iface, // interface to call on
                                       msgStruct->msgName); // method name
   }

   if (NULL == *msg)
   {
      fprintf(stderr, "Message Null\n");
      exit(1);
   }

   // append arguments onto signal
   dbus_message_iter_init_append(*msg, args);

   for(int i=0;i<msgMemberCnt;i++)
   {
      if (!dbus_message_iter_append_basic(args, msgField[i].type, getPtrOfVal(&msgField[i]))) {
         fprintf(stderr, "Out Of Memory!\n");
         exit(1);
      }
      prtSendLog(*msg, &msgField[i]);
   }
}


/**
 * Connect to the DBUS bus and send a broadcast signal or
 * Call a method on a remote object
 */
void sendMsg(dbus_session_params* dbus, msgStruct_t* msgStruct)
{
   DBusMessage* msg;
   DBusMessageIter args;
   char* obj = dbus->obj;
   DBusConnection* conn = dbus->conn;
   char* server = dbus->server;
   DBusPendingCall* pending;
   dbus_uint32_t serial = 0;
   char* rcvdMsg = "";

   buildMsg(dbus, obj, server, msgStruct, &msg, &args);

   // send message and get a handle for a reply
   //if (!dbus_connection_send_with_reply (conn, msg, &pending, -1)) { // -1 is default timeout
   if(server != NULL) // For Unicast
   {
      if (!dbus_connection_send_with_reply (conn, msg, &pending, dbus->pendingTimeMs)) { // -1 is default timeout
         fprintf(stderr, "Out Of Memory!\n");
         exit(1);
      }
      if (NULL == pending) {
         fprintf(stderr, "Pending Call Null\n");
         exit(1);
      }
   }
   else // For Broadcast
   {
      if (!dbus_connection_send(conn, msg, &serial)) {
         fprintf(stderr, "Out Of Memory!\n");
         exit(1);
      }
   }
   dbus_connection_flush(conn);

   // free message
   dbus_message_unref(msg);

   if(server != NULL) // For Unicast
   {
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

      if (!dbus_message_iter_init(msg, &args))
         fprintf(stderr, "Message has no arguments!\n");
      else if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args))
         fprintf(stderr, "Argument is not string!\n");
      else
         dbus_message_iter_get_basic(&args, &rcvdMsg);
      printf("Got Reply [ \'%s\' ]\n", rcvdMsg);

      // free reply
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
}


void reply_to_method_call(DBusMessage* msg, DBusConnection* conn, char* iface, char *rcvdSig)
{
   DBusMessage* reply;
   DBusMessageIter args;
   dbus_uint32_t serial = 0;
   //char* param = "";
   char repMsg[100] = "RPLY_MSG";
   char *pRepMsg = repMsg;

   parseRcvdMsg(msg, iface, rcvdSig);

   // create a reply from the message
   reply = dbus_message_new_method_return(msg);

   // add the arguments to the reply
   dbus_message_iter_init_append(reply, &args);
   if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &pRepMsg)) {
      fprintf(stderr, "Out Of Memory!\n");
      exit(1);
   }

   // send the reply && flush the connection
   if (!dbus_connection_send(conn, reply, &serial)) {
      fprintf(stderr, "Out Of Memory!\n");
      exit(1);
   }

   printf("Reply with [ \'%s\' ]\n", repMsg);

   dbus_connection_flush(conn);

   // free the reply
   dbus_message_unref(reply);
}


int receiveRoutin(dbus_session_params *dbus)
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
         // free the message
         dbus_message_unref(msg);
      }
      else if (dbus_message_is_signal(msg, iface, MSG_NAME1))
      {
         parseRcvdMsg(msg, iface, MSG_NAME1);
         // free the message
         dbus_message_unref(msg);
      }
      else if (dbus_message_is_signal(msg, iface, MSG_NAME2))
      {
         parseRcvdMsg(msg, iface, MSG_NAME2);
         // free the message
         dbus_message_unref(msg);
      }
      else if (dbus_message_is_method_call(msg, iface, MSG_NAME3))
      {
         usleep(1000000); //For pending test
         reply_to_method_call(msg, conn, iface, MSG_NAME3);
      }
      else
      {
         /* No operation */
      }
      ret = 0;
   }
   return ret;
}

void dbusInit(dbus_session_params *dbus)
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

void dbusRecvIfaceSet(dbus_session_params *dbus)
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
   dbus_session_params *dbus = (dbus_session_params *)arg;

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
   printf ("Syntax: dbus-example [send] [ifaceNum] [msgNum]\n");
   printf ("        dbus-example [recv]\n");
   exit(0);
}

int main(int argc, char** argv)
{
//================================================================
// [ Example ]
// Server side
// # ./dbus-example recv
// Client side (interface number: 0, 1 / message number: 3, 5, 7)
// # ./dbus-example send 0 3
//================================================================

   pthread_t tid;
   dbus_session_params *dbus_brd_send;
   dbus_session_params *dbus_uni_send;

   char method_srv0[] = METHOD_SERVER0;
   char method_srv1[] = METHOD_SERVER1;

//----------------------------------------------------------------
// Broadcast parameters
   dbus_session_params dbus0_brd_send = (dbus_session_params){{0}, NULL, DBUS_OBJ0, DBUS_IFACE0, SIGNAL_SOURCE0, NULL, 0};
   dbus_session_params dbus0_brd_recv = (dbus_session_params){{0}, NULL, DBUS_OBJ0, DBUS_IFACE0, SIGNAL_SINK0  , NULL, 0};
   dbus_session_params dbus1_brd_send = (dbus_session_params){{0}, NULL, DBUS_OBJ0, DBUS_IFACE1, SIGNAL_SOURCE1, NULL, 0};
   dbus_session_params dbus1_brd_recv = (dbus_session_params){{0}, NULL, DBUS_OBJ0, DBUS_IFACE1, SIGNAL_SINK1  , NULL, 0};

// Unicast parameters
   dbus_session_params dbus0_uni_send = (dbus_session_params){{0}, NULL, DBUS_OBJ0, DBUS_IFACE0, METHOD_CALLER0, method_srv0, 1500}; //wait(max 1500ms) to receive delayed reply
   dbus_session_params dbus0_uni_recv = (dbus_session_params){{0}, NULL, DBUS_OBJ0, DBUS_IFACE0, METHOD_SERVER0 , NULL, 0};
   dbus_session_params dbus1_uni_send = (dbus_session_params){{0}, NULL, DBUS_OBJ0, DBUS_IFACE1, METHOD_CALLER1, method_srv1, 2500}; //wait(max 2500ms) to receive delayed reply
   dbus_session_params dbus1_uni_recv = (dbus_session_params){{0}, NULL, DBUS_OBJ0, DBUS_IFACE1, METHOD_SERVER1 , NULL, 0};
//----------------------------------------------------------------

//----------------------------------------------------------------
// Test case 0
   int tmpNum0 = 234;
   msgField_t d003[3] = { 
                          (msgField_t){DBUS_TYPE_STRING, "asdf"}, 
                          (msgField_t){DBUS_TYPE_INT32, &tmpNum0}, 
                          (msgField_t){DBUS_TYPE_STRING, "TEST - received msg0003"} 
                        };
   msgStruct_t msg003 = (msgStruct_t){ MSG_NAME0, 3, d003 };

// Test case 1
   int tmpNum1 = 32767;
   int tmpNum2 = -256;
   int tmpNum3 = 127;
   msgField_t d005[4] = { 
                          (msgField_t){DBUS_TYPE_INT16 , &tmpNum1}, 
                          (msgField_t){DBUS_TYPE_INT32 , &tmpNum2},
                          (msgField_t){DBUS_TYPE_BYTE  , &tmpNum3},
                          (msgField_t){DBUS_TYPE_STRING, "TEST - received msg0005"}
                        };
   msgStruct_t msg005 = (msgStruct_t){ MSG_NAME1, 4, d005 };

// Test case 2
   msgField_t d007[1] = { 
                          (msgField_t){DBUS_TYPE_STRING, "TEST - received msg0007"}
                        };
   msgStruct_t msg007 = (msgStruct_t){ MSG_NAME3, 1, d007 };
//----------------------------------------------------------------

   if (argc < 2)
   {
      usage();
   }
   else
   {
      /* No operation */
   }

   if (argc >= 4)
   {
      if (0 == strcmp(argv[1], "send"))
      {
         /* Select interface */
         if(atoi(argv[2]) == 0)
         {
            dbus_brd_send = &dbus0_brd_send;
            dbus_uni_send = &dbus0_uni_send;
         }
         else if(atoi(argv[2]) == 1)
         {
            dbus_brd_send = &dbus1_brd_send;
            dbus_uni_send = &dbus1_uni_send;
         }
         else
         {
            /* No operation */
         }

         /* Send message selected by message number */
         dbusInit(dbus_brd_send);
         dbusInit(dbus_uni_send);
         if(atoi(argv[3]) == MSG_NUM_3)
         {
            sendMsg(dbus_brd_send, &msg003);
         }
         else if(atoi(argv[3]) == MSG_NUM_5)
         {
            sendMsg(dbus_brd_send, &msg005);
         }
         else if(atoi(argv[3]) == MSG_NUM_10)
         {
            sendMsg(dbus_uni_send, &msg007);
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
   else if (0 == strcmp(argv[1], "recv"))
   {
      // Receive thread for broadcast message
      dbusInit(&dbus0_brd_recv);
      dbusInit(&dbus1_brd_recv);
      dbusRecvIfaceSet(&dbus0_brd_recv);
      dbusRecvIfaceSet(&dbus1_brd_recv);
      pthread_create(&tid, NULL, receive, (void *)&dbus0_brd_recv);
      pthread_create(&tid, NULL, receive, (void *)&dbus1_brd_recv);

      // Receive thread for unicast message
      dbusInit(&dbus0_uni_recv);
      dbusInit(&dbus1_uni_recv);
      pthread_create(&tid, NULL, receive, (void *)&dbus0_uni_recv);
      pthread_create(&tid, NULL, receive, (void *)&dbus1_uni_recv);

      while (1)
      {
         usleep(100000);
      }
   }
   else 
   {
      usage();
   }
   return 0;
}
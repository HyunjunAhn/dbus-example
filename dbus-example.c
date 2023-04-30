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

#define SIGNAL_IFACE "test.signal.Type"
#define METHOD_IFACE "test.method.Type"

#define SIGNAL_MEMBER0 "Test"
#define SIGNAL_MEMBER1 "Tick"
#define SIGNAL_MEMBER2 "Num"
/**
 * Connect to the DBUS bus and send a broadcast signal
 */
void sendsignalStr(char* sigvalue)
{
   DBusMessage* msg;
   DBusMessageIter args;
   DBusConnection* conn;
   DBusError err;
   int ret;
   dbus_uint32_t serial = 0;
   char buf[100];
   char* pBuf = buf;

   printf("Sending signal with value %s\n", sigvalue);

   // initialise the error value
   dbus_error_init(&err);

   // connect to the DBUS system bus, and check for errors
   conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
   if (dbus_error_is_set(&err)) {
      fprintf(stderr, "Connection Error (%s)\n", err.message);
      dbus_error_free(&err);
   }
   if (NULL == conn) {
      exit(1);
   }

   // register our name on the bus, and check for errors
   ret = dbus_bus_request_name(conn, "test.signal.source", DBUS_NAME_FLAG_REPLACE_EXISTING , &err);
   if (dbus_error_is_set(&err)) {
      fprintf(stderr, "Name Error (%s)\n", err.message);
      dbus_error_free(&err);
   }
   if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
      exit(1);
   }

   for(int j=0;j<5;j++)
   {
      // create a signal & check for errors
      msg = dbus_message_new_signal("/test/signal/Object", // object name of the signal
                                    SIGNAL_IFACE, // interface name of the signal
                                    SIGNAL_MEMBER0); // name of the signal
      if (NULL == msg)
      {
         fprintf(stderr, "Message Null\n");
         exit(1);
      }

      sprintf(buf, "%s_%d", sigvalue, j);
      // append arguments onto signal
      dbus_message_iter_init_append(msg, &args);
      if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &pBuf)) {
         fprintf(stderr, "Out Of Memory!\n");
         exit(1);
      }

      // send the message and flush the connection
      if (!dbus_connection_send(conn, msg, &serial)) {
         fprintf(stderr, "Out Of Memory!\n");
         exit(1);
      }
      dbus_connection_flush(conn);

      printf("Signal Sent with value %s\n", buf);

      // free the message
      dbus_message_unref(msg);
      usleep(100000);
   }
}

void sendsignalInt(int sigvalue)
{
   DBusMessage* msg;
   DBusMessageIter args;
   DBusConnection* conn;
   DBusError err;
   int ret;
   dbus_uint32_t serial = 0;
   int buf = 0;
   char* pBuf = &buf;

   printf("Sending signal with value %d\n", sigvalue);

   // initialise the error value
   dbus_error_init(&err);

   // connect to the DBUS system bus, and check for errors
   conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
   if (dbus_error_is_set(&err)) {
      fprintf(stderr, "Connection Error (%s)\n", err.message);
      dbus_error_free(&err);
   }
   if (NULL == conn) {
      exit(1);
   }

   // register our name on the bus, and check for errors
   ret = dbus_bus_request_name(conn, "test.signal.source", DBUS_NAME_FLAG_REPLACE_EXISTING , &err);
   if (dbus_error_is_set(&err)) {
      fprintf(stderr, "Name Error (%s)\n", err.message);
      dbus_error_free(&err);
   }
   if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
      exit(1);
   }

   for(int j=0;j<5;j++)
   {
      // create a signal & check for errors
      msg = dbus_message_new_signal("/test/signal/Object", // object name of the signal
                                    SIGNAL_IFACE, // interface name of the signal
                                    SIGNAL_MEMBER2); // name of the signal
      if (NULL == msg)
      {
         fprintf(stderr, "Message Null\n");
         exit(1);
      }

      buf = sigvalue*10 + j;
      // append arguments onto signal
      dbus_message_iter_init_append(msg, &args);
      if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, pBuf)) {
         fprintf(stderr, "Out Of Memory!\n");
         exit(1);
      }

      // send the message and flush the connection
      if (!dbus_connection_send(conn, msg, &serial)) {
         fprintf(stderr, "Out Of Memory!\n");
         exit(1);
      }
      dbus_connection_flush(conn);
      printf("Signal Sent with value %d (path:%s, iface:%s, member:%s)\n", buf, 
      dbus_message_get_path(msg), dbus_message_get_interface(msg), 
      dbus_message_get_member(msg));

      // free the message
      dbus_message_unref(msg);
      usleep(100000);
   }
}


/**
 * Call a method on a remote object
 */
void query(char* sendMsg)
{
   DBusMessage* msg;
   DBusMessageIter args;
   DBusConnection* conn;
   DBusError err;
   DBusPendingCall* pending;
   int ret;
   bool stat;
   dbus_uint32_t level;
   char* rcvdMsg = "";

   printf("[%s] message sent.\n", sendMsg);

   // initialiset the errors
   dbus_error_init(&err);

   // connect to the system bus and check for errors
   conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
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
      exit(1);
   }

   // create a new method call and check for errors
   msg = dbus_message_new_method_call("test.method.server", // target for the method call
                                      "/test/method/Object", // object to call on
                                      METHOD_IFACE, // interface to call on
                                      "Method"); // method name
   if (NULL == msg) {
      fprintf(stderr, "Message Null\n");
      exit(1);
   }

   // append arguments
   dbus_message_iter_init_append(msg, &args);
   if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &sendMsg)) {
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
   bool stat = true;
   dbus_uint32_t level = 21614;
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
   DBusMessage* reply;
   DBusMessageIter args;
   DBusConnection* conn;
   DBusError err;
   int ret;
   char* param;

   printf("Listening for method calls\n");

   // initialise the error
   dbus_error_init(&err);

   // connect to the bus and check for errors
   conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
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
      exit(1);
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

/**
 * Listens for signals on the bus
 */
void receive()
{
   DBusMessage* msg;
   DBusMessageIter args;
   DBusConnection* conn;
   DBusError err;
   int ret;
   char* sigvalue_str;
   int sigvalue_int; 

   printf("Listening for signals\n");

   // initialise the errors
   dbus_error_init(&err);

   // connect to the bus and check for errors
   conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
   if (dbus_error_is_set(&err)) {
      fprintf(stderr, "Connection Error (%s)\n", err.message);
      dbus_error_free(&err);
   }
   if (NULL == conn) {
      exit(1);
   }

   // request our name on the bus and check for errors
   ret = dbus_bus_request_name(conn, "test.signal.sink", DBUS_NAME_FLAG_REPLACE_EXISTING , &err);
   if (dbus_error_is_set(&err)) {
      fprintf(stderr, "Name Error (%s)\n", err.message);
      dbus_error_free(&err);
   }
   if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
      printf("Not primary owner\n");
      //exit(1);
   }

   // add a rule for which messages we want to see
   char tmp[100];
   sprintf(tmp, "type='signal',interface=\'%s\'", SIGNAL_IFACE);
   dbus_bus_add_match(conn, tmp, &err); // see signals from the given interface
   dbus_connection_flush(conn);
   if (dbus_error_is_set(&err)) {
      fprintf(stderr, "Match Error (%s)\n", err.message);
      exit(1);
   }
   printf("Match rule sent\n");

   // loop listening for signals being emmitted
   while (true) {

      // non blocking read of the next available message
      dbus_connection_read_write(conn, 0);
      msg = dbus_connection_pop_message(conn);

      // loop again if we haven't read a message
      if (NULL == msg) {
         usleep(10000);
         continue;
      }

      // check if the message is a signal from the correct interface and with the correct name
      if ((dbus_message_is_signal(msg, SIGNAL_IFACE, SIGNAL_MEMBER0))
          || (dbus_message_is_signal(msg, SIGNAL_IFACE, SIGNAL_MEMBER1))) {

         // read the parameters
         if (!dbus_message_iter_init(msg, &args))
            fprintf(stderr, "Message Has No Parameters\n");
         else if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args))
            fprintf(stderr, "Argument is not string!\n");
         else
            dbus_message_iter_get_basic(&args, &sigvalue_str);

         printf("Got Signal with value \'%s\'\n", sigvalue_str);
      }
      else if (dbus_message_is_signal(msg, SIGNAL_IFACE, SIGNAL_MEMBER2))
      {
         // read the parameters
         if (!dbus_message_iter_init(msg, &args))
            fprintf(stderr, "Message Has No Parameters\n");
         else if (DBUS_TYPE_INT32 != dbus_message_iter_get_arg_type(&args))
            fprintf(stderr, "Argument is not integer!\n");
         else
            dbus_message_iter_get_basic(&args, &sigvalue_int);

         printf("Got Signal with value %d\n", sigvalue_int);
      }

      // free the message
      dbus_message_unref(msg);
   }
}

int main(int argc, char** argv)
{
   if (2 > argc) {
      printf ("Syntax: dbus-example [sendstr|sendint|receive|listen|query] [<param>]\n");
      return 1;
   }
   char* param = "no param";
   if (3 >= argc && NULL != argv[2]) param = argv[2];
   if (0 == strcmp(argv[1], "sendstr"))
      sendsignalStr(param);
   else if (0 == strcmp(argv[1], "sendint"))
      sendsignalInt(atoi(param));
   else if (0 == strcmp(argv[1], "receive"))
      receive();
   else if (0 == strcmp(argv[1], "listen"))
      listen();
   else if (0 == strcmp(argv[1], "query"))
      query(param);
   else {
      printf ("Syntax: dbus-example [send|receive|listen|query] [<param>]\n");
      return 1;
   }
   return 0;
}
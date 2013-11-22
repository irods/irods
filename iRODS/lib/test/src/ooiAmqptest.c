#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <amqp.h>
#include <amqp_framing.h>
#include <time.h>
#include <msgpack.h>
#include "rodsClient.h"

#define PROD_CHAN	1
#define CONS_CHAN	2

int 
initOoiReqProp (amqp_basic_properties_t *props, char *replyQueueName,
char *receiver, char *op, char *format, char *ts);
int
initRecvApiReply (amqp_connection_state_t conn, amqp_frame_t *frame);
int
recvApiReplyProp (amqp_connection_state_t conn, amqp_frame_t *frame);
int
recvApiReplyBody (amqp_connection_state_t conn, int channel,
amqp_frame_t *frame, bytesBuf_t **extBytesBuf);
int
printAmqpHeaders (amqp_table_t *headers);
int 
clearProp (amqp_basic_properties_t *props);
int
amqp_status (amqp_rpc_reply_t x);

#define NUM_OOI_HEADER_ENTRIES	16

int main(int argc, char const * const *argv) {
  char const *hostname;
  int port;
  char const *exchange;
  char const *routingkey, *replyRoutingkey;
  int sockfd;
  amqp_connection_state_t conn;
  amqp_bytes_t replyQueue;
  amqp_queue_declare_ok_t *queue_declare_ret;
  amqp_basic_properties_t props;
  amqp_bytes_t message_bytes;
  char receiverStr[128], tsStr[128], replyToStr[128];
  msgpack_sbuffer* buffer;
  msgpack_packer* pk;
  amqp_frame_t frame;
  int status;
  bytesBuf_t *extBytesBuf;	/* used only if more than 1 receive is needed */
  amqp_basic_properties_t *p;
  msgpack_unpacked unpackedRes;
  msgpack_object *unpackedObj;
  size_t offset;
  char bankId[1204];
  char apiResult[1204];
  amqp_rpc_reply_t astatus;

  hostname = "localhost";
  port = 5672;
  /* exchange = "ion_mwan-hp"; */
  exchange = "ion_one";
  routingkey = "bank";
  replyRoutingkey = "myAPIReply";
  bzero (&props, sizeof (props));

  conn = amqp_new_connection();

  sockfd = amqp_open_socket(hostname, port);
  if (sockfd < 0) {
    fprintf(stderr, "amqp_open_socket error, staus = %d\n", sockfd);
    exit(1);
  }
  amqp_set_sockfd(conn, sockfd);
  astatus = amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, 
    "guest", "guest");
  status = amqp_status (astatus);
  if (status < 0) {
    fprintf(stderr, "amqp_login of guest error, staus = %d\n", status);
    exit(2);
  }

  amqp_channel_open(conn, PROD_CHAN);
  astatus = amqp_get_rpc_reply(conn);
  status = amqp_status (astatus);
  if (status < 0) {
    fprintf(stderr, "amqp_channel_open PROD_CHAN error, staus = %d\n", status);
    exit(2);
  }
  amqp_channel_open(conn, CONS_CHAN);
  astatus = amqp_get_rpc_reply(conn);
  status = amqp_status (astatus);
  if (status < 0) {
    fprintf(stderr, "amqp_channel_open CONS_CHAN error, staus = %d\n", status);
    exit(2);
  }

  queue_declare_ret = amqp_queue_declare(conn, CONS_CHAN, amqp_empty_bytes, 
                       0, 0, 0, 1, amqp_empty_table);
  astatus = amqp_get_rpc_reply(conn);
  status = amqp_status (astatus);
  if (status < 0) {
    fprintf(stderr, "queue_declare_ret error, staus = %d\n", status);
    exit(2);
  }
  replyQueue = amqp_bytes_malloc_dup(queue_declare_ret->queue);
  if (replyQueue.bytes == NULL) {
    fprintf(stderr, "Out of memory while copying queue name");
    return 1;
  } else {
    printf ("replyQueue name:%s\n", (char *) replyQueue.bytes);
  }
  amqp_queue_bind(conn, CONS_CHAN, replyQueue, amqp_cstring_bytes(exchange), 
    amqp_cstring_bytes(replyRoutingkey), amqp_empty_table);
  /* XXXXXX setting no_ack in amqp_basic_consume is important because it does
   * not work with ack */
  amqp_basic_consume(conn, CONS_CHAN, replyQueue, amqp_empty_bytes, 0, 1, 0, 
    amqp_empty_table);

  snprintf (receiverStr, 128, "%s,%s", exchange, routingkey);
  snprintf (tsStr, 128, "%d", (int) time (0));
  snprintf (replyToStr, 128, "%s,%s", exchange, replyRoutingkey);
  initOoiReqProp (&props, (char *) replyToStr, receiverStr, 
      /* (char *) "new_account", (char *) "bank_new_account_in", tsStr); */
      (char *) "new_account", (char *) "dict", tsStr);

  /* creates buffer and serializer instance. */
  buffer = msgpack_sbuffer_new();
  pk = msgpack_packer_new(buffer, msgpack_sbuffer_write);
  msgpack_pack_map (pk, 2);
  msgpack_pack_raw (pk, strlen ("account_type"));
  msgpack_pack_raw_body (pk, "account_type", strlen ("account_type"));
  msgpack_pack_raw (pk, strlen ("Savings"));
  msgpack_pack_raw_body (pk, "Savings", strlen ("Savings"));
  msgpack_pack_raw (pk, strlen ("name"));
  msgpack_pack_raw_body (pk, "name", strlen ("name"));
  msgpack_pack_raw (pk, strlen ("John"));
  msgpack_pack_raw_body (pk, "John", strlen ("John"));

  message_bytes.len = buffer->size;
  message_bytes.bytes = buffer->data;

  printf ("len=%d, data=%s\n", message_bytes.len, (char *) message_bytes.bytes);

  status = amqp_basic_publish (conn, PROD_CHAN,
				    amqp_cstring_bytes(exchange),
				    amqp_cstring_bytes(routingkey),
				    0,
				    0,
				    &props,
				    message_bytes);
  if (status < 0) {
    fprintf(stderr, "amqp_basic_publish error, staus = %d\n", status);
    exit(2);
  }
  msgpack_sbuffer_free(buffer);
  msgpack_packer_free (pk);
  clearProp (&props);
  status = initRecvApiReply (conn, &frame);
  if (status < 0) return status;

  status = recvApiReplyProp (conn, &frame);
  if (status < 0) return status;

  p = (amqp_basic_properties_t *) frame.payload.properties.decoded;
  printAmqpHeaders (&p->headers);

  status = recvApiReplyBody (conn, CONS_CHAN, &frame, &extBytesBuf);
  if (status < 0) return status;

  msgpack_unpacked_init(&unpackedRes);
  offset = 0;
  while (msgpack_unpack_next(&unpackedRes, 
   (const char*) frame.payload.body_fragment.bytes, 
    frame.payload.body_fragment.len, &offset)) {
    unpackedObj = &unpackedRes.data;
    /* expect a str return */
    if (unpackedObj->type != MSGPACK_OBJECT_RAW) {
      fprintf (stderr, "unexpected unpacked type %d\n", unpackedObj->type);
    } else {
      if (unpackedObj->via.raw.size >= 1024) {
        fprintf (stderr, "unpackedObj size %d too large\n", 
          unpackedObj->via.raw.size);
      } else {
          strncpy (bankId, unpackedObj->via.raw.ptr, unpackedObj->via.raw.size);
          bankId[unpackedObj->via.raw.size] = '\0';
          printf ("bankId = %s\n", bankId);
      }
    }
#if 0
    msgpack_object_print(stdout, unpackedObj);
    puts("");
#endif
  }
  snprintf (tsStr, 128, "%d", (int) time (0));
  initOoiReqProp (&props, (char *) replyToStr, receiverStr,
      /* (char *) "deposit", (char *) "bank_deposit_in", tsStr); */
      (char *) "deposit", (char *) "dict", tsStr);

  /* creates buffer and serializer instance. */
  buffer = msgpack_sbuffer_new();
  pk = msgpack_packer_new(buffer, msgpack_sbuffer_write);
  msgpack_pack_map (pk, 2);
  msgpack_pack_raw (pk, strlen ("account_id"));
  msgpack_pack_raw_body (pk, "account_id", strlen ("account_id"));
  msgpack_pack_raw (pk, strlen (bankId));
  msgpack_pack_raw_body (pk, bankId, strlen (bankId));
  msgpack_pack_raw (pk, strlen ("amount"));
  msgpack_pack_raw_body (pk, "amount", strlen ("amount"));
  msgpack_pack_float (pk, 150.0);

  message_bytes.len = buffer->size;
  message_bytes.bytes = buffer->data;

  printf ("len=%d, data=%s\n", message_bytes.len, (char *) message_bytes.bytes);

  status = amqp_basic_publish (conn,
                                    PROD_CHAN,
                                    amqp_cstring_bytes(exchange),
                                    amqp_cstring_bytes(routingkey),
                                    0,
                                    0,
                                    &props,
                                    message_bytes);

  if (status < 0) {
    fprintf(stderr, "amqp_basic_publish error, staus = %d\n", status);
    exit(2);
  }

  msgpack_sbuffer_free(buffer);
  msgpack_packer_free (pk);
  clearProp (&props);

  status = initRecvApiReply (conn, &frame);
  if (status < 0) return status;

  status = recvApiReplyProp (conn, &frame);
  if (status < 0) return status;

  p = (amqp_basic_properties_t *) frame.payload.properties.decoded;
  printAmqpHeaders (&p->headers);

  status = recvApiReplyBody (conn, CONS_CHAN, &frame, &extBytesBuf);
  if (status < 0) return status;

  msgpack_unpacked_init(&unpackedRes);

  offset = 0;
  while (msgpack_unpack_next(&unpackedRes,
   (const char*) frame.payload.body_fragment.bytes,
    frame.payload.body_fragment.len, &offset)) {
    unpackedObj = &unpackedRes.data;
    /* expect a str return */
    if (unpackedObj->type != MSGPACK_OBJECT_RAW) {
      fprintf (stderr, "unexpected unpacked type %d\n", unpackedObj->type);
    } else {
      if (unpackedObj->via.raw.size >= 1024) {
        fprintf (stderr, "unpackedObj size %d too large\n",
          unpackedObj->via.raw.size);
      } else {
          strncpy (apiResult, unpackedObj->via.raw.ptr, unpackedObj->via.raw.size);
          apiResult[unpackedObj->via.raw.size] = '\0';
          printf ("apiResult = %s\n", apiResult);
      }
    }
  }

  /* list account */
  snprintf (tsStr, 128, "%d", (int) time (0));
  initOoiReqProp (&props, (char *) replyToStr, receiverStr,
      /* (char *) "list_accounts", (char *) "bank_list_accounts_in", tsStr); */
      (char *) "list_accounts", (char *) "dict", tsStr);

  /* creates buffer and serializer instance. */
  buffer = msgpack_sbuffer_new();
  pk = msgpack_packer_new(buffer, msgpack_sbuffer_write);
  msgpack_pack_map (pk, 1);
  msgpack_pack_raw (pk, strlen ("name"));
  msgpack_pack_raw_body (pk, "name", strlen ("name"));
  msgpack_pack_raw (pk, strlen ("John"));
  msgpack_pack_raw_body (pk, "John", strlen ("John"));

  message_bytes.len = buffer->size;
  message_bytes.bytes = buffer->data;

  printf ("len=%d, data=%s\n", message_bytes.len, (char *) message_bytes.bytes);

  status = amqp_basic_publish (conn,
                                    PROD_CHAN,
                                    amqp_cstring_bytes(exchange),
                                    amqp_cstring_bytes(routingkey),
                                    0,
                                    0,
                                    &props,
                                    message_bytes);
  if (status < 0) {
    fprintf(stderr, "amqp_basic_publish error, staus = %d\n", status);
    exit(2);
  }
  msgpack_sbuffer_free(buffer);
  msgpack_packer_free (pk);
  clearProp (&props);
  status = initRecvApiReply (conn, &frame);
  if (status < 0) return status;

  status = recvApiReplyProp (conn, &frame);
  if (status < 0) return status;
  p = (amqp_basic_properties_t *) frame.payload.properties.decoded;
  printAmqpHeaders (&p->headers);

  status = recvApiReplyBody (conn, CONS_CHAN, &frame, &extBytesBuf);
  if (status < 0) return status;

  msgpack_unpacked_init(&unpackedRes);
  offset = 0;
  while (msgpack_unpack_next(&unpackedRes,
   (const char*) frame.payload.body_fragment.bytes,
    frame.payload.body_fragment.len, &offset)) {
    unpackedObj = &unpackedRes.data;
    /* expect a str return */
    if (unpackedObj->type == MSGPACK_OBJECT_RAW) {
      if (unpackedObj->via.raw.size >= 1024) {
        fprintf (stderr, "unpackedObj size %d too large\n",
          unpackedObj->via.raw.size);
      } else {
          strncpy (bankId, unpackedObj->via.raw.ptr, unpackedObj->via.raw.size);
          bankId[unpackedObj->via.raw.size] = '\0';
          printf ("bankId = %s\n", bankId);
      }
    } else if (unpackedObj->type == MSGPACK_OBJECT_ARRAY) {
      msgpack_object_print (stdout, *unpackedObj);
    } else {
      msgpack_object_print (stdout, *unpackedObj);
    }
  }


  amqp_maybe_release_buffers(conn);
  astatus = amqp_channel_close(conn, PROD_CHAN, AMQP_REPLY_SUCCESS);
  status = amqp_status (astatus);
  if (status < 0) {
    fprintf(stderr, "amqp_channel_close PROD_CHAN error, staus = %d\n", status);
  }
  astatus = amqp_channel_close(conn, CONS_CHAN, AMQP_REPLY_SUCCESS);
  status = amqp_status (astatus);
  if (status < 0) {
    fprintf(stderr, "amqp_channel_close CONS_CHAN error, staus = %d\n", status);
  }
  astatus = amqp_connection_close(conn, AMQP_REPLY_SUCCESS);
  status = amqp_status (astatus);
  if (status < 0) {
    fprintf(stderr, "amqp_connection_close error, staus = %d\n", status);
  }
  status = amqp_destroy_connection(conn);
  if (status < 0) {
    fprintf(stderr, "amqp_destroy_connection error, staus = %d\n", status);
  }

  return 0;
}

int
initRecvApiReply (amqp_connection_state_t conn, amqp_frame_t *frame)
{
    amqp_basic_deliver_t *d;
    int status;

    amqp_maybe_release_buffers(conn);
    status = amqp_simple_wait_frame(conn, frame);
    if (status < 0) {
      fprintf(stderr, 
        "initRecvApiReply: amqp_simple_wait_frame failed. status %d\n", status);
      return status;
    }
    printf ("Frame type %d, channel %d\n", frame->frame_type, frame->channel);
    if (frame->frame_type != AMQP_FRAME_METHOD) {
	fprintf (stderr, "frame_type %d is not AMQP_FRAME_METHOD", 
	 frame->frame_type);
	return -1;
    }

    printf("Method %s\n", amqp_method_name(frame->payload.method.id));
    if (frame->payload.method.id != AMQP_BASIC_DELIVER_METHOD) {
	fprintf (stderr, "method.id %x is not AMQP_BASIC_DELIVER_METHOD",
	  frame->payload.method.id);
    } else {
      d = (amqp_basic_deliver_t *) frame->payload.method.decoded;
      printf("Delivery %u, exchange %.*s routingkey %.*s\n",
             (unsigned) d->delivery_tag,
             (int) d->exchange.len, (char *) d->exchange.bytes,
             (int) d->routing_key.len, (char *) d->routing_key.bytes);
    }
    return 0;
}

int
recvApiReplyProp (amqp_connection_state_t conn, amqp_frame_t *frame)
{
    amqp_basic_properties_t *p;
    int status;

    status = amqp_simple_wait_frame(conn, frame);
    if (status < 0) {
      fprintf(stderr, 
        "recvApiReplyProp: amqp_simple_wait_frame failed. status %d\n", status);
      return status;
    }

    if (frame->frame_type != AMQP_FRAME_HEADER) {
      fprintf (stderr, 
        "recvApiReplyProp: frame_type %d is not AMQP_FRAME_HEADER", 
        frame->frame_type);
      return -1;
    }
    p = (amqp_basic_properties_t *) frame->payload.properties.decoded;

    if (p != NULL) 
      return 0;
    else 
      return -1;
}

int
recvApiReplyBody (amqp_connection_state_t conn, int channel,
amqp_frame_t *frame, bytesBuf_t **extBytesBuf)
{
    size_t body_target;
    size_t body_received;
    amqp_basic_deliver_t *d;
    int status;


    *extBytesBuf = NULL;
    body_target = frame->payload.properties.body_size;
    body_received = 0;

    while (body_received < body_target) {
      status = amqp_simple_wait_frame(conn, frame);
      if (status < 0)
        break;

      if (frame->frame_type != AMQP_FRAME_BODY) {
        fprintf(stderr, "Expected body!");
        return -1;
      }

      body_received += frame->payload.body_fragment.len;

#if 0
      amqp_dump(frame->payload.body_fragment.bytes,
                frame->payload.body_fragment.len);
#endif
    }
    d = (amqp_basic_deliver_t *) frame->payload.method.decoded;
#if 0
    amqp_basic_ack(conn, channel, d->delivery_tag, 0);
#endif
    if (body_received != body_target) {
      /* Can only happen when amqp_simple_wait_frame returns <= 0 */
      /* We break here to close the connection */
      fprintf (stderr, "body_received %d != body_target %d",
        (int) body_received, (int) body_target);
        return -1;
    }
    return 0;
}

int initOoiReqProp (amqp_basic_properties_t *props, char *replyToStr,
char *receiver, char *op, char *format, char *ts)
{
  amqp_table_entry_t *entries;
  int i = 0;
  bzero (props, sizeof (amqp_basic_properties_t));
  props->_flags = AMQP_BASIC_CONTENT_TYPE_FLAG | 
    AMQP_BASIC_DELIVERY_MODE_FLAG | AMQP_BASIC_HEADERS_FLAG;
#if 0
    props.content_type = amqp_cstring_bytes("text/plain");
#endif
  props->delivery_mode = 2; /* persistent delivery mode */
  props->headers.num_entries = NUM_OOI_HEADER_ENTRIES;

  props->headers.entries = entries = (amqp_table_entry_t *) calloc 
	(NUM_OOI_HEADER_ENTRIES, sizeof (amqp_table_entry_t));

  entries[i].key = amqp_cstring_bytes("protocol");
  entries[i].value.kind = AMQP_FIELD_KIND_UTF8;
  entries[i].value.value.bytes = amqp_cstring_bytes("rpc");
  i++;

  entries[i].key = amqp_cstring_bytes("sender-name");
  entries[i].value.kind = AMQP_FIELD_KIND_UTF8;
  entries[i].value.value.bytes = amqp_cstring_bytes("John");
  i++;

  entries[i].key = amqp_cstring_bytes("encoding");
  entries[i].value.kind = AMQP_FIELD_KIND_UTF8;
  entries[i].value.value.bytes = amqp_cstring_bytes("msgpack");
  i++;

  entries[i].key = amqp_cstring_bytes("reply-by");
  entries[i].value.kind = AMQP_FIELD_KIND_UTF8;
  entries[i].value.value.bytes = amqp_cstring_bytes("todo");
  i++;

  entries[i].key = amqp_cstring_bytes("expiry");
  entries[i].value.kind = AMQP_FIELD_KIND_UTF8;
  entries[i].value.value.bytes = amqp_cstring_bytes("0");
  i++;

#if 0
  entries[i].key = amqp_cstring_bytes("ion-actor-id");
  entries[i].value.kind = AMQP_FIELD_KIND_UTF8;
  entries[i].value.value.bytes = amqp_cstring_bytes("anonymous");
  i++;
#endif

  entries[i].key = amqp_cstring_bytes("reply-to");
  entries[i].value.kind = AMQP_FIELD_KIND_UTF8;
  entries[i].value.value.bytes = amqp_cstring_bytes(replyToStr);
  i++;

  entries[i].key = amqp_cstring_bytes("conv-id");
  entries[i].value.kind = AMQP_FIELD_KIND_UTF8;
  entries[i].value.value.bytes = amqp_cstring_bytes("one_8347-1");
  i++;

  entries[i].key = amqp_cstring_bytes("conv-seq");
  entries[i].value.kind = AMQP_FIELD_KIND_I32;
  entries[i].value.value.i32 = 1;
  i++;

#if 0
  entries[i].key = amqp_cstring_bytes("origin-container-id");
  entries[i].value.kind = AMQP_FIELD_KIND_UTF8;
  entries[i].value.value.bytes = amqp_cstring_bytes("one_8347");
  i++;
#endif

  entries[i].key = amqp_cstring_bytes("sender");
  entries[i].value.kind = AMQP_FIELD_KIND_UTF8;
  entries[i].value.value.bytes = amqp_cstring_bytes("C_client");
  i++;

  entries[i].key = amqp_cstring_bytes("language");
  entries[i].value.kind = AMQP_FIELD_KIND_UTF8;
  entries[i].value.value.bytes = amqp_cstring_bytes("ion-r2");
  i++;

  entries[i].key = amqp_cstring_bytes("sender-type");
  entries[i].value.kind = AMQP_FIELD_KIND_UTF8;
  entries[i].value.value.bytes = amqp_cstring_bytes("service");
  i++;

#if 0
  entries[i].key = amqp_cstring_bytes("sender-service");
  entries[i].value.kind = AMQP_FIELD_KIND_UTF8;
  entries[i].value.value.bytes = amqp_cstring_bytes("ion_one,service_gateway");
  i++;
#endif

  entries[i].key = amqp_cstring_bytes("ts");
  entries[i].value.kind = AMQP_FIELD_KIND_UTF8;
  entries[i].value.value.bytes = amqp_cstring_bytes(ts);
  i++;

  entries[i].key = amqp_cstring_bytes("receiver");
  entries[i].value.kind = AMQP_FIELD_KIND_UTF8;
  entries[i].value.value.bytes = amqp_cstring_bytes(receiver);
  i++;

  entries[i].key = amqp_cstring_bytes("format");
  entries[i].value.kind = AMQP_FIELD_KIND_UTF8;
  entries[i].value.value.bytes = amqp_cstring_bytes(format);
  i++;

  entries[i].key = amqp_cstring_bytes("performative");
  entries[i].value.kind = AMQP_FIELD_KIND_UTF8;
  entries[i].value.value.bytes = amqp_cstring_bytes("request");
  i++;

  entries[i].key = amqp_cstring_bytes("op");
  entries[i].value.kind = AMQP_FIELD_KIND_UTF8;
  entries[i].value.value.bytes = amqp_cstring_bytes(op);
  i++;

  return 0;
}

int clearProp (amqp_basic_properties_t *props)
{
    if (props->headers.entries != NULL) free (props->headers.entries);
    return 0;
}

int
printAmqpHeaders (amqp_table_t *headers)
{
    int i;
    char outstr[1024];
    int len;
    char *ptr;

    if (headers < 0) return -1;

    printf ("headers: { ");    
    for (i = 0; i < headers->num_entries; i++) {
        len = headers->entries[i].key.len;
        ptr = (char *) headers->entries[i].key.bytes;
        if (len >= 1024) {
              printf ("entry %d length too long\n", len);
        } else {
             strncpy (outstr, ptr, len);
             outstr[len] = '\0';
             printf ("%s: ", outstr);
        }
	switch (headers->entries[i].value.kind) {
          case AMQP_FIELD_KIND_I16:
              printf ("%d", headers->entries[i].value.value.i16);
              break;
          case AMQP_FIELD_KIND_U16:
              printf ("%d", headers->entries[i].value.value.u16);
              break;
          case AMQP_FIELD_KIND_I32:
              printf ("%d", headers->entries[i].value.value.i32);
              break;
          case AMQP_FIELD_KIND_U32:
              printf ("%d", headers->entries[i].value.value.u32);
              break;
          case AMQP_FIELD_KIND_I64:
              printf ("%lld", headers->entries[i].value.value.i64);
              break;
          case AMQP_FIELD_KIND_U64:
              printf ("%lld", headers->entries[i].value.value.u64);
              break;
          case AMQP_FIELD_KIND_F32:
              printf ("%f", headers->entries[i].value.value.f32);
              break;
          case AMQP_FIELD_KIND_F64:
              printf ("%f", headers->entries[i].value.value.f64);
              break;
          case AMQP_FIELD_KIND_UTF8:
              len = headers->entries[i].value.value.bytes.len;
              ptr = (char *) headers->entries[i].value.value.bytes.bytes;
              if (len >= 1024) {
		  printf ("entry %d length too long\n", len); 
              } else {
                  strncpy (outstr, ptr, len);
                  outstr[len] = '\0';
                  printf ("%s", outstr); 
              }
              break;
          default:
              printf ("UNKNOW type %d", headers->entries[i].value.kind);
        }
        if (i < headers->num_entries - 1) printf (", ");
    }
    printf ("  }\n");
    return 0;
}

int 
amqp_status (amqp_rpc_reply_t x) {
  switch (x.reply_type) {
    case AMQP_RESPONSE_NORMAL:
      return 0;

    case AMQP_RESPONSE_NONE:
      fprintf(stderr, "missing RPC reply type!\n");
      return -1;
    case AMQP_RESPONSE_LIBRARY_EXCEPTION:
      fprintf(stderr, "%s\n", amqp_error_string(x.library_error));
      return -2;

    case AMQP_RESPONSE_SERVER_EXCEPTION:
      switch (x.reply.id) {
        case AMQP_CONNECTION_CLOSE_METHOD: {
          amqp_connection_close_t *m = (amqp_connection_close_t *) x.reply.decoded;
          fprintf(stderr, "server connection error %d, message: %.*s\n",
                  m->reply_code,
                  (int) m->reply_text.len, (char *) m->reply_text.bytes);
          return -3;
        }
        case AMQP_CHANNEL_CLOSE_METHOD: {
          amqp_channel_close_t *m = (amqp_channel_close_t *) x.reply.decoded;
          fprintf(stderr, "server channel error %d, message: %.*s\n",
                  m->reply_code,
                  (int) m->reply_text.len, (char *) m->reply_text.bytes);
          return -4;
        }
        default:
          fprintf(stderr, "unknown server error, method id 0x%08X\n", 
            x.reply.id);
          return -5;
      }
      break;
  }
  return 0;
}

#include <stdio.h>
#include <string.h>
#include "coco_platform.h"

#define HOST "127.0.0.1"    /* Local host */
#define MESSAGE_SIZE 8192   /* Large enough for the entire message */
#define RESPONSE_SIZE 1024  /* Large enough for the entire response (objective values or constraint violations) */

/**
 * @brief Data type needed for socket communication (used by the suites that need it).
 */
typedef struct {
  unsigned short port;           /**< @brief The port for communication with the external evaluator. */
  char *host_name;               /**< @brief The host name for communication with the external evaluator. */
  int precision_x;               /**< @brief Precision used to write the x-values to the external evaluator. */
  char* prev_message_obj;        /**< @brief Message of the previous evaluation of objectives */
  char* prev_response_obj;       /**< @brief Response of the previous evaluation of objectives */
  char* prev_message_con;        /**< @brief Message of the previous evaluation of constraints */
  char* prev_response_con;       /**< @brief Response of the previous evaluation of constraints */
#if WINSOCK
  SOCKET sock;                   /**< @brief Socket on Windows. */
  SOCKADDR_IN serv_addr;         /**< @brief Server address on Windows. */
#else
  int sock;                      /**< @brief Socket on non-Windows platforms. */
  struct sockaddr_in serv_addr;  /**< @brief Server address on non-Windows platforms. */
#endif
} socket_communication_data_t;

/**
 * @brief Frees the memory of a socket_communication_data_t object.
 */
static void socket_communication_data_finalize(void *stuff) {

  socket_communication_data_t *data;

  assert(stuff != NULL);
  data = (socket_communication_data_t *) stuff;
  if (data->host_name != NULL) {
    coco_free_memory(data->host_name);
  }

  /* Free the strings */
  if (data->prev_message_obj != NULL) {
    coco_free_memory(data->prev_message_obj);
  }
  if (data->prev_response_obj != NULL) {
    coco_free_memory(data->prev_response_obj);
  }
  if (data->prev_message_con != NULL) {
    coco_free_memory(data->prev_message_con);
  }
  if (data->prev_response_con != NULL) {
    coco_free_memory(data->prev_response_con);
  }

#if WINSOCK
  /* Tell the socket server to reset */
  send(data->sock, "RESET", (int)strlen("RESET") + 1, 0);
  closesocket(data->sock);
  WSACleanup();
#else
  /* Tell the socket server to reset */
  send(data->sock, "RESET", strlen("RESET") + 1, 0);
  close(data->sock);
#endif
}

static socket_communication_data_t *socket_communication_data_initialize(
    const char *suite_options, const unsigned short default_port) {

#if WINSOCK
  WSADATA wsa;
#endif
  socket_communication_data_t *data;
  data = (socket_communication_data_t *) coco_allocate_memory(sizeof(*data));

  data->host_name = coco_strdup(HOST);
  if (coco_options_read_string(suite_options, "host_name", data->host_name) == 0) {
    strcpy(data->host_name, HOST);
  }

  data->port = default_port;
  if (coco_options_read(suite_options, "port", "%hu", &(data->port)) == 0) {
  }

  data->precision_x = 8;
  if (coco_options_read_int(suite_options, "precision_x", &(data->precision_x)) != 0) {
    if ((data->precision_x < 1) || (data->precision_x > 32)) {
      data->precision_x = 8;
      coco_warning("socket_communication_data_initialize(): Adjusted precision_x value to %d",
          data->precision_x);
    }
  }

#if WINSOCK
  /* Initialize Winsock */
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    coco_error("socket_communication_data_initialize(): Winsock initialization failed: %d", WSAGetLastError());
  }

  /* Create a socket */
  if ((data->sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
    coco_error("socket_communication_data_initialize(): Could not create socket: %d", WSAGetLastError());
  }

  (data->serv_addr).sin_addr.s_addr = inet_addr(data->host_name);
  (data->serv_addr).sin_family = AF_INET;
  (data->serv_addr).sin_port = htons(data->port);

  /* Connect to the evaluator */
  if (connect(data->sock, (SOCKADDR *) &(data->serv_addr), sizeof(data->serv_addr)) < 0) {
    coco_error("socket_communication_data_initialize(): Connection failed (host = %s, port = %d)\nIs the server running?",
        data->host_name, data->port);
  }
#else
  /* Create a socket */
  if ((data->sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    coco_error("socket_communication_data_initialize(): Socket creation error");
  }

  (data->serv_addr).sin_family = AF_INET;
  (data->serv_addr).sin_port = htons(data->port);

  /* Convert IPv4 and IPv6 addresses from text to binary form */
  if (inet_pton(AF_INET, data->host_name, &(data->serv_addr).sin_addr) <= 0) {
    coco_error("socket_communication_data_initialize(): Invalid address / Address not supported");
  }

  /* Connect to the evaluator */
  if (connect(data->sock, (struct sockaddr*) &(data->serv_addr), sizeof(data->serv_addr)) < 0) {
    coco_error("socket_communication_data_initialize(): Connection failed (host = %s, port = %d)\nIs the server running?",
        data->host_name, data->port);
  }
#endif
  data->prev_message_obj = coco_strdup("");
  data->prev_response_obj = coco_strdup("");
  data->prev_message_con = coco_strdup("");
  data->prev_response_con = coco_strdup("");
  return data;
}

/**
 * Creates the message for the evaluator. The message has the following format:
 * s <s> t <t> r <r> f <f> i <i> d <d> x <x1> <x2> ... <xd>
 * Where
 * <s> is the suite name (for example, "toy-socket")
 * <t> is the type of evaluation (one of "objectives", "constraints", "add_info")
 * <r> is the number of values to be returned
 * <f> is the function number
 * <i> is the instance number
 * <d> is the problem dimension
 * <xi> is the i-th value of x (there should be exactly d x-values)
 */
static void socket_communication_create_message(char *message,
                                                const char *evaluation_type,
                                                const size_t number_of_values,
                                                const double *x,
                                                const coco_problem_t *problem) {
  size_t i;
  int write_count, offset;
  socket_communication_data_t *data;
  coco_suite_t *suite = problem->suite;

  assert(suite);
  data = (socket_communication_data_t *) suite->data;
  assert(data);

  offset = sprintf(message, "s %s t %s r %lu f %lu i %lu d %lu x ",
      suite->suite_name, evaluation_type,
      (unsigned long) number_of_values,
      (unsigned long) problem->suite_dep_function,
      (unsigned long) problem->suite_dep_instance,
      (unsigned long) problem->number_of_variables);
  for (i = 0; i < problem->number_of_variables; i++) {
    if (i < problem->number_of_integer_variables)
      write_count = sprintf(message + offset, "%d ", coco_double_to_int(x[i]));
    else
      write_count = sprintf(message + offset, "%.*e ", data->precision_x, x[i]);
    offset += write_count;
  }
}

/**
 * Reads the evaluator response and saves it into values. The response should have
 * the following format:
 * <v1> ... <vn>
  * Where
 * <vi> is the i-th value
 * n is the expected number of values
 */
static void socket_communication_save_response(const char *response,
                                               const size_t expected_number_of_values,
                                               double *values) {
  size_t i;
  int read_count, offset = 0;

  for (i = 0; i < expected_number_of_values; i++) {
    if (sscanf(response + offset, "%lf%*c%n", &values[i], &read_count) != 1) {
      fprintf(stderr, "socket_communication_save_response(): Failed to read response %s at %s",
          response, response + offset);
      exit(EXIT_FAILURE);
    }
    offset += read_count;
  }
}

/**
 * Sends the message to the external evaluator through sockets. The external evaluator must be running
 * a server using the same port. Returns the response.
 *
 * Should be working for different platforms.
 */
static char* socket_communication_get_response(const socket_communication_data_t *data,
                                               const char *message) {

#if WINSOCK
  size_t response_len;
#else
  long int response_len;
#endif
  char response[RESPONSE_SIZE];
  assert(data->sock);

#if WINSOCK
  /* Send message */
  if (send(data->sock, message, (int)strlen(message) + 1, 0) < 0) {
    coco_error("socket_communication_evaluate(): Send failed: %d", WSAGetLastError());
  }
  coco_debug("Sent message: %s", message);

  /* Receive the response */
  if ((response_len = (size_t) recv(data->sock, response, RESPONSE_SIZE, 0)) == SOCKET_ERROR) {
    coco_error("socket_communication_evaluate(): Receive failed: %d", WSAGetLastError());
  }
  response[response_len] = '\0';
  coco_debug("Received response: %s (length %d)", response, response_len);
#else
  /* Send message */
  if (send(data->sock, message, strlen(message) + 1, 0) < 0) {
    coco_error("socket_communication_evaluate(): Send failed");
  }
  coco_debug("Sent message: %s", message);

  /* Receive the response */
  response_len = read(data->sock, response, RESPONSE_SIZE);
  response[response_len] = '\0';
  coco_debug("Received response: %s (length %ld)", response, response_len);
#endif
  return coco_strdup(response);
}

/**
 * @brief Calls the external evaluator to evaluate the objective values for x.
 */
static void socket_evaluate_function(coco_problem_t *problem, const double *x, double *y) {

  char message[MESSAGE_SIZE];
  const char evaluation_type[] = "objectives";
  socket_communication_data_t *data;
  coco_suite_t *suite = problem->suite;

  assert(suite);
  data = (socket_communication_data_t *) suite->data;
  assert(data);

  socket_communication_create_message(message, evaluation_type, problem->number_of_objectives, x, problem);
  if (strcmp(message, data->prev_message_obj) != 0) {
    /* Get the response from the socket server */
    coco_free_memory(data->prev_message_obj);
    data->prev_message_obj = coco_strdup(message);
    coco_free_memory(data->prev_response_obj);
    data->prev_response_obj = socket_communication_get_response(data, message);
  }
  socket_communication_save_response(data->prev_response_obj, problem->number_of_objectives, y);
  coco_debug("objective message %s", message);
}

/**
 * @brief Calls the external evaluator to evaluate the constraint violations for x.
 */
static void socket_evaluate_constraint(coco_problem_t *problem, const double *x, double *y, int update_counter) {

  char message[MESSAGE_SIZE];
  const char evaluation_type[] = "constraints";
  socket_communication_data_t *data;
  coco_suite_t *suite = problem->suite;

  assert(suite);
  data = (socket_communication_data_t *) suite->data;
  assert(data);

  socket_communication_create_message(message, evaluation_type, problem->number_of_constraints, x, problem);
  if (strcmp(message, data->prev_message_con) != 0) {
    /* Get the response from the socket server */
    coco_free_memory(data->prev_message_con);
    data->prev_message_con = coco_strdup(message);
    coco_free_memory(data->prev_response_con);
    data->prev_response_con = socket_communication_get_response(data, message);
  }
  socket_communication_save_response(data->prev_response_con, problem->number_of_constraints, y);
  coco_debug("constraint message %s", message);
  (void) update_counter; /* To silence the compiler */
}


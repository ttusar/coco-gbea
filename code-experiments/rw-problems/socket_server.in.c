/**
 * The socket server in C.
 *
 * Uses the toy_socket_evaluator to evaluate problems from the toy-socket suite. Additional
 * evaluators can be used -- whether they are included or not depends on the respective
 * preprocessor directives (see the #define and #if directives below that start with EVALUATE_).
 * These definitions can be modified directly or through do.py.
 *
 * If the server receives the message 'RESET', it closes the current socket and opens a new one.
 * If the server receives the message 'SHUTDOWN', it shuts down.
 *
 * Change code below to connect it to other evaluators (for other suites) -- see occurrences
 * of 'ADD HERE'.
 */

/* The winsock2.h header *needs* to be included before windows.h! */
#if (defined(_WIN32) || defined(_WIN64) || defined(__MINGW64__) || defined(__CYGWIN__))
#include <winsock2.h>
#if _MSC_VER
#pragma comment(lib, "ws2_32.lib") /* Winsock library */
#endif
#define WINSOCK 1
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#define WINSOCK 0
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MESSAGE_SIZE 8192   /* Large enough for the entire message */
#define RESULT_PRECISION 16 /* Precision used to write objective values and constraint violations */
#define RESPONSE_SIZE 1024  /* Large enough for the entire response (objective values or constraint violations) */
#define STRING_SIZE 64      /* Large enough for a single string (the name of the suite etc.) */
/* Types of the evaluation function */
#define EVAL_TYPE_OBJ "objectives"
#define EVAL_TYPE_CON "constraints"

#include "toy_socket/toy_socket_evaluator.c"  /* Include the toy_evaluator for evaluation */

#define EVALUATE_RW_TOP_TRUMPS 0              /* Value can be modified through do.py */
#if EVALUATE_RW_TOP_TRUMPS > 0
#include "top_trumps/rw_top_trumps.h"         /* Include rw_top_trumps for evaluation */
#endif
/* ADD HERE includes of other evaluators, for example
#define EVALUATE_MY_EVALUATOR 0
#if EVALUATE_MY_EVALUATOR > 0
#include "my-suite/my_evaluator.c"
#endif
*/

/**
 * This is an interface for the evaluation function that needs to be implemented by the external
 * evaluators.
 */
typedef void (*evaluate_t)(char *suite_name, size_t number_of_values, size_t function,
    size_t instance, size_t dimension, const double *x, double *values);

/**
 * Parses the message and calls an evaluator to compute the evaluation (can be used to evaluate
 * objectives as well as constraints). Constructs and returns the response.
 */
char *evaluate_message(char *message) {

  char suite_name[STRING_SIZE];
  char evaluation_type[STRING_SIZE];
  size_t number_of_values, i;
  size_t function, instance, dimension;
  double *x, *values;
  char *response = "", *pointer;
  int read_count;
  int char_count, offset = 0;
  evaluate_t evaluate_objectives = NULL;
  evaluate_t evaluate_constraints = NULL;

  /* Parse the message
   *
   * char_count is used to count how many characters are read and offset moves the pointer
   * along the message accordingly
   */
  if ((read_count = sscanf(message, "s %s t %s r %lu f %lu i %lu d %lu x%*c%n", suite_name,
      evaluation_type, &number_of_values, &function, &instance, &dimension, &char_count)) != 6) {
    fprintf(stderr, "evaluate_message(): Failed to read beginning of the message %s", message);
    fprintf(stderr, "(read %d instead of %d items)", read_count, 6);
    exit(EXIT_FAILURE);
  }
  x = malloc(dimension * sizeof(double));
  offset = char_count;
  for (i = 0; i < dimension; i++) {
    if (sscanf(message + offset, "%lf%*c%n", &x[i], &char_count) != 1) {
      fprintf(stderr, "evaluate_message(): Failed to read message %s", message);
      exit(EXIT_FAILURE);
    }
    offset += char_count;
  }

  /* Choose the right function */
  if ((strcmp(suite_name, "toy-socket") == 0) || (strcmp(suite_name, "toy-socket-biobj") == 0)) {
    evaluate_objectives = evaluate_toy_socket_objectives;
    evaluate_constraints = evaluate_toy_socket_constraints;
  }
#if EVALUATE_RW_TOP_TRUMPS > 0
  else if ((strcmp(suite_name, "rw-top-trumps") == 0) || (strcmp(suite_name, "rw-top-trumps-biobj") == 0)) {
    evaluate_objectives = evaluate_rw_top_trumps;
  }
#endif
  /* ADD HERE the function for another evaluator, for example
  else if (strcmp(suite_name, "my-suite") == 0) {
    evaluate_objectives = evaluate_my_suite_objectives;
    evaluate_constraints = evaluate_my_suite_constraints;
  } */
  else {
    fprintf(stderr, "evaluate_message(): Suite %s not supported", suite_name);
    exit(EXIT_FAILURE);
  }

  /* Evaluate x and save the result to values */
  values = malloc(number_of_values * sizeof(double));
  if ((strcmp(evaluation_type, EVAL_TYPE_OBJ) == 0))
    evaluate_objectives(suite_name, number_of_values, function, instance, dimension, x, values);
  else if ((strcmp(evaluation_type, EVAL_TYPE_CON) == 0))
    evaluate_constraints(suite_name, number_of_values, function, instance, dimension, x, values);
  else {
    fprintf(stderr, "evaluate_message(): Evaluation type %s not supported", evaluation_type);
    exit(EXIT_FAILURE);
  }
  free(x);

  /* Construct the response (pointer keeps track of the current place in the response) */
  response = (char *) malloc(RESPONSE_SIZE);
  pointer = response;
  for (i = 0; i < number_of_values; i++) {
    char_count = sprintf(pointer, "%.*e ", RESULT_PRECISION, values[i]);
    pointer += char_count;
  }

  free(values);
  return response;
}

/**
 * Starts the server on the given port.
 *
 * Should be working for different platforms.
 */
void socket_server_start(unsigned short port, int silent) {

  int address_size;
  char message[MESSAGE_SIZE];
  char *response;

#if WINSOCK == 1
  WSADATA wsa;
  SOCKET sock, new_sock;
  SOCKADDR_IN address;
  int message_len;
  char yes = 0;

  /* Initialize Winsock */
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    fprintf(stderr, "socket_server_start(): Winsock initialization failed: %d", WSAGetLastError());
    return;
  }

  /* Create a socket file descriptor */
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
    fprintf(stderr, "socket_server_start(): Could not create socket: %d", WSAGetLastError());
    return;
  }

  /* Forcefully attach socket to the port */
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))) {
    fprintf(stderr, "socket_server_start(): Socket could not be attached to the port: %d", WSAGetLastError());
    return;
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY; /* "any address" in IPV4 */
  address.sin_port = htons(port);

  /* Bind */
  if (bind(sock, (SOCKADDR *) &address, sizeof(address)) < 0) {
    fprintf(stderr, "socket_server_start(): Bind failed: %d", WSAGetLastError());
    return;
  }

  /* Listen */
  if (listen(sock, 3) < 0) {
    fprintf(stderr, "socket_server_start(): Listen failed: %d", WSAGetLastError());
    return;
  }

  printf("Socket server (C) ready, listening on port %d\n", port);
  address_size = sizeof(address);

  while (1) {
    /* Accept an incoming connection */
    if ((new_sock = accept(sock, (SOCKADDR *) &address, &address_size)) == INVALID_SOCKET) {
      fprintf(stderr, "socket_server_start(): Accept failed: %d", WSAGetLastError());
      return;
    }

    while (1) {
      /* Read the message */
      if ((message_len = recv(new_sock, message, MESSAGE_SIZE, 0)) == SOCKET_ERROR) {
        fprintf(stderr, "socket_server_start(): Receive failed: %d", WSAGetLastError());
        closesocket(new_sock);
        return;
      }
      if (silent == 0)
        printf("Received message: %s (length %d)\n", message, message_len);

      /* Check if the message is a request for reset */
      if (strncmp(message, "RESET", strlen("RESET")) == 0) {
        printf("Socket server (C) reset\n");
        closesocket(new_sock);
        break;
      }

      /* Check if the message is a request for shut down */
      if (strncmp(message, "SHUTDOWN", strlen("SHUTDOWN")) == 0) {
        printf("Socket server (C) shut down \n");
        closesocket(new_sock);
        return;
      }

      /* Parse the message and evaluate its contents using an evaluator */
      response = evaluate_message(message);

      /* Send the response */
      send(new_sock, response, (int)strlen(response) + 1, 0);
      if (silent == 0)
        printf("Sent response %s (length %ld)\n", response, strlen(response));

      free(response);
    }
  }
  closesocket(new_sock);
#else
  int sock, new_sock;
  struct sockaddr_in address;
  long message_len;
  int yes = 0;

  /* Create a socket file descriptor */
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket_server_start(): Socket creation error");
    exit(EXIT_FAILURE);
  }

  /* Forcefully attach socket to the port */
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))) {
    perror("socket_server_start(): Socket could not be attached to the port");
    exit(EXIT_FAILURE);
  }
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY; /* "any address" in IPV4 */
  address.sin_port = htons(port);

  /* Bind */
  if (bind(sock, (struct sockaddr*) &address, sizeof(address)) < 0) {
    perror("socket_server_start(): Bind failed");
    exit(EXIT_FAILURE);
  }

  /* Listen */
  if (listen(sock, 3) < 0) {
    perror("socket_server_start(): Listen failed");
    exit(EXIT_FAILURE);
  }

  printf("Socket server (C) ready, listening on port %d\n", port);
  address_size = sizeof(address);

  while (1) {
    /* Accept an incoming connection */
    if ((new_sock = accept(sock, (struct sockaddr*) &address, (socklen_t*) &address_size)) < 0) {
      perror("socket_server_start(): Accept failed");
      exit(EXIT_FAILURE);
    }

    while (1) {
      /* Read the message */
      if ((message_len = read(new_sock, message, MESSAGE_SIZE)) < 0) {
        perror("socket_server_start(): Receive failed");
        close(new_sock);
        exit(EXIT_FAILURE);
      }
      if (silent == 0)
        printf("Received message: %s (length %ld)\n", message, message_len);

      /* Check if the message is a request for reset */
      if (strncmp(message, "RESET", strlen("RESET")) == 0) {
        printf("Socket server (C) reset\n");
        close(new_sock);
        break;
      }

      /* Check if the message is a request for shut down */
      if (strncmp(message, "SHUTDOWN", strlen("SHUTDOWN")) == 0) {
        printf("Socket server (C) shut down\n");
        close(new_sock);
        return;
      }

      /* Parse the message and evaluate its contents using an evaluator */
      response = evaluate_message(message);

      /* Send the response */
      send(new_sock, response, strlen(response) + 1, 0);
      if (silent == 0)
        printf("Sent response %s (length %ld)\n", response, strlen(response));

      free(response);
    }
  }
  close(new_sock);
#endif
}

int main(int argc, char* argv[])
{
  int silent = 0;
  long port;
  unsigned short port_short;

  if ((argc >= 2) && (argc <= 3)) {
    port = strtol(argv[1], NULL, 10);
    port_short = (unsigned short)port;
    printf("Socket server (C) called on port %d\n", port_short);
    if (argc == 3) {
      if (strcmp(argv[2], "silent") == 0) {
        silent = 1;
      } else {
        printf("Ignoring input option %s\n", argv[2]);
      }
    }
  }
  else {
    printf("Incorrect options\nUsage:\nsocket_server PORT <\"silent\">");
    return -1;
  }
  socket_server_start(port_short, silent);
  return 0;
}

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

/**
 * An evaluator of objectives for problems from the toy-socket suite to demonstrate external
 * evaluation in C.
 *
 * The memory for objectives needs to be allocated by the method that invokes this one.
 */
void evaluate_toy_socket_objectives(char *suite_name, size_t number_of_objectives, size_t function,
    size_t instance, size_t dimension, const double *x, double *objectives)
{
  double value;
  size_t i;

  if ((strcmp(suite_name, "toy-socket") == 0) && (number_of_objectives == 1)) {
    value = 1e-5 * (double)instance;
    if (function == 1) {
      /* Function 1 is the sum of the absolute x-values */
      for (i = 0; i < dimension; i++) {
        value += fabs(x[i]);
      }
      objectives[0] = value;
    } else if (function == 2) {
      /* Function 2 is the sum of squares of all x-values */
      for (i = 0; i < dimension; i++) {
        value += x[i] * x[i];
      }
      objectives[0] = value;
    } else {
      fprintf(stderr, "evaluate_toy_socket_objectives(): suite %s has no function %lu", suite_name, function);
      exit(EXIT_FAILURE);
    }
  } else if ((strcmp(suite_name, "toy-socket-biobj") == 0) && (number_of_objectives == 2)) {
    if ((function == 1) || (function == 2)) {
      /* Objective 1 is the sum of the absolute x-values */
      value = 1e-5 * (double)instance;
      for (i = 0; i < dimension; i++) {
        value += fabs(x[i]);
      }
      objectives[0] = value;
      /* Objective 2 is the sum of squares of all x-values */
      value =1e-5 * (double)instance;
      for (i = 0; i < dimension; i++) {
        value += x[i] * x[i];
      }
      objectives[1] = value;
    } else {
      fprintf(stderr, "evaluate_toy_socket_objectives(): suite %s has no function %lu", suite_name, function);
      exit(EXIT_FAILURE);
    }
  } else {
    fprintf(stderr, "evaluate_toy_socket_objectives(): suite %s cannot have %lu objectives",
        suite_name, number_of_objectives);
    exit(EXIT_FAILURE);
  }

}

/**
 * An evaluator of constraints for problems from the toy-socket suite to demonstrate external
 * evaluation in C.
 *
 * The memory for constraints needs to be allocated by the method that invokes this one.
 */
void evaluate_toy_socket_constraints(char *suite_name, size_t number_of_constraints, size_t function,
    size_t instance, size_t dimension, const double *x, double *constraints)
{
  double average;
  size_t i;

  assert(dimension > 0);
  average = 1e-5 * (double)instance;
  for (i = 0; i < dimension; i++) {
    average += fabs(x[i]);
  }
  average = average / (double)dimension;

  if ((strcmp(suite_name, "toy-socket") == 0) && (function == 1) && (number_of_constraints == 2)) {
    /* Function 1 of the single-objective suite has two constraints */
    /* Constraint violation 1 is the difference between the 0.2 and the average of absolute x-values */
    constraints[0] = (average < 0.2) ? (0.2 - average) : 0;
    /* Constraint violation 2 is the difference between the average and 0.5 */
    constraints[1] = (average > 0.5) ? (average - 0.5) : 0;
  } else if ((strcmp(suite_name, "toy-socket-biobj") == 0) && (function == 2) && (number_of_constraints == 1)) {
    /* Function 2 of the bi-objective suite has one constraint */
    /* Constraint violation 1 is the difference between the average and 0.5 */
    constraints[0] = (average > 0.5) ? (average - 0.5) : 0;
  } else {
    fprintf(stderr, "evaluate_toy_socket_constraints(): suite %s function %lu does not have %lu constraints",
        suite_name, function, number_of_constraints);
    exit(EXIT_FAILURE);
  }

}

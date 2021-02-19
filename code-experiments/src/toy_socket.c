/**
 * @file toy_socket.c
 *
 * @brief Implementation of a toy problem to demonstrate communication with an external evaluator
 * through sockets.
 */
#include "coco.h"
#include "coco_platform.h"
#include "socket_communication.c"


/**
 * @brief Creates the toy-socket problem.
 */
static coco_problem_t *toy_socket_problem_allocate(const size_t number_of_objectives,
                                                   const size_t number_of_constraints,
                                                   const size_t function,
                                                   const size_t dimension,
                                                   const size_t instance,
                                                   const char *problem_id_template,
                                                   const char *problem_name_template) {

  coco_problem_t *problem = NULL;
  size_t i;

  if ((number_of_objectives != 1) && (number_of_objectives != 2))
    coco_error("toy_socket_problem_allocate(): %lu objectives are not supported (only 1 or 2)",
        (unsigned long)number_of_objectives);

  /* Provide the region of interest */
  problem = coco_problem_allocate(dimension, number_of_objectives, number_of_constraints);
  for (i = 0; i < dimension; ++i) {
    problem->smallest_values_of_interest[i] = -1;
    problem->largest_values_of_interest[i] = 1;
  }
  problem->number_of_integer_variables = 0;
  problem->evaluate_function = socket_evaluate_function;
  problem->evaluate_constraint = socket_evaluate_constraint;

  coco_problem_set_id(problem, problem_id_template, function, instance, dimension);
  coco_problem_set_name(problem, problem_name_template, function, instance, dimension);
  coco_problem_set_type(problem, "toy_socket");

  problem->is_opt_known = 0;
  /* Unknown best_parameter */
  if (problem->best_parameter != NULL) {
    coco_free_memory(problem->best_parameter);
    problem->best_parameter = NULL;
  }

  if (number_of_objectives == 1) {
    /* Since best value is unknown, provide a reference point */
    if (problem->best_value != NULL) {
      assert(problem->best_value);
      problem->best_value[0] = 0.0;
    }
  }
  else if (number_of_objectives == 2) {
    /* Need to provide estimation of the ideal and nadir points for all bi-objective problem instances */
    problem->best_value[0] = 0;
    problem->best_value[1] = 0;
    problem->nadir_value[0] = 15;
    problem->nadir_value[1] = 15;
  }
  return problem;
}

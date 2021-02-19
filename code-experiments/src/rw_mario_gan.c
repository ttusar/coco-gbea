/**
 * @file rw_mario_gan.c
 *
 * @brief Implementation of Mario GAN functions
 */
#include "coco.h"
#include "coco_platform.h"
#include "socket_communication.c"


/**
 * @brief Creates the rw-mario-gan problem.
 */
static coco_problem_t *rw_mario_gan_problem_allocate(const size_t number_of_objectives,
                                                     const size_t function,
                                                     const size_t dimension,
                                                     const size_t instance,
                                                     const char *problem_id_template,
                                                     const char *problem_name_template) {

  coco_problem_t *problem = NULL;
  size_t i;

  if ((number_of_objectives != 1) && (number_of_objectives != 2))
    coco_error("rw_mario_gan_problem_allocate(): %lu objectives are not supported (only 1 or 2)",
        (unsigned long)number_of_objectives);

  /* Provide the region of interest */
  problem = coco_problem_allocate(dimension, number_of_objectives, 0);
  for (i = 0; i < dimension; ++i) {
    problem->smallest_values_of_interest[i] = -1;
    problem->largest_values_of_interest[i] = 1;
  }
  problem->number_of_integer_variables = 0;
  problem->evaluate_function = socket_evaluate_function;

  coco_problem_set_id(problem, problem_id_template, function, instance, dimension);
  coco_problem_set_name(problem, problem_name_template, function, instance, dimension);

  /* Set type (for grouping) */
  if (number_of_objectives == 1) {
    if (function <= 10) {
      if (function % 2 == 0)
        coco_problem_set_type(problem, "direct-underground");
      else
        coco_problem_set_type(problem, "direct-overworld");
    }
    else if ((function == 11) || (function == 15) || (function == 17) ||
        (function == 21) || (function == 23) || (function == 27))
      coco_problem_set_type(problem, "simulated-overworld-single");
    else if ((function == 12) || (function == 16) || (function == 18) ||
        (function == 22) || (function == 24) || (function == 28))
      coco_problem_set_type(problem, "simulated-underground-single");
    else if ((function == 13) || (function == 19) || (function == 25))
      coco_problem_set_type(problem, "simulated-overworld-concatenated");
    else if ((function == 14) || (function == 20) || (function == 26))
      coco_problem_set_type(problem, "simulated-underground-concatenated");
  } else if (number_of_objectives == 2) {
    if (function <= 2)
      coco_problem_set_type(problem, "direct-underground");
    else if (function <= 4)
      coco_problem_set_type(problem, "simulated-overworld-single");
    else if (function <= 6)
      coco_problem_set_type(problem, "simulated-underground-single");
    else if (function <= 8)
      coco_problem_set_type(problem, "simulated-overworld-concatenated");
    else if (function <= 10)
      coco_problem_set_type(problem, "simulated-underground-concatenated");
  }

  problem->is_opt_known = 0;
  /* Unknown best_parameter */
  if (problem->best_parameter != NULL) {
    coco_free_memory(problem->best_parameter);
    problem->best_parameter = NULL;
  }

  if (number_of_objectives == 1) {
    /* Since best value is unknown, provide a reference point */
    assert(problem->best_value);
    problem->best_value[0] = 0.0;
  }
  else if (number_of_objectives == 2) {
    /* Need to provide estimation of the ideal and nadir points for all bi-objective problem instances */
    problem->best_value[0] = 0;
    problem->best_value[1] = 0;
    problem->nadir_value[0] = 1;
    problem->nadir_value[1] = 1;
  }
  return problem;
}

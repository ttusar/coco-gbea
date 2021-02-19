#include "coco.h"
#include "coco_platform.h"
#include "socket_communication.c"


static void rw_top_trumps_set_bounds(coco_problem_t *problem,
                                     const size_t instance) {

  /* Low and high bounds are given for each of the 15 instances (rows) and repeated every
   * 4 dimensions (columns) */
  double low_bound[15][4] = {
      { 39,  78,  20,  34},
      { 70,   9,  35,   7},
      { 22,  39,  14,  56},
      { 13,  19,  21,  42},
      {  5,   8,   8,  28},
      { 14,  40,   9,  25},
      { 49,  21,   1,   3},
      { 35,   4,  25,  84},
      { 21,  20,  46,  63},
      { 57,  18,  18,  42},
      { 53,  31,  41,  22},
      { 44,  12,  13,  31},
      { 34,  13,  33,  16},
      { 26,  22,   5,   3},
      { 35,  28,  78,  39}
  };
  double high_bound[15][4] = {
      { 84,  80,  91,  77},
      { 81,  12,  42,  70},
      { 56,  44,  29,  86},
      { 92,  26,  36,  65},
      { 27,  99,  15,  45},
      { 96,  80,  81,  65},
      { 87,  59,  51, 100},
      { 79,  91,  95,  87},
      { 70,  36,  88,  72},
      { 61,  51,  82,  58},
      { 93,  50,  75,  45},
      { 79,  82,  69,  52},
      { 63,  90,  61,  79},
      {100,  46,  57,  60},
      { 67,  52,  98,  39}
  };
  size_t i;

  if (instance - 1 > 15) {
    coco_error("rw_top_trumps_set_bounds(): instance number %lu not supported", instance);
  }

  for (i = 0; i < problem->number_of_variables; i++) {
    problem->smallest_values_of_interest[i] = low_bound[instance - 1][i % 4];
    problem->largest_values_of_interest[i] = high_bound[instance - 1][i % 4];
  }
}

/**
 * @brief Creates a single- or bi-objective rw-top-trumps problem.
 */
static coco_problem_t *rw_top_trumps_problem_allocate(const size_t number_of_objectives,
                                                      const size_t function,
                                                      const size_t dimension,
                                                      const size_t instance,
                                                      const char *problem_id_template,
                                                      const char *problem_name_template) {

  coco_problem_t *problem = NULL;

  if ((number_of_objectives != 1) && (number_of_objectives != 2))
      coco_error("rw_top_trumps_problem_allocate(): %lu objectives are not supported (only 1 or 2)",
        (unsigned long)number_of_objectives);

  problem = coco_problem_allocate(dimension, number_of_objectives, 0);
  rw_top_trumps_set_bounds(problem, instance);

  problem->number_of_integer_variables = dimension;
  problem->evaluate_function = socket_evaluate_function;
  problem->evaluate_constraint = socket_evaluate_constraint;

  coco_problem_set_id(problem, problem_id_template, function, instance, dimension);
  coco_problem_set_name(problem, problem_name_template, function, instance, dimension);
  coco_problem_set_type(problem, "rw-top-trumps");

  /* Set type (for grouping) */
  if (number_of_objectives == 1) {
    if (function <= 2)
      coco_problem_set_type(problem, "direct");
    else
      coco_problem_set_type(problem, "simulated");
  } else if (number_of_objectives == 2) {
    if (function == 1)
      coco_problem_set_type(problem, "direct");
    else
      coco_problem_set_type(problem, "simulated");
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
    problem->best_value[0] = 0;
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


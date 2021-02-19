/**
 * @file suite_rw_mario_gan_biobj.c
 *
 * @brief Implementation of the bi-objective suite Mario GAN. A single-objective version can be
 * found in the file suite_rw_mario_gan.c
 */

#include "coco.h"
#include "rw_mario_gan.c"
#include "socket_communication.c"

static coco_suite_t *coco_suite_allocate(const char *suite_name,
                                         const size_t number_of_functions,
                                         const size_t number_of_dimensions,
                                         const size_t *dimensions,
                                         const char *default_instances,
                                         const int known_optima);

/**
 * @brief Sets the dimensions and default instances for the rw-mario-gan-biobj suite.
 */
static coco_suite_t *suite_rw_mario_gan_biobj_initialize(const char *suite_options) {

  coco_suite_t *suite;
  const size_t dimensions[] = { 10, 20, 30, 40 };

  suite = coco_suite_allocate("rw-mario-gan-biobj", 10, 4, dimensions, "instances: 1-7", 0);

  suite->data = socket_communication_data_initialize(suite_options, 7200);
  suite->data_free_function = socket_communication_data_finalize;
  return suite;
}

/**
 * @brief Sets the instances associated with years.
 */
static const char *suite_rw_mario_gan_biobj_get_instances_by_year(const int year) {
   if (year == 0) {
    return "1";
  }
  else {
    coco_error("suite_mario_gan_biobj_get_instances_by_year(): year %d not defined for suite rw-mario-gan-biobj", year);
    return NULL;
  }
}

/**
 * @brief Returns the problem from the mario-gan-biobj suite that corresponds to the given parameters.
 *
 * @param suite The COCO suite.
 * @param function_idx Index of the function (starting from 0).
 * @param dimension_idx Index of the dimension (starting from 0).
 * @param instance_idx Index of the instance (starting from 0).
 * @return The problem that corresponds to the given parameters.
 */
static coco_problem_t *suite_rw_mario_gan_biobj_get_problem(coco_suite_t *suite,
                                                            const size_t function_idx,
                                                            const size_t dimension_idx,
                                                            const size_t instance_idx) {

  coco_problem_t *problem = NULL;

  const size_t function = suite->functions[function_idx];
  const size_t dimension = suite->dimensions[dimension_idx];
  const size_t instance = suite->instances[instance_idx];

  const char *problem_id_template = "rw-mario-gan-biobj_f%02lu_i%02lu_d%02lu";
  const char *problem_name_template = "bi-objective Mario GAN suite problem f%lu instance %lu in %luD";

  problem = rw_mario_gan_problem_allocate(2, function, dimension, instance,
      problem_id_template, problem_name_template);
  assert(problem != NULL);

  problem->suite_dep_function = function;
  problem->suite_dep_instance = instance;
  problem->suite_dep_index = coco_suite_encode_problem_index(suite, function_idx, dimension_idx, instance_idx);

  return problem;
}

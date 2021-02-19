"""Classes needed to solve COCO problems using jMetalPy"""

import random
from cocoex import Problem as CocoexProblem
from jmetal.core.solution import FloatSolution
from jmetal.core.problem import Problem as JMetalProblem
from typing import List, TypeVar
from abc import ABC
S = TypeVar('S')


class CocoSolution(FloatSolution):
    """ Class representing a 'COCO solution', a mixed-integer solution which is nevertheless
    encoded using float variables """

    def __init__(self, lower_bound: List[float], upper_bound: List[float],
                 number_of_objectives: int, number_of_constraints: int = 0,
                 number_of_integer_variables: int = 0):
        super(CocoSolution, self).__init__(lower_bound, upper_bound, number_of_objectives,
                                           number_of_constraints)
        self.lower_bound = lower_bound
        self.upper_bound = upper_bound
        self.number_of_integer_variables = number_of_integer_variables

    def __copy__(self):
        new_solution = CocoSolution(
            lower_bound=self.lower_bound,
            upper_bound=self.upper_bound,
            number_of_objectives=self.number_of_objectives,
            number_of_constraints=self.number_of_constraints,
            number_of_integer_variables=self.number_of_integer_variables
        )
        new_solution.objectives = self.objectives[:]
        new_solution.variables = self.variables[:]
        new_solution.constraints = self.constraints[:]
        new_solution.attributes = self.attributes.copy()
        return new_solution


class CocoProblem(JMetalProblem[CocoSolution], ABC):
    """ Class representing a COCO problem. """

    def __init__(self, problem: CocoexProblem, use_as_continuous=True):
        """If use_as_continuous, the bounds of the integer variables are shifted so that an
        algorithm can use them as if they were continuous"""
        super(CocoProblem, self).__init__()
        self.problem = problem
        self.number_of_variables = problem.dimension
        self.number_of_objectives = problem.number_of_objectives
        self.number_of_constraints = problem.number_of_constraints
        self.number_of_integer_variables = problem.number_of_integer_variables
        self.lower_bound = problem.lower_bounds
        self.upper_bound = problem.upper_bounds
        if use_as_continuous:
            for i in range(self.number_of_integer_variables):
                self.lower_bound[i] -= (0.5 - 1e-6)
                self.upper_bound[i] += (0.5 - 1e-6)
        self.reference_point = problem.largest_fvalues_of_interest

    def create_solution(self) -> CocoSolution:
        new_solution = CocoSolution(
            lower_bound=self.lower_bound,
            upper_bound=self.upper_bound,
            number_of_objectives=self.number_of_objectives,
            number_of_constraints=self.number_of_constraints,
            number_of_integer_variables=self.number_of_integer_variables
        )
        new_solution.variables = [
            random.uniform(self.lower_bound[i]*1.0, self.upper_bound[i]*1.0)
            for i in range(self.number_of_variables)]
        for i in range(self.number_of_integer_variables):
            new_solution.variables[i] = round(new_solution.variables[i])
        return new_solution

    def evaluate(self, solution: CocoSolution) -> CocoSolution:
        if len(solution.constraints) > 0:
            solution.constraints = self.problem.constraint(solution.variables)
        solution.objectives = self.problem(solution.variables)
        return solution

    def get_name(self) -> str:
        return self.problem.id

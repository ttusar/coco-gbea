"""
An evaluator for problems from the toy-socket suite to demonstrate external evaluation of
objectives and constraints in Python
"""


def evaluate_toy_socket_objectives(suite_name, func, instance, x):
    y0 = instance * 1e-5
    if suite_name == 'toy-socket':
        if func == 1:
            # Function 1 is the sum of the absolute x-values
            return [y0 + sum([abs(xi) for xi in x])]
        elif func == 2:
            # Function 2 is the sum of squares of all x-values
            return [y0 + sum([xi * xi for xi in x])]
        else:
            raise ValueError('Suite {} has no function {}'.format(suite_name, func))
    elif suite_name == 'toy-socket-biobj':
        if func == 1 or func == 2:
            # Objective 1 is the sum of the absolute x-values
            y1 = y0 + sum([abs(xi) for xi in x])
            # Objective 2 is the sum of squares of all x-values
            y2 = y0 + sum([xi * xi for xi in x])
            return [y1, y2]
        else:
            raise ValueError('Suite {} has no function {}'.format(suite_name, func))
    else:
        raise ValueError('Suite {} not supported'.format(suite_name))


def evaluate_toy_socket_constraints(suite_name, func, instance, x):
    y0 = instance * 1e-5
    average = y0 + sum([abs(xi) for xi in x])
    average = average / len(x)

    if suite_name == 'toy-socket' and func == 1:
        # Function 1 of the single-objective suite has two constraints
        # Violation 1 is the difference between the 0.2 and the average of absolute x-values
        y1 = (0.2 - average) if average < 0.2 else 0
        # Violation 2 is the difference between the average and 0.5
        y2 = (average - 0.5) if average > 0.5 else 0
        return [y1, y2]
    elif suite_name == 'toy-socket-biobj' and func == 2:
        # Function 3 of the bi-objective suite has one constraint
        # Violation 1 is the difference between the average and 0.5
        y1 = (average - 0.5) if average > 0.5 else 0
        return [y1]
    else:
        raise ValueError('Suite {} function {} does not have constraints'.format(suite_name, func))

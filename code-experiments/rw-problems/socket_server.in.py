"""
The socket server in Python.
Uses the toy_socket_evaluator to evaluate problems from the toy-socket suite. Additional evaluators
can be used -- whether they are included or not depends on the values of the respective variables
(see the variables that start with EVALUATE_). These definitions can be modified directly or through
do.py.
If the server receives the message 'RESET', it closes the current socket and opens a new one.
If the server receives the message 'SHUTDOWN', it shuts down.
Change code below to connect it to other evaluators (for other suites) -- see occurrences of
'ADD HERE'.
Note that separate functions are used for evaluating objectives and constraints.
"""
import sys
import socket
from toy_socket.toy_socket_evaluator import evaluate_toy_socket_objectives
from toy_socket.toy_socket_evaluator import evaluate_toy_socket_constraints

HOST = '127.0.0.1'        # Local host
MESSAGE_SIZE = 8192       # Large enough for a number of x-values
RESULT_PRECISION = 16     # Precision used to write objective and constraint values
# Types of the evaluation function
EVAL_TYPE_OBJ = 'objectives'
EVAL_TYPE_CON = 'constraints'

EVALUATE_RW_MARIO_GAN = 0
if EVALUATE_RW_MARIO_GAN:
    from mario_gan.mario_gan_evaluator import evaluate_mario_gan
# ADD HERE imports from other evaluators, for example
# EVALUATE_MY_EVALUATOR = 0
# if EVALUATE_MY_EVALUATOR:
# from .my_suite.my_evaluator import evaluate_my_suite_objectives, evaluate_my_suite_constraints
#
# The evaluators should support the following usage:
# result = evaluate_my_suite(suite_name, function, instance, x)
# where the result is a list of values (even if it contains a single value)


def evaluate_message(message):
    """Parses the message and calls an evaluator to compute the evaluation. Then constructs a
    response. Returns the response.
    If the number of objectives ('o') is greater than 0, invokes evaluation of objectives.
    If the number of constraints ('c') is greater than 0, invokes evaluation of constraints.
    """
    try:
        # Parse the message
        msg = message.split(' ')
        suite_name = msg[msg.index('s') + 1]
        evaluation_type = msg[msg.index('t') + 1]
        num_values = int(msg[msg.index('r') + 1])
        func = int(msg[msg.index('f') + 1])
        dimension = int(msg[msg.index('d') + 1])
        instance = int(msg[msg.index('i') + 1])
        x = [float(m) for m in msg[msg.index('x') + 1:] if m != '']
        if len(x) != dimension:
            raise ValueError('Number of x values {} does not match dimension {}'.format(len(x),
                                                                                        dimension))

        # Find the right evaluator
        evaluate_objectives = None
        evaluate_constraints = None
        if 'toy-socket' in suite_name:
            evaluate_objectives = evaluate_toy_socket_objectives
            evaluate_constraints = evaluate_toy_socket_constraints
        elif EVALUATE_RW_MARIO_GAN > 0:
            if 'mario-gan' in suite_name or 'mario-gan-biobj' in suite_name:
                evaluate_objectives = evaluate_mario_gan
        # ADD HERE the functions for another evaluator, for example
        # elif EVALUATE_MY_EVALUATOR > 0:
        #     if 'my-suite' in suite_name:
        #         evaluate_objectives = evaluate_my_suite_objectives
        #         evaluate_constraints = evaluate_my_suite_constraints
        else:
            raise ValueError('Suite {} not supported'.format(suite_name))

        if evaluation_type == EVAL_TYPE_OBJ:
            # Evaluate the objective values of x and save the result to values
            values = evaluate_objectives(suite_name, func, instance, x)
        elif evaluation_type == EVAL_TYPE_CON:
            # Evaluate the constraint violations of x and save the result to values
            values = evaluate_constraints(suite_name, func, instance, x)
        else:
            raise ValueError('Evaluation type {} not supported'.format(evaluation_type))

        if len(values) != num_values:
            raise ValueError('Number of result values {} does not match {}'.format(len(values),
                                                                                   num_values))

        # Construct the response
        response = ''
        for value in values:
            response += '{:.{p}e} '.format(value, p=RESULT_PRECISION)
        return str.encode(response)
    except Exception as e:
        print('Error within message evaluation: {} \nMessage = {}'.format(e, message))
        raise e


def socket_server_start(port, silent=False):
    s = None
    try:
        # Create socket
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        # Bind socket to local host and port
        try:
            s.bind((HOST, port))
        except socket.error as e:
            print('Bind failed: {}'.format(e))
            raise e

        # Start listening on socket
        s.listen(1)
        print('Socket server (Python) ready, listening on port {}'.format(port))

        # Talk with the client
        while True:
            try:
                # Wait to accept a connection - blocking call
                conn, addr = s.accept()
            except socket.error as e:
                print('Accept failed: {}'.format(e))
                raise e
            with conn:
                while True:
                    # Read the message
                    message = conn.recv(MESSAGE_SIZE).decode("utf-8")
                    # Make sure to remove and null endings
                    message = message.split('\x00', 1)[0]
                    if not silent:
                        print('Received message: {}'.format(message))
                    # Check if the message is a request for reset
                    if message == 'RESET':
                        print('Socket server (Python) reset')
                        break
                    # Check if the message is a request for shut down
                    if message == 'SHUTDOWN':
                        print('Socket server (Python) shut down')
                        return
                    # Parse the message and evaluate its contents using an evaluator
                    response = evaluate_message(message)
                    # Send the response
                    conn.sendall(response)
                    if not silent:
                        print('Sent response: {}'.format(response.decode("utf-8")))
    except KeyboardInterrupt or SystemExit as e:
        print('Socket server (Python) terminated: {}'.format(e))
        raise e
    except Exception as e:
        print('Socket server (Python) error: {}'.format(e))
        raise e
    finally:
        if s is not None:
            s.close()


if __name__ == '__main__':
    silent = False
    port = None
    if 2 <= len(sys.argv) <= 3:
        port = int(sys.argv[1])
        print('Socket server (Python) called on port {}'.format(port))
        if len(sys.argv) == 3:
            if sys.argv[2] == 'silent':
                silent = True
            else:
                print('Ignoring input option {}'.format(sys.argv[2]))
    else:
        print('Incorrect options\nUsage:\nsocket_server PORT <\"silent\">')
        exit(-1)
    socket_server_start(port=port, silent=silent)
# Using socket communication for external evaluation 

| To get started with the GBEA suites, see [GBEA readme](GBEA.md) |
|-----------------------------------------------------------------|

Socket communication between COCO and an external evaluator can be used to ease inclusion 
of new suites of problems into COCO, for example, those that implement real-world problems. 
Socket communication is demonstrated on the example of two test suites, `toy-socket` and 
`toy-socket-biobj`; the first contains single-objective and the second bi-objective optimization problems.

Other currently supported suites that use this kind of external evaluation of
solutions are `rw-top-trumps(-biobj)` and `rw-mario-gan(-biobj)`. They are not included in the
COCO repository directly, but are downloaded when invoked (they are then stored in the
`top_trumps` and `mario_gan` folders of the `code-experiments/rw-problems` folder).
See [GBEA.md](GBEA.md) for more information.

An external evaluator is basically a server that listens for messages
from the client (COCO). For each evaluation of the objectives or constraints
for a solution, COCO sends a message with information
on the problem and the solution to the external evaluator. The external evaluator then 
computes and returns the objective values (or constraint violations) to COCO as a response message.

Two external evaluators are available - one in Python and the other in C (see the files
in this folder). Both are implemented using two files - one takes care of the socket 
communication (files `socket_server.py` and `socket_server.c` for Python and C, respectively) 
and the other of the actual evaluation of solutions (files `toy_socket_evaluator.py` and 
`toy_socket_evaluator.c` in the `toy_socket` folder for Python and C, respectively). 

It should be rather easy to add additional (real-world) evaluators to the socket servers, 
look for text starting with `ADD HERE` in the files `socket_server.py` and `socket_server.c`.
Currently, the socket server in C supports also the `rw-top-trumps(-biobj)` suites and the socket
server in Python the `rw-mario-gan(-biobj)` suites.

Note that the interfaces to other languages supported by COCO works seamlessly with these external
evaluators.

## Running experiments

### Running a prepared experiment on the `toy-socket` suite

By calling

````
python do.py run-rw-experiment
````

form the root directory of the repository, the following will happen:
- the C socket server will be started
- the Python example experiment will be run on the `toy-socket` suite
- when the example experiment will complete, the C socket server will be stopped

The results of the experiment will be saved to the folder `code-experiments/build/python/exdata`.

### Starting/stopping the socket server(s)

Both socket servers (configured so that they can call any available evaluator) are started by calling

````
python do.py run-socket-servers <port=1234>
````

and stopped by calling

````
python do.py stop-socket-servers <port=1234>
````

The ports used by the socket servers in C and Python are set to `7000` and `7200` (respectively)
by default to enable both servers to work at the same time. The ports can be changed as shown
in the code snippet above, however, this also entails setting the port number in the suite
options (the syntax used in suite options is `"port: 1234"`). In general it should be fine to leave
the ports unchanged.

Running the servers in this way enables to freely use any of the available external evaluators.
If, however, one wishes to run only the server for a specific evaluator, this can be done by invoking

````
python do.py run-toy-socket-server <port=1234>
````
for the `toy-socket` evaluator

````
python do.py run-rw-top-trumps-server <port=1234>
````
for the `rw-top-trumps` evaluator and

````
python do.py run-rw-mario-gan-server <port=1234>
````
for the `rw-mario-gan` evaluator.

The advantage of these specific calls is that they do not require to download and compile the 
external evaluators
that are not needed.

To stop these servers, simply call
````
python do.py stop-socket-servers <port=1234>
````

Without a specific port, this command will stop the sockets only on the default ports. 

Note that the `python do.py run-*-server*` calls above change some of the files in the 
`code-experiments/rw-problems` folder (this is needed to configure the builds). All changes are 
reverted by the call `python do.py stop-socket-servers`.

### Running custom experiments

Before running your own experiment that needs external evaluation of solutions, make sure to
run the required socket server(s) as described above. Then the experiment can be carried out as
usually. After the experiment, call `python do.py stop-socket-servers` to stop the server(s).

For more information on how to run experiments on the GBEA suites (`rw-top-trumps(-biobj)` and 
`rw-mario-gan(-biobj)`), see [GBEA.md](GBEA.md).
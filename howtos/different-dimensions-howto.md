How to experiment with different dimensions
===========================================

The problem dimensions are hard-coded in the suites,
but it is still possible to run experiments with other dimensions.
To do so, follow these three steps:

##### 1. Find the suite you are interested in

Look in the `code-experiments/src` folder for a file named `suite_NAME.c`, where `NAME`
is the name of the suite you are interested in. In that file, find the 
`suite_NAME_initialize(void)` function. 

##### 2. Change the suite dimensions

The array of dimensions looks something like: 
```c
  const size_t dimensions[] = { 2, 3, 5, 10, 20, 40 };
```

Use the dimensions you wish (see the disclaimer below).

##### 3. Recompile the code

You need to recompile the code, which is done by any of the `python do.py build-LANG` or
`python do.py run-LANG` commands (where `LANG` is one of the supported languages).

Experiments will now use the new dimensions.

##### 4. Post-processing

The (probably) only changes that need to be made in the postprocessing are in `cocopp.testbedsettings`. Search for the corresponding `Testbed` class (there is one per test suite, for example `GECCOBBOBTestbed` for the `bbob` test suite). In each `Testbed` class, there are variables that are related to the displayed dimensions. If you change these accordingly, it might run already "out of the box" (not tested! please submit an issue if this does not work!):

```python
import cocopp
cocopp.testbedsettings.GECCOBBOBTestbed.settings['dimensions_to_display'] = (10, 15, 20, 25, 30, 40)
cocopp.testbedsettings.GECCOBBOBTestbed.settings['goto_dimension'] = 30
cocopp.testbedsettings.GECCOBBOBTestbed.settings['reference_algorithm_filename'] = ''  # no reference algorithm because it is only defined for standard dimensions
cocopp.main('youralgorithmfolder/')
```

##### Disclaimer

The `bbob` functions do not work for dimension `d=1`. 

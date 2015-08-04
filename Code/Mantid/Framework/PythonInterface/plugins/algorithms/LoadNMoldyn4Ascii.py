#pylint: disable=no-init

from mantid.simpleapi import *
from mantid.kernel import *
from mantid.api import *

import numpy as np
import ast
import re

#------------------------------------------------------------------------------

VARIABLE_REGEX = re.compile(r'#\s+variable name:\s+(.*)')
TYPE_REGEX = re.compile(r'#\s+type:\s+([A-z]+)')
AXIS_REGEX = re.compile(r'#\s+axis:\s+([A-z]+)\|([A-z]+)')
UNIT_REGEX = re.compile(r'#\s+units:\s+(.*)')

SLICE_1D_HEADER_REGEX = re.compile(r'#slice:\[([0-9]+)L\]')
SLICE_2D_HEADER_REGEX = re.compile(r'#slice:\[([0-9]+)L,\s+([0-9]+)L\]')

#------------------------------------------------------------------------------

class LoadNMoldyn4Ascii(PythonAlgorithm):

    _axis_cache = None

    _data_directory = None
    _functions = None

#------------------------------------------------------------------------------

    def category(self):
        return 'PythonAlgorithms;Inelastic;Simulation'


    def summary(self):
        return 'Imports functions from .dat files output by nMOLDYN 4.'

#------------------------------------------------------------------------------

    def PyInit(self):
        self.declareProperty(FileProperty('Filename', '',
                                          action=FileAction.Directory),
                             doc='Path to directory containg .dat files')

        self.declareProperty(StringArrayProperty('Functions'),
                             doc='Names of functions to attempt to load from file')

        self.declareProperty(WorkspaceProperty('OutputWorkspace', '',
                                               direction=Direction.Output),
                             doc='Output workspace name')

#------------------------------------------------------------------------------

    def validateInputs(self):
        self._setup()
        issues = dict()

        if len(self._functions) == 0:
            issues['Functions'] = 'Must specify at least one function to load'

        return issues

#------------------------------------------------------------------------------

    def PyExec(self):
        self._setup()
        loaded_function_workspaces = []

        for func_name in self._functions:
            try:
                # Load the intensity data
                function = self._load_function(func_name)
                # Load (or retrieve) the axis data
                v_axis = self._load_axis(function[2][0])
                x_axis = self._load_axis(function[2][1])

                # Create the workspace for function
                create_workspace = AlgorithmManager.Instance().create('CreateWorkspace')
                create_workspace.initialize()
                create_workspace.setLogging(False)
                create_workspace.setProperty('OutputWorkspace', func_name)
                create_workspace.setProperty('DataX', x_axis[0])
                create_workspace.setProperty('DataY', function[0])
                create_workspace.setProperty('NSpec', v_axis[0].size)
                create_workspace.setProperty('UnitX', x_axis[1])
                create_workspace.setProperty('YUnitLabel', function[1])
                create_workspace.setProperty('VerticalAxisValues', v_axis[0])
                create_workspace.setProperty('VerticalAxisUnit', 'Empty')
                create_workspace.setProperty('WorkspaceTitle', func_name)
                create_workspace.execute()

                loaded_function_workspaces.append(func_name)

            except ValueError, rerr:
                logger.warning('Failed to load function {0}. Error was: {1}'.format(func_name, str(rerr)))

        # Process the loaded workspaces
        out_ws_name = self.getPropertyValue('OutputWorkspace')
        if len(loaded_function_workspaces) == 0:
            raise RuntimeError('Failed to load any functions for data')
        elif len(self._functions) == 1:
            RenameWorkspace(InputWorkspace=loaded_function_workspaces[0],
                            OutputWorkspace=out_ws_name)
        else:
            GroupWorkspaces(InputWorkspaces=loaded_function_workspaces,
                            OutputWorkspace=out_ws_name)

        # Set the output workspace
        self.setProperty('OutputWorkspace', out_ws_name)

#------------------------------------------------------------------------------

    def _setup(self):
        """
        Gets algorithm properties.
        """
        self._axis_cache = {}
        self._data_directory = self.getPropertyValue('Filename')
        self._functions = [x.strip() for x in self.getProperty('Functions').value]

#------------------------------------------------------------------------------

    def _load_function(self, function_name):
        """
        Loads a function from the data directory.

        @param function_name Name of the function to load
        @return Tuple of (Numpy array of data, unit, (v axis name, x axis name))
        @exception ValueError If function is not found
        """
        function_filename = os.path.join(self._data_directory, '{0}.dat'.format(function_name))
        if not os.path.isfile(function_filename):
            raise ValueError('File for function "{0}" not found'.format(function_name))

        data = None
        dimensions = (0, 0)
        axis = (None, None)
        unit = None

        with open(function_filename, 'rU') as f_handle:
            while True:
                line = f_handle.readline()
                if not line:
                    break

                # Ignore empty lines
                if len(line[0]) == 0:
                    pass

                # Parse header lines
                elif line[0] == '#':
                    variable_match = VARIABLE_REGEX.match(line)
                    if variable_match and variable_match.group(1) != function_name:
                        raise ValueError('Function name differs from file name')

                    axis_match = AXIS_REGEX.match(line)
                    if axis_match:
                        axis = (axis_match.group(1), axis_match.group(2))

                    unit_match = UNIT_REGEX.match(line)
                    if unit_match:
                        unit = unit_match.group(1)

                    slice_match = SLICE_2D_HEADER_REGEX.match(line)
                    if slice_match:
                        dimensions = (int(slice_match.group(1)), int(slice_match.group(2)))
                        # Now parse the data
                        data = self._load_2d_slice(f_handle, dimensions)

        return (data, unit, axis)

#------------------------------------------------------------------------------

    def _load_axis(self, axis_name):
        """
        Loads an axis by name from the data directory.

        @param axis_name Name of axis to load
        @return Tuple of (Numpy array of data, unit)
        @exception ValueError If axis is not found
        """
        if axis_name in self._axis_cache:
            return self._axis_cache[axis_name]

        axis_filename = os.path.join(self._data_directory, '{0}.dat'.format(axis_name))
        if not os.path.isfile(axis_filename):
            raise ValueError('File for axis "{0}" not found'.format(axis_name))

        data = None
        length = 0
        unit = None

        with open(axis_filename, 'rU') as f_handle:
            while True:
                line = f_handle.readline()
                if not line:
                    break

                # Ignore empty lines
                if len(line[0]) == 0:
                    pass

                # Parse header lines
                elif line[0] == '#':
                    variable_match = VARIABLE_REGEX.match(line)
                    if variable_match and variable_match.group(1) != axis_name:
                        raise ValueError('Axis name differs from file name')

                    unit_match = UNIT_REGEX.match(line)
                    if unit_match:
                        unit = unit_match.group(1)

                    slice_match = SLICE_1D_HEADER_REGEX.match(line)
                    if slice_match:
                        length = int(slice_match.group(1))
                        # Now parse the data
                        data = self._load_1d_slice(f_handle, length)

        return (data, unit)

#------------------------------------------------------------------------------

    def _load_1d_slice(self, f_handle, length):
        """
        Loads a 1D slice from the open file.

        @param f_handle Handle to the open file with the iterator at the slice header
        @param length Length of data
        @return Numpy array of length [length]
        """
        data = np.ndarray(shape=(length), dtype=float)

        for idx in range(length):
            line = f_handle.readline()

            # End of file or empty line (either way end of data)
            if not line or len(line) == 0:
                break

            data[idx] = ast.literal_eval(line)

        return data

#------------------------------------------------------------------------------

    def _load_2d_slice(self, f_handle, dimensions):
        """
        Loads a 2D slice from the open file.

        @param f_handle Handle to the open file with the iterator at the slice header
        @param dimensions Tuple containing dimensions (rows/vertical axis, cols/x axis)
        @return Numpy array of shape [dimensions]
        """
        data = np.ndarray(shape=dimensions, dtype=float)

        for v_idx in range(dimensions[0]):
            line = f_handle.readline()

            # End of file or empty line (either way end of data)
            if not line or len(line) == 0:
                break

            values = [ast.literal_eval(s) for s in line.split()]
            data[v_idx] = np.array(values)

        return data

#------------------------------------------------------------------------------

AlgorithmFactory.subscribe(LoadNMoldyn4Ascii)

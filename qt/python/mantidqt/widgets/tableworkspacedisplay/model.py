# coding=utf-8
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from __future__ import (absolute_import, division, print_function)


from mantid.dataobjects import PeaksWorkspace, TableWorkspace
from mantid.kernel import V3D
from mantidqt.widgets.tableworkspacedisplay.marked_columns import MarkedColumns


class TableWorkspaceDisplayModel:
    SPECTRUM_PLOT_LEGEND_STRING = '{}-{}'
    BIN_PLOT_LEGEND_STRING = '{}-bin-{}'

    ALLOWED_WORKSPACE_TYPES = [PeaksWorkspace, TableWorkspace]

    @classmethod
    def supports(cls, ws):
        """
        Checks that the provided workspace is supported by this display.
        :param ws: Workspace to be checked for support
        :raises ValueError: if the workspace is not supported
        """
        if not any(isinstance(ws, allowed_type) for allowed_type in cls.ALLOWED_WORKSPACE_TYPES):
            raise ValueError("The workspace type is not supported: {0}".format(ws))

    def __init__(self, ws):
        """
        Initialise the model with the workspace
        :param ws: Workspace to be used for providing data
        :raises ValueError: if the workspace is not supported
        """
        self.supports(ws)

        self.ws = ws
        self.ws_num_rows = self.ws.rowCount()
        self.ws_num_cols = self.ws.columnCount()
        self.ws_column_types = self.ws.columnTypes()
        self.conversion_functions = self.map_from_type_names(self.ws_column_types)
        self.marked_columns = MarkedColumns()
        self._original_column_headers = self.get_column_headers()

    def _get_bool_from_str(self, obj):
        if isinstance(obj, bool):
            return obj
        elif isinstance(obj, str):
            obj = obj.lower()
            if obj == "true":
                return True
            elif obj == "false":
                return False
            else:
                raise ValueError("'{}' is not a valid bool string.".format(obj))
        else:
            raise ValueError("Type '{}' cannot be converted to bool.".format(obj))

    def _get_v3d_from_str(self, string):
        if '[' in string and ']' in string:
            string = string[1:-1]
        if ',' in string:
            return V3D(*[float(x) for x in string.split(',')])
        else:
            raise RuntimeError("'{}' is not a valid V3D string.".format(string))

    def map_from_type_names(self, column_types):
        """
        :type column_types: list[str]
        :param column_types: List of all columns' types
        :raises ValueError: If the type cannot be mapped to.
        :return: List of functions to convert the data to the correct type
        """
        convert_types = []
        for type in column_types:
            type = type.lower()
            if 'int' in type:
                convert_types.append(int)
            elif 'double' in type or 'float' in type:
                convert_types.append(float)
            elif 'string' in type:
                convert_types.append(str)
            elif 'bool' in type:
                convert_types.append(self._get_bool_from_str)
            elif 'v3d' in type:
                convert_types.append(self._get_v3d_from_str)
            else:
                raise ValueError("Trying to set data for unknown column type {}".format(type))

        return convert_types

    def original_column_headers(self):
        return self._original_column_headers[:]

    def build_current_labels(self):
        return self.marked_columns.build_labels()

    def get_name(self):
        return self.ws.name()

    def get_column_headers(self):
        return self.ws.getColumnNames()

    def get_column(self, index):
        return self.ws.column(index)

    def get_number_of_rows(self):
        return self.ws_num_rows

    def get_number_of_columns(self):
        return self.ws_num_cols

    def get_column_header(self, index):
        return self.get_column_headers()[index]

    def is_peaks_workspace(self):
        return isinstance(self.ws, PeaksWorkspace)

    def set_cell_data(self, row, col, data):
        # convert the data to the correct Python type,
        # so that it can be bound to the correct C++ type by Boost
        data = self.conversion_functions[col](data)
        self.ws.setCell(row, col, data)

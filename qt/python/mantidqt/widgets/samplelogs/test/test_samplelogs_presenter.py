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

import matplotlib
matplotlib.use('Agg') # noqa: E402

from mantidqt.widgets.samplelogs.model import SampleLogsModel
from mantidqt.widgets.samplelogs.presenter import SampleLogs
from mantidqt.widgets.samplelogs.view import SampleLogsView

import unittest
try:
    from unittest import mock
except ImportError:
    import mock


class SampleLogsTest(unittest.TestCase):

    def setUp(self):
        self.view = mock.Mock(spec=SampleLogsView)
        self.view.get_row_log_name =  mock.Mock(return_value="Speed5")
        self.view.get_selected_row_indexes =  mock.Mock(return_value=[5])
        self.view.get_exp = mock.Mock(return_value=1)

        self.model = mock.Mock(spec=SampleLogsModel)
        self.model.get_ws = mock.Mock(return_value='ws')
        self.model.is_log_plottable = mock.Mock(return_value=True)
        self.model.get_statistics = mock.Mock(return_value=[1,2,3,4])
        self.model.get_exp = mock.Mock(return_value=0)

    def test_sampleLogs(self):

        presenter = SampleLogs(None, model=self.model, view=self.view)

        # setup calls
        self.assertEqual(self.view.set_model.call_count, 1)
        self.assertEqual(self.model.getItemModel.call_count, 1)

        # plot_logs
        presenter.plot_logs()
        self.model.is_log_plottable.assert_called_once_with("Speed5")
        self.assertEqual(self.model.get_ws.call_count, 1)
        self.view.plot_selected_logs.assert_called_once_with('ws', 0, [5])

        # update_stats
        presenter.update_stats()
        self.assertEqual(self.model.get_statistics.call_count, 1)
        self.view.get_row_log_name.assert_called_with(5)
        self.view.set_statistics.assert_called_once_with([1,2,3,4])
        self.assertEqual(self.view.clear_statistics.call_count, 0)

        self.view.reset_mock()
        self.view.get_selected_row_indexes =  mock.Mock(return_value=[2,5])
        presenter.update_stats()
        self.assertEqual(self.view.set_statistics.call_count, 0)
        self.assertEqual(self.view.clear_statistics.call_count, 1)

        # changeExpInfo
        self.model.reset_mock()
        self.view.reset_mock()

        presenter.changeExpInfo()
        self.assertEqual(self.view.get_selected_row_indexes.call_count, 3)
        self.assertEqual(self.view.get_exp.call_count, 1)
        self.model.set_exp.assert_called_once_with(1)
        self.view.set_selected_rows.assert_called_once_with([2,5])

        # clicked
        self.model.reset_mock()
        self.view.reset_mock()

        presenter.clicked()
        self.assertEqual(self.view.get_selected_row_indexes.call_count, 2)

        # double clicked
        self.model.reset_mock()
        self.view.reset_mock()

        index = mock.Mock()
        index.row = mock.Mock(return_value=7)

        presenter.doubleClicked(index)
        self.view.get_row_log_name.assert_called_once_with(7)
        self.model.get_log.assert_called_once_with("Speed5")


if __name__ == '__main__':
    unittest.main()

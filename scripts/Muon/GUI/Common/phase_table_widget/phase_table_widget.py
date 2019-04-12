from Muon.GUI.Common.phase_table_widget.phase_table_view import PhaseTableView
from Muon.GUI.Common.phase_table_widget.phase_table_presenter import PhaseTablePresenter

class PhaseTabWidget(object):
    def __init__(self, context, parent):
        self.phase_table_view = PhaseTableView(parent)

        self.phase_table_presenter = PhaseTablePresenter(self.phase_table_view, context)

        self.phase_table_view.set_calculate_phase_table_action(self.phase_table_presenter.handle_calulate_phase_table_clicked)
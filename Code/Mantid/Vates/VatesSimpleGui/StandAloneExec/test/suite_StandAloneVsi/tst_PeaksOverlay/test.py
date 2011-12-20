
def main():
    source(findFile("scripts", "test_helpers.py"))
    source(findFile("scripts", "common_checks.py"))
    startApplication("VatesSimpleGui")
    openFile("TOPAZ_3680_5_sec_MDEW.nxs")
    switch_mode("splatterPlot")
    check_mode_buttons(std=True, ms=True, ts=True, sp=False)

    set_ptw_lineedit_property(":ScrollArea.Number of Points_QLineEdit", 10000)
    apply_ptw_settings()

    openFile("TOPAZ_3680_10_sec_40.peaks")
    
    sendEvent("QWheelEvent", waitForObject(":renderFrame.Viewport_pqQVTKWidget"), 386, 329, 120, 0, 2)
    sendEvent("QWheelEvent", waitForObject(":renderFrame.Viewport_pqQVTKWidget"), 386, 329, 120, 0, 2)
    sendEvent("QWheelEvent", waitForObject(":renderFrame.Viewport_pqQVTKWidget"), 386, 329, 120, 0, 2)
    sendEvent("QWheelEvent", waitForObject(":renderFrame.Viewport_pqQVTKWidget"), 386, 329, 120, 0, 2)
    sendEvent("QWheelEvent", waitForObject(":renderFrame.Viewport_pqQVTKWidget"), 386, 329, 120, 0, 2)
    sendEvent("QWheelEvent", waitForObject(":renderFrame.Viewport_pqQVTKWidget"), 386, 329, 120, 0, 2)
    sendEvent("QWheelEvent", waitForObject(":renderFrame.Viewport_pqQVTKWidget"), 386, 329, 120, 0, 2)
    sendEvent("QWheelEvent", waitForObject(":renderFrame.Viewport_pqQVTKWidget"), 386, 329, 120, 0, 2)
    sendEvent("QWheelEvent", waitForObject(":renderFrame.Viewport_pqQVTKWidget"), 386, 329, 120, 0, 2)
    mouseDrag(waitForObject(":renderFrame.Viewport_pqQVTKWidget"), 384, 329, -79, 4, 1, Qt.LeftButton)


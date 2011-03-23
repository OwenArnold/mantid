#include "RebinningCutterObjectPanel.h"
#include "pqObjectPanelInterface.h"
#include "pqPropertyManager.h"
#include "pqNamedWidgets.h"
#include "vtkSMStringVectorProperty.h"
#include <QLayout>
#include "GeometryWidget.h"
#include "MantidVatesAPI/RebinningCutterPresenter.h"

  RebinningCutterObjectPanel::RebinningCutterObjectPanel(pqProxy* pxy, QWidget* p) :
    pqAutoGeneratedObjectPanel(pxy, p), m_setup(false)
  {
    //Auto generated widgets are replaced by Custom Widgets. Autogenerated ones need to be removed.
    removeAutoGeneratedWidgets();

    //Empty geometry widget added to layout.
    m_geometryWidget = new GeometryWidget;
    this->layout()->addWidget(m_geometryWidget);
  }

  void RebinningCutterObjectPanel::updateInformationAndDomains()
  {
    if (!m_setup)
    {
      this->proxy()->UpdatePropertyInformation();

      vtkSMStringVectorProperty* inputGeometryProperty = vtkSMStringVectorProperty::SafeDownCast(
          this->proxy()->GetProperty("InputGeometryXML"));

      std::string geometryXMLString = inputGeometryProperty->GetElement(0);
      m_geometryWidget->constructWidget(Mantid::VATES::getDimensions(geometryXMLString, false));

      //Get the property which is used to specify the applied x dimension.
      vtkSMProperty * appliedXDimesionXML = this->proxy()->GetProperty("AppliedXDimensionXML");

      //Get the property which is used to specify the applied y dimension.
      vtkSMProperty * appliedYDimesionXML = this->proxy()->GetProperty("AppliedYDimensionXML");

      //Get the property which is used to specify the applied z dimension.
      vtkSMProperty * appliedZDimesionXML = this->proxy()->GetProperty("AppliedZDimensionXML");

      //Hook up geometry change event to listener on filter.
      this->propertyManager()->registerLink(m_geometryWidget, "XDimensionXML",
          SIGNAL(valueChanged()), this->proxy(), appliedXDimesionXML);

      //Hook up geometry change event to listener on filter.
      this->propertyManager()->registerLink(m_geometryWidget, "YDimensionXML",
          SIGNAL(valueChanged()), this->proxy(), appliedYDimesionXML);

      //Hook up geometry change event to listener on filter.
      this->propertyManager()->registerLink(m_geometryWidget, "ZDimensionXML",
          SIGNAL(valueChanged()), this->proxy(), appliedZDimesionXML);

      m_setup = true;
    }
  }


  void RebinningCutterObjectPanel::removeAutoGeneratedWidgets()
  {
    popWidget(); // Autogenerated Z-dimension QLineEdit
    popWidget(); // Autogenerated Z-dimension QLabel
    popWidget(); // Autogenerated Y-dimension QLineEdit
    popWidget(); // Autogenerated Y-dimension QLabel
    popWidget(); // Autogenerated X-dimension QLineEdit
    popWidget(); // Autogenerated X-dimension QLabel
  }

  void RebinningCutterObjectPanel::popWidget()
  {
    //Pop the last widget off the layout and hide it.
    QLayoutItem* pLayoutItem = layout()->itemAt(layout()->count() - 1);
    QWidget* pWidget = pLayoutItem->widget();
    if (NULL == pWidget)
    {
      throw std::domain_error(
          "Error ::popWidget(). Attempting to pop a non-widget object off the layout!");
    }
    else
    {
      pWidget->setHidden(true);
      this->layout()->removeItem(pLayoutItem);
    }
  }
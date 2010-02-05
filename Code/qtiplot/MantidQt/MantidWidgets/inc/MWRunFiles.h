#ifndef MANTIDQTMANTIDWIDGETS_MWRUNFILES_H_
#define MANTIDQTMANTIDWIDGETS_MWRUNFILES_H_

#include "MantidQtMantidWidgets/ui_MWRunFiles.h"
#include "MantidQtMantidWidgets/MantidWidget.h"
#include "WidgetDllOption.h"
#include <QString>
#include <QSettings>
#include <QComboBox>

#include <QMessageBox>

namespace MantidQt
{
  namespace MantidWidgets
  {
    class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS MWRunFiles : public MantidWidget
    {
      Q_OBJECT

    public:
      MWRunFiles(QWidget *parent=NULL, QString prevSettingsGr="MWRunFiles/standardload", bool allowEmpty=false, const QComboBox * const instrum=NULL, QString label="", QString toolTip="", QString exts=DEFAULT_FILTER);
	  const std::vector<std::string>& getFileNames() const;
	  virtual QString getFile1() const;
	  
      signals:
	    void fileChanged();
		
    protected:
      /// The form generated by Qt Designer
      Ui::MWRunFiles m_designedWidg;
	  ///constains the name of the instrument that the runs files are for
	  QString m_instrument;
      /// contains the directory the user last selected to load a file from
	  QSettings m_prevSets;
	  /// A coma separtated list of file allowed files extensions for the input files
      const QString m_filter;
	  /// the tooltip passed to this function, information about which instrument's runs we load is added to the end of this string
	  const QString m_toolTipOrig;
	  /// An array of valid file names derived from the entries in the leNumber LineEdit
	  std::vector<std::string> m_files;
	  /// stores whether or not a file must be specified
	  const bool m_allowEmpty;

	  virtual QString openFileDia();
	  void readRunNumAndRanges();
	  void readComasAndHyphens(const std::string &in, std::vector<std::string> &out);

      static const QString DEFAULT_FILTER;
	  
	  protected slots:
        virtual void browseClicked();
		virtual void readEntries();
	    virtual void instrumentChange(const QString &newInstr);
	};

    class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS MWRunFile : public MWRunFiles
    {
      Q_OBJECT

    public:
	  MWRunFile(QWidget *parent=NULL, QString prevSettingsGr="MWRunFiles/standardload", bool allowEmpty=false, const QComboBox * const instrum=NULL, QString label="", QString toolTip="", QString exts=DEFAULT_FILTER);

	  /** Returns the user entered filename, throws if the file is not found or mulitiple
      *  files were entered
      *  @return a filename validated against a FileProperty
      *  @throw invalid_argument if the file couldn't be found or multiple files were entered
      */
      QString getFileName() const {return getFile1();}
      signals:
	    void fileChanged();
	  public slots:
	    void suggestFilename(const QString &newName);
	protected:
	  /// it is possible to set and change the default value for this widget, this stores the last default value given to it
	  QString m_suggestedName;
	  /// stores if the widget has been changed by the user away from its default value
	  bool m_userChange;
	  virtual QString openFileDia();
	
	private slots:
      void browseClicked();
	  void instrumentChange(const QString &);
	  void readEntries();
	};
  }
}

#endif // MANTIDQTMANTIDWIDGETS_MWRUNFILES_H_
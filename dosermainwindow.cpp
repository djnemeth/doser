#include "dosermainwindow.h"
#include <QStatusBar>
#include "doserwidget.h"

DoserMainWindow::DoserMainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	setWindowTitle("DoSer");
	setMinimumHeight(400);
	setMinimumWidth(800);
	statusBar()->showMessage("Please select the image to be segmented.");

	DoserWidget* view = new DoserWidget;
	connect(view, SIGNAL(status(const QString&)), statusBar(), SLOT(showMessage(const QString&)));
	setCentralWidget(view);
}

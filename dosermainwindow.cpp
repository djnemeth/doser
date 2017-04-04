#include "dosermainwindow.h"
#include <QStatusBar>

DoserMainWindow::DoserMainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	setWindowTitle("DoSer");
	setMinimumHeight(400);
	setMinimumWidth(750);
	statusBar()->showMessage("Please select the image to be segmented.");

	view = new DoserWidget;
	connect(view, SIGNAL(status(const QString&)), statusBar(), SLOT(showMessage(const QString&)));
	setCentralWidget(view);
}

DoserMainWindow::~DoserMainWindow()
{

}

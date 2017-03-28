#include "dosermainwindow.h"
#include <QStatusBar>

DoserMainWindow::DoserMainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	setWindowTitle("DoSer");
	view = new DoserWidget;
	setCentralWidget(view);
	setMinimumHeight(300);
	setMinimumWidth(700);
	statusBar()->showMessage("Please select the image to be segmented.");
}

DoserMainWindow::~DoserMainWindow()
{

}

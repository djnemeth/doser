#include "dosermainwindow.h"

DoserMainWindow::DoserMainWindow(QWidget *parent)
	: QMainWindow(parent)
{
	setWindowTitle("DoSer");
	view = new DoserWidget;
	setCentralWidget(view);
	setMinimumHeight(300);
	setMinimumWidth(700);
}

DoserMainWindow::~DoserMainWindow()
{

}

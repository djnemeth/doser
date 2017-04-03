#ifndef DOSERMAINWINDOW_H
#define DOSERMAINWINDOW_H

#include <QMainWindow>
#include "doserwidget.h"

class DoserMainWindow : public QMainWindow
{
	Q_OBJECT

public:
	DoserMainWindow(QWidget* parent = nullptr);
	~DoserMainWindow();

	DoserWidget* view;
};

#endif // DOSERMAINWINDOW_H

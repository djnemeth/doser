#ifndef DOSERWIDGET_H
#define DOSERWIDGET_H

#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QWidget>

enum SegmentationMode
{
	QUICK, DEEP, BOTH
};

class DoserWidget : public QWidget
{
	Q_OBJECT

public:
	static const int DEEP_COLUMN_INDEX = 2;
	static const int QUICK_COLUMN_INDEX = 3;

	explicit DoserWidget(QWidget *parent = nullptr);

signals:

private slots:
	void modeChanged();

private:
	QGroupBox* createSettingsGui();
	void displayGridColumn(int column, bool isVisible);
	QGridLayout* mainLayout;
	QComboBox* modeComboBox;
};

#endif // DOSERWIDGET_H

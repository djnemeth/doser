#ifndef DOSERWIDGET_H
#define DOSERWIDGET_H

#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMap>
#include <QVector>
#include <QWidget>

enum SegmentationMode
{
	QUICK_MODE, DEEP_MODE, BOTH_MODE
};

enum ImageLabelType
{
	SOURCE_LABEL, DEEP_LABEL, QUICK_LABEL
};

class DoserWidget : public QWidget
{
	Q_OBJECT

public:
	static const int DEEP_GROUP_COLUMN_INDEX = 2;
	static const int QUICK_GROUP_COLUMN_INDEX = 3;

	explicit DoserWidget(QWidget *parent = nullptr);

signals:

private slots:
	void modeChanged();

private:
	QGroupBox* createSettingsGui();
	QVector<QGroupBox*> createGuiGroups();
	void displayGridColumn(int column, bool isVisible);

	QGridLayout* mainLayout;
	QComboBox* modeComboBox;
	QMap<ImageLabelType, QLabel*> imageLabels;
	QLabel* deepImageLabel;
	QLabel* quickImageLabel;
};

#endif // DOSERWIDGET_H

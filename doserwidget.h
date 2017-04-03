#ifndef DOSERWIDGET_H
#define DOSERWIDGET_H

#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMap>
#include <QVector>
#include <QWidget>

#include "dosermodel.h"

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
	void changeGuiMode();
	void imageChanged(const QImage& image);
	void openImage();

private:
	QGroupBox* createSettingsGui();
	QVector<QGroupBox*> createGuiGroups();
	void displayGridColumn(int column, bool isVisible);

	DoserModel model;
	QGridLayout* mainLayout;
	QComboBox* modeComboBox;
	QMap<ImageLabelType, QLabel*> imageLabels;
};

#endif // DOSERWIDGET_H
